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

using namespace std::chrono;
using namespace std;

struct Metadata {
    // Plancks constants
    float R1;
    float R2;
    float B;
    float F;
    float O;
};

int main() {
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
            if (info) {
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

            // Can go up to 479, 639
//    std::cout << kelvinMat.at<float>(479, 639) - 273.15 << std::endl;
//    std::cout << kelvinMat.at<float>(0, 0) - 273.15 << std::endl;

            // Find the max temp in the frame
            float maxTemp = 0;
            for (int row = 0; row < celciusMat.rows; ++row) {
                for (int col = 0; col < celciusMat.cols; ++col) {
                    if (celciusMat.at<float>(row, col) > maxTemp) {
                        maxTemp = celciusMat.at<float>(row, col);
                    }
                }
            }

            std::cout << "The max temp is: " << maxTemp << std::endl;

            auto finish = std::chrono::high_resolution_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish-start).count() << " ms" << std::endl;

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
