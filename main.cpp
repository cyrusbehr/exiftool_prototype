#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <memory>
#include "ExifTool.h"
#include <chrono>
#include <cstdio>
#include <stdio.h>

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

    std::cout << meta.R1 << " " << meta.R2 << " " << meta.B << " " << meta.F << " " << meta.O << std::endl;

    // Create the command
    std::string commandStr = outFilename + " -b -RawThermalImage";
    std::cout << commandStr << std::endl;
    int ret = etPtr->Command(commandStr.c_str());
    if (ret <= 0) {
        std::cout << "Unable to perform command" << std::endl;
        return -1;
    }

    // Wait max of 1 seconds
    int cmdNum = etPtr->Complete(1);
    if (cmdNum <= 0) {
        std::cout << "Unable to communicate with exiftool process" << std::endl;
        return -1;
    }
    std::cout << cmdNum << std::endl;

    std::cout << etPtr->GetError() << std::endl;

    auto outputLen = etPtr->GetOutputLen();
    std::cout << outputLen << std::endl;

    auto finish = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish-start).count() << " ms" << std::endl;

    // Cleanup and remove the temp file
    // TODO
//    remove(outFilename.c_str());

    return 0;
}
