cmake_minimum_required(VERSION 3.20)
project(HevcGTest)

set(CMAKE_CXX_STANDARD 17)

set(GTEST_ROOT $ENV{ANDROID_NDK}/sources/third_party/googletest)
add_library(gtest STATIC ${GTEST_ROOT}/src/gtest-all.cc)
add_library(gtest_main STATIC ${GTEST_ROOT}/src/gtest_main.cc)
include_directories(${GTEST_ROOT}/include)

add_executable(hevc_gtest hevc_decode_test.cpp)

target_include_directories(hevc_gtest PRIVATE
    ../../../main/cpp
    ../../../../src/shared
    ${GTEST_ROOT}/include)

target_link_libraries(hevc_gtest
    gtest_main gtest
    mediandk
    android
    log)
