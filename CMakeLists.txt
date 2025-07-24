cmake_minimum_required(VERSION 3.20)
project(MRDesktop)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find_package(PkgConfig REQUIRED)

add_executable(mrdesktop src/main.cpp)

target_include_directories(mrdesktop PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(WIN32)
    target_compile_definitions(mrdesktop PRIVATE WIN32_LEAN_AND_MEAN)
endif()