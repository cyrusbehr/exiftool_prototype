cmake_minimum_required(VERSION 3.16)
project(exiftool)

set(CMAKE_CXX_STANDARD 14)

include_directories(${CMAKE_CURRENT_LIST_DIR}/cpp_exiftool/inc)

add_executable(exiftool
        main.cpp
        ${CMAKE_CURRENT_LIST_DIR}/cpp_exiftool/src/ExifTool.cpp
        ${CMAKE_CURRENT_LIST_DIR}/cpp_exiftool/src/ExifToolPipe.cpp
        ${CMAKE_CURRENT_LIST_DIR}/cpp_exiftool/src/TagInfo.cpp
        )