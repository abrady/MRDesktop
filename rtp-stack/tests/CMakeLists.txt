enable_testing()
add_subdirectory(third_party/googletest)
file(GLOB TEST_SRCS *.cpp)
add_executable(rtp_tests ${TEST_SRCS})

target_link_libraries(rtp_tests PRIVATE rtp_stack gtest_main)

add_test(NAME rtp_tests COMMAND rtp_tests)
