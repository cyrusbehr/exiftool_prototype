set(ENV{CC} "/usr/bin/gcc-7")
set(ENV{CXX} "/usr/bin/g++-7")

cmake_minimum_required(VERSION 3.16)
project(exiftool)


set(CMAKE_CXX_STANDARD 14)

include_directories(${CMAKE_CURRENT_LIST_DIR}/cpp_exiftool/inc)

include_directories("/home/cyrus/work/IRYX/prototype/frame_reader_a400/curlpp-0.8.1/include")
link_directories("/home/cyrus/work/IRYX/prototype/frame_reader_a400/curlpp-0.8.1/build")

find_package(OpenCV REQUIRED)
find_package(CURL REQUIRED)

add_executable(exiftool
        main.cpp
        ${CMAKE_CURRENT_LIST_DIR}/cpp_exiftool/src/ExifTool.cpp
        ${CMAKE_CURRENT_LIST_DIR}/cpp_exiftool/src/ExifToolPipe.cpp
        ${CMAKE_CURRENT_LIST_DIR}/cpp_exiftool/src/TagInfo.cpp
        )

target_link_libraries(exiftool ${OpenCV_LIBS} libcurlpp.a ${CURL_LIBRARIES})