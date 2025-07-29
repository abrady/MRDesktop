#!/usr/bin/env python3
"""
Cross-platform build script for MRDesktop project.
Replaces build.bat with platform-independent Python implementation.
"""

import argparse
import os
import platform
import subprocess
import sys
from pathlib import Path


def run_command(cmd, cwd=None):
    """Run a command and return the result."""
    try:
        if isinstance(cmd, str):
            # For string commands, use shell=True
            result = subprocess.run(
                cmd, 
                cwd=cwd, 
                shell=True, 
                check=True, 
                capture_output=True, 
                text=True
            )
        else:
            # For list commands, don't use shell
            result = subprocess.run(
                cmd, 
                cwd=cwd, 
                shell=False, 
                check=True, 
                capture_output=True, 
                text=True
            )
        return True, result.stdout
    except subprocess.CalledProcessError as e:
        print(f"Command failed: {' '.join(cmd) if isinstance(cmd, list) else cmd}")
        print(f"Error: {e.stderr}")
        return False, e.stderr


def get_build_directory(build_type, system):
    """Get the appropriate build directory for the given build type and system."""
    if system == "Windows":
        if build_type.lower() == "debug":
            return Path("build/debug")
        else:
            return Path("build/release")
    elif system == "Darwin":  # macOS
        if build_type.lower() == "debug":
            return Path("build/debug")
        else:
            return Path("build/release")
    else:  # Linux
        if build_type.lower() == "debug":
            return Path("build/linux-debug")
        else:
            return Path("build/linux-release")


def build_project(build_type):
    """Build the project using CMake."""
    system = platform.system()
    
    # Normalize build type
    build_type = "Debug" if build_type.lower() == "debug" else "Release"
    
    print(f"Building MRDesktop project...")
    print(f"Build type: {build_type}")
    print(f"Platform: {system}")
    
    # Get build directory
    build_dir = get_build_directory(build_type, system)
    
    # Check if build directory exists
    if not build_dir.exists():
        print(f"Build directory '{build_dir}' doesn't exist.")
        print("Run 'python configure.py' first.")
        return False
    
    print(f"Building in directory: {build_dir}")
    
    # Build with parallel compilation
    print("Starting build with parallel compilation...")
    success, output = run_command(["cmake", "--build", str(build_dir), "--parallel"])
    
    if not success:
        print("Build failed!")
        return False
    
    print("Build successful!")
    return True


def show_run_instructions(system, build_dir):
    """Show instructions for running the built applications."""
    print()
    print("To run the applications:")
    
    if system == "Windows":
        print("  run_server.bat           - Start the desktop server")
        print("  run_console_client.bat   - Connect to localhost")  
        print("  run_console_client.bat [IP] - Connect to specific IP")
        print()
        print("Example: run_console_client.bat 192.168.1.100")
    else:
        # For Linux/macOS, show the actual binary paths
        server_binary = build_dir / "MRDesktopServer"
        client_binary = build_dir / "MRDesktopConsoleClient"
        
        print(f"  {server_binary}         - Start the desktop server")
        print(f"  {client_binary}         - Connect to localhost")
        print(f"  {client_binary} [IP]    - Connect to specific IP")
        print()
        print(f"Example: {client_binary} 192.168.1.100")
        print()
        if system == "Linux":
            print("Or use the Linux container:")
            print("  python linux/build-linux.py build [debug|release]")


def get_cpu_count():
    """Get the number of CPU cores for parallel building."""
    try:
        return os.cpu_count() or 1
    except:
        return 1


def main():
    parser = argparse.ArgumentParser(description="Build MRDesktop project")
    parser.add_argument(
        "build_type", 
        nargs="?", 
        default="debug", 
        choices=["debug", "release", "Debug", "Release"],
        help="Build type (default: debug)"
    )
    parser.add_argument(
        "-j", "--jobs",
        type=int,
        default=0,
        help="Number of parallel build jobs (default: auto-detect)"
    )
    parser.add_argument(
        "--config",
        type=str,
        help="Specify build configuration explicitly (overrides build_type)"
    )
    
    args = parser.parse_args()
    
    # Normalize build type
    build_type = "Debug" if args.build_type.lower() == "debug" else "Release"
    if args.config:
        build_type = args.config
    
    system = platform.system()
    
    # Determine number of parallel jobs
    if args.jobs <= 0:
        jobs = get_cpu_count()
    else:
        jobs = args.jobs
    
    print(f"Using {jobs} parallel build jobs")
    
    # Build the project
    success = build_project(build_type)
    
    if success:
        build_dir = get_build_directory(build_type, system)
        show_run_instructions(system, build_dir)
        return 0
    else:
        return 1


if __name__ == "__main__":
    sys.exit(main())