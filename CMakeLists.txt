cmake_minimum_required(VERSION 3.20)
project(MRDesktop)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create server executable
add_executable(MRDesktopServer src/server/main.cpp)

# Create client executable
add_executable(MRDesktopClient src/client/main.cpp)

# Set up include directories
target_include_directories(MRDesktopServer PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(MRDesktopClient PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(WIN32)
    # Server needs DXGI and other Windows APIs
    target_compile_definitions(MRDesktopServer PRIVATE WIN32_LEAN_AND_MEAN)
    target_link_libraries(MRDesktopServer PRIVATE dxgi d3d11 ws2_32)
    
    # Client for now just basic Windows
    target_compile_definitions(MRDesktopClient PRIVATE WIN32_LEAN_AND_MEAN)
    target_link_libraries(MRDesktopClient PRIVATE ws2_32)
endif()