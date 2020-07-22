#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <memory>
#include "ExifTool.h"
#include <chrono>
#include <cstdio>
#include <stdio.h>
#include "curlpp/Options.hpp"
#include <cstring>
#include <opencv2/opencv.hpp>
#include "tf_sdk.h"

using namespace std::chrono;
using namespace std;

struct ROI {
    ROI() = default;
    ROI(const Trueface::FaceBoxAndLandmarks& fb) {
        x1 = fb.topLeft.x;
        y1 = fb.topLeft.y;
        x2 = fb.bottomRight.x;
        y2 = fb.bottomRight.y;
    }
    // Top left
    int x1 = 0;
    int y1 = 0;
    // Bottom right
    int x2 = 0;
    int y2 = 0;
};

bool getEyeduct(const cv::Mat& img, ROI& eyeduct, std::unique_ptr<Trueface::SDK>& m_tfSDK) {
    auto errorCode = m_tfSDK->setImage(img.data, img.cols, img.rows, Trueface::ColorCode::bgr);
    if (errorCode != Trueface::ErrorCode::NO_ERROR) {
//        m_logger->warn("Unable to set the temperature image");
        return false;
    }

    bool found;
    Trueface::FaceBoxAndLandmarks faceBoxAndLandmarks;
    m_tfSDK->detectLargestFace(faceBoxAndLandmarks, found);
    if (!found) {
//        std::cout << "No face found in temperature frame!" << std::endl;
//        m_logger->debug("No face found in the temperature frame");
        return false;
    }

//    std::cout << "Face detected!" << std::endl;

    float yaw, pitch, roll;
    errorCode = m_tfSDK->estimateHeadOrientation(faceBoxAndLandmarks, yaw, pitch, roll);
    if (errorCode != Trueface::ErrorCode::NO_ERROR) {
//        m_logger->warn("Unable to compute thermal image orientation");
        return false;
    }


    if (yaw < -0.75 || yaw > 0.75) {
//        m_logger->debug("Yaw angle too extreme");
        return false;
    }

    const auto& leftEye = faceBoxAndLandmarks.landmarks[0];
    const auto& rightEye = faceBoxAndLandmarks.landmarks[1];

    // Use half the distance from the left eye to right eye to generate the box.
    const size_t sideLength = (rightEye.x - leftEye.x) / 1.75; // Use 1.75 instead of 2 to ensure we capture the eye duct region

    if (yaw > 0.f) {
        eyeduct.x1 = leftEye.x;
        eyeduct.x2 = leftEye.x + sideLength;
        eyeduct.y1 = leftEye.y - sideLength / 2;
        if (eyeduct.y1 < 0) {
            eyeduct.y1 = 0;
        }
        eyeduct.y2 = leftEye.y + sideLength / 2;
    } else {
        eyeduct.x1 = rightEye.x - sideLength;
        eyeduct.x2 = rightEye.x;
        eyeduct.y1 = rightEye.y - sideLength / 2;
        if (eyeduct.y1 < 0) {
            eyeduct.y1 = 0;
        }
        eyeduct.y2 = rightEye.y + sideLength / 2;
    }

    return true;
}

struct Metadata {
    // Plancks constants
    float R1;
    float R2;
    float B;
    float F;
    float O;
};

