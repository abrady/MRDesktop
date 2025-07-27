#!/bin/bash

# MRDesktop Linux Development Container Script
# This script creates a Linux development environment with the project mounted as a volume
# so changes sync between container and host.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
CONTAINER_NAME="mrdesktop-dev"
IMAGE_NAME="mrdesktop:linux-dev"

echo "MRDesktop Linux Development Container"
echo "====================================="
echo "Project root: $PROJECT_ROOT"
echo

# Function to check if Podman is available
check_podman() {
    if ! podman info >/dev/null 2>&1; then
        echo "Error: Podman is not running or not accessible"
        echo "Please install/start Podman and try again"
        exit 1
    fi
}

# Function to build the Docker image
build_image() {
    echo "Building Docker image..."
    cd "$SCRIPT_DIR"
    podman build -t "$IMAGE_NAME" .
    echo "Docker image built successfully!"
    echo
}

# Function to check if image exists
image_exists() {
    podman image inspect "$IMAGE_NAME" >/dev/null 2>&1
}

# Function to stop and remove existing container
cleanup_container() {
    if podman ps -a --format "table {{.Names}}" | grep -q "^$CONTAINER_NAME$"; then
        echo "Stopping and removing existing container..."
        podman stop "$CONTAINER_NAME" >/dev/null 2>&1 || true
        podman rm "$CONTAINER_NAME" >/dev/null 2>&1 || true
    fi
}

# Function to run the development container
run_container() {
    echo "Starting development container..."
    echo "Project directory will be mounted at /workspace"
    echo "Any changes made in the container will sync with your host filesystem."
    echo
    
    podman run -it \
        --name "$CONTAINER_NAME" \
        --rm \
        -v "$PROJECT_ROOT:/workspace" \
        -w /workspace \
        -p 8080:8080 \
        -p 8081:8081 \
        --cap-add=SYS_PTRACE \
        "$IMAGE_NAME" \
        /bin/bash
}

# Main execution
main() {
    check_podman
    
    # Parse command line arguments
    case "${1:-}" in
        "build")
            build_image
            ;;
        "clean")
            cleanup_container
            if image_exists; then
                echo "Removing Docker image..."
                podman rmi "$IMAGE_NAME"
                echo "Image removed!"
            fi
            ;;
        "shell"|"")
            # Build image if it doesn't exist
            if ! image_exists; then
                echo "Docker image not found. Building..."
                build_image
            fi
            
            cleanup_container
            run_container
            ;;
        "help"|"-h"|"--help")
            echo "Usage: $0 [command]"
            echo
            echo "Commands:"
            echo "  shell     Start development container (default)"
            echo "  build     Build the Docker image"
            echo "  clean     Remove container and image"
            echo "  help      Show this help message"
            echo
            echo "The container mounts your project directory at /workspace"
            echo "Changes made in the container sync with your host filesystem."
            ;;
        *)
            echo "Unknown command: $1"
            echo "Use '$0 help' for usage information"
            exit 1
            ;;
    esac
}

main "$@"