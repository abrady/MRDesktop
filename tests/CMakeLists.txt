# MRDesktop Tests
cmake_minimum_required(VERSION 3.20)

# Find required packages
find_package(GTest CONFIG REQUIRED)

# Check if GTest was found
if(NOT GTest_FOUND)
    message(FATAL_ERROR "GTest not found! Install it via vcpkg: vcpkg install gtest")
endif()

# Enable testing
enable_testing()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/shared
)

# Basic unit tests
add_executable(basic_tests
    test_protocol.cpp
    test_video_encoder.cpp
    test_video_decoder.cpp
    ../src/shared/VideoEncoder.cpp
    ../src/shared/VideoDecoder.cpp
)

target_include_directories(basic_tests PRIVATE ${FFMPEG_INCLUDE_DIRS})

target_link_libraries(basic_tests 
    GTest::gtest 
    GTest::gtest_main
    ${FFMPEG_LIBRARIES}
)

# Add platform-specific libraries for basic tests
if(WIN32)
    target_compile_definitions(basic_tests PRIVATE WIN32_LEAN_AND_MEAN)
    target_link_libraries(basic_tests ws2_32 mf mfplat mfuuid wmcodecdspuuid)
endif()

# Integration tests
add_executable(integration_tests
    test_network_integration.cpp
    ../src/shared/NetworkReceiver.cpp
    ../src/shared/VideoDecoder.cpp
)

target_include_directories(integration_tests PRIVATE ${FFMPEG_INCLUDE_DIRS})

target_link_libraries(integration_tests 
    GTest::gtest 
    GTest::gtest_main
    ${FFMPEG_LIBRARIES}
)

# Add platform-specific libraries for integration tests
if(WIN32)
    target_compile_definitions(integration_tests PRIVATE WIN32_LEAN_AND_MEAN)
    target_link_libraries(integration_tests ws2_32 mf mfplat mfuuid wmcodecdspuuid)
elseif(UNIX AND NOT APPLE)
    # Linux-specific libraries if needed
    target_link_libraries(integration_tests pthread)
endif()

# Register tests with CTest
add_test(NAME BasicTests COMMAND basic_tests)
add_test(NAME IntegrationTests COMMAND integration_tests)

# Test discovery for better IDE integration
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.10")
    include(GoogleTest)
    gtest_discover_tests(basic_tests)
    gtest_discover_tests(integration_tests)
endif()