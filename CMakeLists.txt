cmake_minimum_required(VERSION 3.16)
project(exiftool)

set(CMAKE_CXX_STANDARD 14)

include_directories(${CMAKE_CURRENT_LIST_DIR}/cpp_exiftool/inc)

find_package(OpenCV REQUIRED)

add_executable(exiftool
        main.cpp
        ${CMAKE_CURRENT_LIST_DIR}/cpp_exiftool/src/ExifTool.cpp
        ${CMAKE_CURRENT_LIST_DIR}/cpp_exiftool/src/ExifToolPipe.cpp
        ${CMAKE_CURRENT_LIST_DIR}/cpp_exiftool/src/TagInfo.cpp
        )

target_link_libraries(exiftool ${OpenCV_LIBS})