int main() {
    // Initialize the SDK
    Trueface::ConfigurationOptions options;
    options.frModel = Trueface::FacialRecognitionModel::LITE;
    options.smallestFaceHeight = 40;
    auto m_tfSDK = std::make_unique<Trueface::SDK>(options);

    auto valid = m_tfSDK->setLicense("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJlbW90aW9uIjp0cnVlLCJmciI6dHJ1ZSwiYnlwYXNzX2dwdV91dWlkIjp0cnVlLCJwYWNrYWdlX2lkIjpudWxsLCJleHBpcnlfZGF0ZSI6IjIwMjEtMDEtMDEiLCJncHVfdXVpZCI6W10sInRocmVhdF9kZXRlY3Rpb24iOnRydWUsIm1hY2hpbmVzIjoxLCJhbHByIjp0cnVlLCJuYW1lIjoiQ3lydXMgR1BVIiwidGtleSI6Im5ldyIsImV4cGlyeV90aW1lX3N0YW1wIjoxNjA5NDU5MjAwLjAsImF0dHJpYnV0ZXMiOnRydWUsInR5cGUiOiJvZmZsaW5lIiwiZW1haWwiOiJjeXJ1c0B0cnVlZmFjZS5haSJ9.2uQZTG_AXcHVXEFahbvkM8-gmosLPxjSSnbfEAz5gpY");
    if (!valid) {
        std::string errMsg = "Token is not valid or is expired!";
//        m_logger->error(errMsg);
        throw std::runtime_error(errMsg);
    }

    const std::string IP = "192.168.0.5";
    const std::string endpoint = "http://" + IP + "/api/image/current?tempUnit=C&overlay=off";
    auto etPtr = std::make_unique<ExifTool>();

    bool run = true;
    while(run) {
        try {
            auto start = std::chrono::high_resolution_clock::now();
            std::ostringstream os;
            os << curlpp::options::Url(endpoint);
            std::string imgStr = os.str();
            std::vector<char> imgBuffer(imgStr.begin(), imgStr.end());

            // Get temp filename
            std::string outFilename = std::tmpnam(nullptr);
            std::ofstream outfile (outFilename,std::ofstream::binary);
            outfile.write (imgBuffer.data() ,imgBuffer.size());
            outfile.close();

            // Write the buffer to a temporary file to simulate scenario where we get buffer from http request.

            TagInfo* info = etPtr->ImageInfo(outFilename.c_str());

            Metadata meta;

            if (info) {
                // print returned information
                for (TagInfo *i=info; i; i=i->next) {
                    if (std::string(i->name) == "PlanckR1") {
                        meta.R1 = std::stof(i->value);
                    } else if (std::string(i->name) == "PlanckR2") {
                        meta.R2 = std::stof(i->value);
                    } else if (std::string(i->name) == "PlanckB") {
                        meta.B = std::stof(i->value);
                    } else if (std::string(i->name) == "PlanckF") {
                        meta.F = std::stof(i->value);
                    } else if (std::string(i->name) == "PlanckO") {
                        meta.O = std::stof(i->value);
                    }
                }

                // we are responsible for deleting the information when done
                delete info;
            } else if (etPtr->LastComplete() <= 0) {
                cerr << "Error executing exiftool!" << endl;
            }

//            std::cout << meta.R1 << " " << meta.R2 << " " << meta.B << " " << meta.F << " " << meta.O << std::endl;

            // Extract the binary data
            int cmdNum = etPtr->ExtractInfo(outFilename.c_str(), "-b\n-RawThermalImage");
            if (cmdNum < 0) {
                std::cout << "There was an error issuing the command" << std::endl;
                return -1;
            }

            bool foundTag = false;
            std::string tiffFilename;

            // Wait up to 1 seconds
            TagInfo *myinfo = etPtr->GetInfo(cmdNum, 1);
            if (myinfo) {
                for (TagInfo *i = myinfo; i; i = i->next) {
                    if (std::string(i->name) == "RawThermalImage") {
                        foundTag = true;
//                        std::cout << i->name << " valueLen: " << i->valueLen << " numLen: " << i->numLen << std::endl;
                        tiffFilename = cv::tempfile(".tiff");
//                        std::cout << tiffFilename << std::endl;
                        std::ofstream tiffFile;
                        tiffFile.open(tiffFilename, std::ios::out | std::ios::binary);
                        tiffFile.write(i->value, i->valueLen);
                        tiffFile.close();
                    }
                }
                // we are responsible for deleting the information when done
                delete myinfo;
            } else {
                std::cout << etPtr->GetError() << std::endl;
                cerr << "Error executing exiftool!" << endl;
            }

            if (!foundTag) {
                std::cout << "Unable to find tag" << std::endl;
                return -1;
            }

            cv::Mat thermalImg = cv::imread(tiffFilename, cv::IMREAD_UNCHANGED);

            thermalImg.convertTo(thermalImg, CV_32FC1);

            cv::Mat loggedMat;
            cv::log( meta.R1 / (meta.R2 * (thermalImg + meta.O)) + meta.F, loggedMat);
            cv::Mat celciusMat = meta.B / loggedMat - 273.15;


            auto finish = std::chrono::high_resolution_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish-start).count() << " ms" << std::endl;

            ROI eyeduct;
            cv::Mat img = cv::imread(outFilename);
            bool didFindFace = getEyeduct(img, eyeduct, m_tfSDK);
            if (!didFindFace) {
                std::cout << "No face found" << std::endl;
                continue;
            }

            cv::Rect roi (eyeduct.x1, eyeduct.y1, eyeduct.x2 - eyeduct.x1, eyeduct.y2 - eyeduct.y1);
            cv::Mat tempCropped = celciusMat(roi);

            float maxTemp = 0;
            for (int row = 0; row < tempCropped.rows; ++row) {
                for (int col = 0; col < tempCropped.cols; ++col) {
                    if (tempCropped.at<float>(row, col) > maxTemp) {
                        maxTemp = tempCropped.at<float>(row, col);
                    }
                }
            }

            std::cout << "--------- Eyeduct Temp ------------" << std::endl;
            std::cout << maxTemp << " c" << std::endl;
            std::cout << "-----------------------------------" << std::endl;


            remove(outFilename.c_str());
        } catch( curlpp::RuntimeError &e ) {
            std::cout << "------------ERROR----------" << std::endl;
            std::cout << e.what() << std::endl;
            std::cout << "------------ERROR----------" << std::endl;
        } catch( curlpp::LogicError &e ) {
            std::cout << "------------ERROR----------" << std::endl;
            std::cout << e.what() << std::endl;
            std::cout << "------------ERROR----------" << std::endl;
        }
    }

    return 0;
}
