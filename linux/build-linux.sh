#!/bin/bash
# Linux build script using Podman/Docker

set -e

# Script configuration
CONTAINER_ENGINE=${CONTAINER_ENGINE:-"podman"}
IMAGE_NAME="mrdesktop-linux"
BUILD_TYPE=${1:-"debug"}

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

echo_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

echo_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if container engine is available
if ! command -v ${CONTAINER_ENGINE} &> /dev/null; then
    echo_error "${CONTAINER_ENGINE} is not installed or not in PATH"
    echo_info "Please install Podman or Docker to continue"
    exit 1
fi

# Function to build the container image
build_image() {
    echo_info "Building ${CONTAINER_ENGINE} image..."
    ${CONTAINER_ENGINE} build -t ${IMAGE_NAME} .
    if [ $? -eq 0 ]; then
        echo_info "Image built successfully"
    else
        echo_error "Failed to build image"
        exit 1
    fi
}

# Function to run the build inside container
run_build() {
    echo_info "Running Linux build (${BUILD_TYPE}) in container..."
    
    # Create build directory if it doesn't exist
    mkdir -p build

    ${CONTAINER_ENGINE} run --rm \
        -v "$(pwd):/workspace" \
        -w /workspace \
        ${IMAGE_NAME} \
        bash -c "
            echo 'Configuring CMake for Linux ${BUILD_TYPE}...'
            cmake --preset linux-${BUILD_TYPE}
            
            echo 'Building project...'
            cmake --build build/${BUILD_TYPE} --parallel \$(nproc)
            
            echo 'Build completed successfully!'
            echo 'Server binary: build/${BUILD_TYPE}/MRDesktopServer'
            echo 'Console client binary: build/${BUILD_TYPE}/MRDesktopConsoleClient'
        "
}

# Function to run tests
run_tests() {
    echo_info "Running tests in container..."
    
    ${CONTAINER_ENGINE} run --rm \
        -v "$(pwd):/workspace" \
        -w /workspace \
        ${IMAGE_NAME} \
        bash -c "
            if [ -d 'build/${BUILD_TYPE}' ]; then
                cd build/${BUILD_TYPE}
                ctest --output-on-failure
            else
                echo 'No build directory found. Run build first.'
                exit 1
            fi
        "
}

# Function to run an interactive shell in the container
run_shell() {
    echo_info "Starting interactive shell in container..."
    
    ${CONTAINER_ENGINE} run --rm -it \
        -v "$(pwd):/workspace" \
        -w /workspace \
        ${IMAGE_NAME} \
        bash
}

# Function to clean build artifacts
clean_build() {
    echo_info "Cleaning build artifacts..."
    rm -rf build/debug build/release install/debug install/release
    echo_info "Clean completed"
}

# Show usage information
show_usage() {
    echo "Usage: $0 [command] [build_type]"
    echo ""
    echo "Commands:"
    echo "  build [debug|release]  - Build the project (default: debug)"
    echo "  test  [debug|release]  - Run tests"
    echo "  shell                  - Start interactive shell in container"
    echo "  image                  - Build container image"
    echo "  clean                  - Clean build artifacts"
    echo "  help                   - Show this help"
    echo ""
    echo "Environment variables:"
    echo "  CONTAINER_ENGINE       - Container engine to use (podman|docker, default: podman)"
    echo ""
    echo "Examples:"
    echo "  $0 build debug         - Build debug version"
    echo "  $0 build release       - Build release version"
    echo "  $0 test                - Run tests"
    echo "  $0 shell               - Interactive development shell"
}

# Main script logic
case "${1:-build}" in
    "build")
        if [ ! "$(${CONTAINER_ENGINE} images -q ${IMAGE_NAME} 2> /dev/null)" ]; then
            echo_warn "Container image not found, building it first..."
            build_image
        fi
        run_build
        ;;
    "test")
        if [ ! "$(${CONTAINER_ENGINE} images -q ${IMAGE_NAME} 2> /dev/null)" ]; then
            echo_error "Container image not found. Run '$0 image' first."
            exit 1
        fi
        run_tests
        ;;
    "shell")
        if [ ! "$(${CONTAINER_ENGINE} images -q ${IMAGE_NAME} 2> /dev/null)" ]; then
            echo_warn "Container image not found, building it first..."
            build_image
        fi
        run_shell
        ;;
    "image")
        build_image
        ;;
    "clean")
        clean_build
        ;;
    "help"|"-h"|"--help")
        show_usage
        ;;
    *)
        echo_error "Unknown command: $1"
        show_usage
        exit 1
        ;;
esac