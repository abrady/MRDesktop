#!/bin/bash
# Run tests for MRDesktop Linux builds

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

# Check if image exists
if [ ! "$(${CONTAINER_ENGINE} images -q ${IMAGE_NAME} 2> /dev/null)" ]; then
    echo_error "Container image not found. Run 'python linux/build-linux.py image' first."
    exit 1
fi

# Check if build exists (Linux builds use linux-prefixed directories)
if [ ! -d "build/linux-${BUILD_TYPE}" ]; then
    echo_warn "Build directory not found. Building first..."
    python linux/build-linux.py build ${BUILD_TYPE}
fi

echo_info "Running Linux tests (${BUILD_TYPE}) in container..."

${CONTAINER_ENGINE} run --rm \
    -v "$(pwd):/workspace" \
    -w /workspace \
    ${IMAGE_NAME} \
    bash -c "
        cd build/linux-${BUILD_TYPE}
        
        echo 'Running CTest...'
        ctest --output-on-failure --verbose
        
        echo ''
        echo 'Running individual test executables for detailed output...'
        
        if [ -f 'tests/basic_tests' ]; then
            echo 'Running basic_tests...'
            ./tests/basic_tests --gtest_output=xml:basic_tests_results.xml
        fi
        
        if [ -f 'tests/integration_tests' ]; then
            echo 'Running integration_tests...'
            ./tests/integration_tests --gtest_output=xml:integration_tests_results.xml
        fi
        
        echo 'Tests completed!'
    "