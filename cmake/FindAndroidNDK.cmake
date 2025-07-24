# FindAndroidNDK.cmake - Find Android NDK installation
# This module finds the Android NDK and sets up toolchain variables

# Try to find Android NDK in common locations
set(ANDROID_NDK_SEARCH_PATHS
    $ENV{ANDROID_NDK_HOME}
    $ENV{ANDROID_NDK_ROOT}
    $ENV{ANDROID_NDK}
    "C:/Microsoft/AndroidNDK/android-ndk-r25c"
    "C:/Microsoft/AndroidNDK64/android-ndk-r25c" 
    "C:/android-ndk-r25c"
    "$ENV{LOCALAPPDATA}/Android/Sdk/ndk-bundle"
    "$ENV{LOCALAPPDATA}/Android/Sdk/ndk"
    "$ENV{HOME}/Android/Sdk/ndk-bundle"
    "$ENV{HOME}/Android/Sdk/ndk"
    "/usr/local/android-ndk"
    "/opt/android-ndk"
)

# Look for NDK directories with version numbers
file(GLOB VERSIONED_NDK_DIRS 
    "$ENV{LOCALAPPDATA}/Android/Sdk/ndk/*"
    "$ENV{HOME}/Android/Sdk/ndk/*"
    "C:/Microsoft/AndroidNDK*/android-ndk-*"
)

list(APPEND ANDROID_NDK_SEARCH_PATHS ${VERSIONED_NDK_DIRS})

# Find the NDK root directory
find_path(ANDROID_NDK_ROOT
    NAMES
        ndk-build
        ndk-build.cmd
        build/cmake/android.toolchain.cmake
    PATHS
        ${ANDROID_NDK_SEARCH_PATHS}
    DOC "Android NDK root directory"
)

if(ANDROID_NDK_ROOT)
    # Find NDK version
    if(EXISTS "${ANDROID_NDK_ROOT}/source.properties")
        file(READ "${ANDROID_NDK_ROOT}/source.properties" NDK_SOURCE_PROPERTIES)
        string(REGEX MATCH "Pkg\\.Revision = ([0-9]+\\.[0-9]+\\.[0-9]+)" _ ${NDK_SOURCE_PROPERTIES})
        set(ANDROID_NDK_VERSION ${CMAKE_MATCH_1})
    else()
        set(ANDROID_NDK_VERSION "unknown")
    endif()
    
    # Set toolchain file path
    set(ANDROID_TOOLCHAIN_FILE "${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake")
    
    # Verify toolchain file exists
    if(EXISTS "${ANDROID_TOOLCHAIN_FILE}")
        set(ANDROID_NDK_FOUND TRUE)
        message(STATUS "Found Android NDK: ${ANDROID_NDK_ROOT} (version ${ANDROID_NDK_VERSION})")
    else()
        set(ANDROID_NDK_FOUND FALSE)
        message(WARNING "Android NDK found but toolchain file missing: ${ANDROID_TOOLCHAIN_FILE}")
    endif()
else()
    set(ANDROID_NDK_FOUND FALSE)
    message(STATUS "Android NDK not found. Searched paths:")
    foreach(path ${ANDROID_NDK_SEARCH_PATHS})
        message(STATUS "  ${path}")
    endforeach()
endif()

# Handle REQUIRED argument
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AndroidNDK
    REQUIRED_VARS ANDROID_NDK_ROOT ANDROID_TOOLCHAIN_FILE
    VERSION_VAR ANDROID_NDK_VERSION
)

# Set up Android build variables if found
if(ANDROID_NDK_FOUND)
    # Common Android settings
    set(ANDROID_ABI "arm64-v8a" CACHE STRING "Android ABI")
    set(ANDROID_NATIVE_API_LEVEL "24" CACHE STRING "Android API level")
    set(ANDROID_STL "c++_shared" CACHE STRING "Android STL")
    
    # Export variables for use in presets
    set(CMAKE_TOOLCHAIN_FILE "${ANDROID_TOOLCHAIN_FILE}" CACHE FILEPATH "Android toolchain file")
    
    mark_as_advanced(
        ANDROID_NDK_ROOT
        ANDROID_TOOLCHAIN_FILE
        ANDROID_NDK_VERSION
    )
endif()