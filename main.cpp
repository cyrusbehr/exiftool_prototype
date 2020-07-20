#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <memory>
#include "ExifTool.h"
#include <chrono>
#include <cstdio>
#include <stdio.h>
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
    const std::string filepath = "/home/cyrus/work/IRYX/prototype/exiftool/img.txt";

    // Read it into a buffer
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
       std::cout << "Unable to read file to buffer" << std::endl;
    }


    auto etPtr = std::make_unique<ExifTool>();

    auto start = std::chrono::high_resolution_clock::now();

    // Write the buffer to a temporary file.
    // Get temp filename
    std::string outFilename = std::tmpnam(nullptr);
    std::ofstream outfile (outFilename,std::ofstream::binary);
    outfile.write (buffer.data() ,size);
    outfile.close();

    TagInfo* info = etPtr->ImageInfo(outFilename.c_str());

    Metadata meta;

    if (info) {
        // print returned information
        for (TagInfo *i=info; i; i=i->next) {
            std::cout << i->name << " " << i->value << std::endl;

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

    std::cout << meta.R1 << " " << meta.R2 << " " << meta.B << " " << meta.F << " " << meta.O << std::endl;

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
                std::cout << i->name << " valueLen: " << i->valueLen << " numLen: " << i->numLen << std::endl;
                tiffFilename = cv::tempfile(".tiff");
                std::cout << tiffFilename << std::endl;
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
    if (thermalImg.depth() == CV_16U) {
        std::cout << "cv_16u" << std::endl;
    }

    thermalImg.convertTo(thermalImg, CV_32FC1);

    cv::Mat loggedMat;
    cv::log( meta.R1 / (meta.R2 * (thermalImg + meta.O)) + meta.F, loggedMat);
    cv::Mat kelvinMat = meta.B / loggedMat;

    std::cout << kelvinMat.at<float>(0,0) - 273.15 << std::endl;

    auto finish = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish-start).count() << " ms" << std::endl;

    remove(outFilename.c_str());

    return 0;
}
