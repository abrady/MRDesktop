#!/usr/bin/env python3
"""
Linux build script using Podman/Docker.
Cross-platform Python replacement for build-linux.sh.
"""

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path


class Colors:
    """ANSI color codes for terminal output."""
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    NC = '\033[0m'  # No Color


def echo_info(message):
    """Print info message with color."""
    print(f"{Colors.GREEN}[INFO]{Colors.NC} {message}")


def echo_warn(message):
    """Print warning message with color."""
    print(f"{Colors.YELLOW}[WARN]{Colors.NC} {message}")


def echo_error(message):
    """Print error message with color."""
    print(f"{Colors.RED}[ERROR]{Colors.NC} {message}")


def run_command(cmd, shell=True, check=True, capture_output=False):
    """Run a command and handle errors."""
    try:
        result = subprocess.run(
            cmd, 
            shell=shell, 
            check=check, 
            capture_output=capture_output,
            text=True
        )
        return result
    except subprocess.CalledProcessError as e:
        echo_error(f"Command failed: {cmd}")
        if capture_output:
            echo_error(f"Error output: {e.stderr}")
        return None


def check_container_engine():
    """Check if container engine is available and return the engine name."""
    container_engine = os.environ.get('CONTAINER_ENGINE', 'podman')
    
    if not shutil.which(container_engine):
        echo_error(f"{container_engine} is not installed or not in PATH")
        echo_info("Please install Podman or Docker to continue")
        sys.exit(1)
    
    return container_engine


def image_exists(container_engine, image_name):
    """Check if container image exists."""
    result = run_command(
        f"{container_engine} images -q {image_name}",
        capture_output=True,
        check=False
    )
    return result and result.stdout.strip()


def build_image(container_engine, image_name):
    """Build the container image."""
    echo_info(f"Building {container_engine} image...")
    
    result = run_command(f"{container_engine} build -t {image_name} .", check=False)
    if result and result.returncode == 0:
        echo_info("Image built successfully")
        return True
    else:
        echo_error("Failed to build image")
        return False


def run_build(container_engine, image_name, build_type):
    """Run the build inside container."""
    echo_info(f"Running Linux build ({build_type}) in container...")
    
    # Create build directory if it doesn't exist
    Path("build").mkdir(exist_ok=True)
    
    # Get absolute path for volume mounting
    workspace_path = Path.cwd().resolve()
    
    # Use the corrected build directory structure for Linux
    build_script = f"""
        echo 'Configuring CMake for Linux {build_type}...'
        cmake --preset linux-{build_type}
        
        echo 'Building project...'
        cmake --build build/linux-{build_type} --parallel $(nproc)
        
        echo 'Build completed successfully!'
        echo 'Server binary: build/linux-{build_type}/MRDesktopServer'
        echo 'Console client binary: build/linux-{build_type}/MRDesktopConsoleClient'
    """
    
    cmd = [
        container_engine, "run", "--rm",
        "-v", f"{workspace_path}:/workspace",
        "-w", "/workspace",
        image_name,
        "bash", "-c", build_script
    ]
    
    result = run_command(cmd, shell=False, check=False)
    return result and result.returncode == 0


def run_tests(container_engine, image_name, build_type):
    """Run tests in container."""
    echo_info("Running tests in container...")
    
    workspace_path = Path.cwd().resolve()
    
    test_script = f"""
        if [ -d 'build/linux-{build_type}' ]; then
            cd build/linux-{build_type}
            ctest --output-on-failure
        else
            echo 'No build directory found. Run build first.'
            exit 1
        fi
    """
    
    cmd = [
        container_engine, "run", "--rm",
        "-v", f"{workspace_path}:/workspace",
        "-w", "/workspace",
        image_name,
        "bash", "-c", test_script
    ]
    
    result = run_command(cmd, shell=False, check=False)
    return result and result.returncode == 0


def run_shell(container_engine, image_name):
    """Run an interactive shell in the container."""
    echo_info("Starting interactive shell in container...")
    
    workspace_path = Path.cwd().resolve()
    
    cmd = [
        container_engine, "run", "--rm", "-it",
        "-v", f"{workspace_path}:/workspace",
        "-w", "/workspace",
        image_name,
        "bash"
    ]
    
    # For interactive shell, don't capture output
    result = subprocess.run(cmd, check=False)
    return result.returncode == 0


def clean_build():
    """Clean build artifacts."""
    echo_info("Cleaning build artifacts...")
    
    paths_to_remove = [
        "build/linux-debug",
        "build/linux-release",
        "install/linux-debug", 
        "install/linux-release"
    ]
    
    for path in paths_to_remove:
        path_obj = Path(path)
        if path_obj.exists():
            if path_obj.is_dir():
                shutil.rmtree(path_obj)
            else:
                path_obj.unlink()
    
    echo_info("Clean completed")


def show_usage():
    """Show usage information."""
    script_name = sys.argv[0]
    print(f"Usage: {script_name} [command] [build_type]")
    print("")
    print("Commands:")
    print("  build [debug|release]  - Build the project (default: debug)")
    print("  test  [debug|release]  - Run tests")
    print("  shell                  - Start interactive shell in container")
    print("  image                  - Build container image")
    print("  clean                  - Clean build artifacts")
    print("  help                   - Show this help")
    print("")
    print("Environment variables:")
    print("  CONTAINER_ENGINE       - Container engine to use (podman|docker, default: podman)")
    print("")
    print("Examples:")
    print(f"  {script_name} build debug         - Build debug version")
    print(f"  {script_name} build release       - Build release version")
    print(f"  {script_name} test                - Run tests")
    print(f"  {script_name} shell               - Interactive development shell")


def main():
    parser = argparse.ArgumentParser(
        description="Linux build script using Podman/Docker",
        add_help=False  # We'll handle help ourselves to match the original behavior
    )
    parser.add_argument('command', nargs='?', default='build',
                       choices=['build', 'test', 'shell', 'image', 'clean', 'help'],
                       help='Command to execute')
    parser.add_argument('build_type', nargs='?', default='debug',
                       choices=['debug', 'release'],
                       help='Build type (default: debug)')
    parser.add_argument('-h', '--help', action='store_true', help='Show help')
    
    # Parse known args to handle the case where build_type might be provided for non-build commands
    args, unknown = parser.parse_known_args()
    
    if args.help or args.command == 'help':
        show_usage()
        return 0
    
    # Configuration
    container_engine = check_container_engine()
    image_name = "mrdesktop-linux"
    
    # Handle commands
    if args.command == 'build':
        if not image_exists(container_engine, image_name):
            echo_warn("Container image not found, building it first...")
            if not build_image(container_engine, image_name):
                return 1
        
        success = run_build(container_engine, image_name, args.build_type)
        return 0 if success else 1
        
    elif args.command == 'test':
        if not image_exists(container_engine, image_name):
            echo_error(f"Container image not found. Run '{sys.argv[0]} image' first.")
            return 1
        
        success = run_tests(container_engine, image_name, args.build_type)
        return 0 if success else 1
        
    elif args.command == 'shell':
        if not image_exists(container_engine, image_name):
            echo_warn("Container image not found, building it first...")
            if not build_image(container_engine, image_name):
                return 1
        
        success = run_shell(container_engine, image_name)
        return 0 if success else 1
        
    elif args.command == 'image':
        success = build_image(container_engine, image_name)
        return 0 if success else 1
        
    elif args.command == 'clean':
        clean_build()
        return 0
        
    else:
        echo_error(f"Unknown command: {args.command}")
        show_usage()
        return 1


if __name__ == "__main__":
    sys.exit(main())