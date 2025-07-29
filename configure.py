#!/usr/bin/env python3
"""
Cross-platform configuration script for MRDesktop project.
Replaces configure.bat with platform-independent Python implementation.
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


def initialize_vcpkg_submodule():
    """Initialize vcpkg submodule if not already done."""
    vcpkg_git_path = Path("extern/vcpkg/.git")
    
    if not vcpkg_git_path.exists():
        print("Initializing vcpkg submodule...")
        success, output = run_command(["git", "submodule", "update", "--init", "--recursive", "extern/vcpkg"])
        if not success:
            print("Failed to initialize vcpkg submodule!")
            return False
    
    return True


def bootstrap_vcpkg():
    """Bootstrap vcpkg if not already done."""
    system = platform.system()
    
    if system == "Windows":
        vcpkg_exe = Path("extern/vcpkg/vcpkg.exe")
        bootstrap_script = "extern/vcpkg/bootstrap-vcpkg.bat"
    else:
        vcpkg_exe = Path("extern/vcpkg/vcpkg")
        bootstrap_script = "extern/vcpkg/bootstrap-vcpkg.sh"
    
    if not vcpkg_exe.exists():
        print("Bootstrapping vcpkg...")
        if system != "Windows":
            # Make sure bootstrap script is executable on Unix systems
            bootstrap_path = Path(bootstrap_script)
            if bootstrap_path.exists():
                run_command(["chmod", "+x", str(bootstrap_path)])
        
        success, output = run_command([bootstrap_script])
        if not success:
            print("Failed to bootstrap vcpkg!")
            return False
    else:
        print("vcpkg already bootstrapped.")
    
    return True


def configure_cmake(build_type):
    """Configure CMake with the appropriate preset."""
    system = platform.system()
    
    if system == "Windows":
        if build_type.lower() == "debug":
            preset = "windows-debug"
        else:
            preset = "windows-release"
    elif system == "Darwin":  # macOS
        if build_type.lower() == "debug":
            preset = "macos-debug"
        else:
            preset = "macos-release"
    else:  # Linux and others
        if build_type.lower() == "debug":
            preset = "linux-debug"
        else:
            preset = "linux-release"
    
    # Check if CMakePresets.json exists
    presets_file = Path("CMakePresets.json")
    if not presets_file.exists():
        print("Error: CMakePresets.json not found!")
        print("Make sure you're running this script from the project root directory.")
        return False
    
    print(f"Configuring {system} {build_type} using preset: {preset}")
    success, output = run_command(["cmake", "--preset", preset])
    if not success:
        print(f"{system} configuration failed!")
        print("This may be due to missing build tools or dependencies.")
        if system == "Windows":
            print("Make sure Visual Studio 2022 with C++ development tools is installed.")
        elif system == "Linux":
            print("Make sure CMake, Ninja, and development tools are installed.")
            print("In container: run 'python linux/build-linux.py shell' for interactive mode.")
        else: # macOS
            print("Make sure Xcode command line tools are installed: xcode-select --install")
        return False
    
    print("CMake configuration completed successfully.")
    return True


def setup_android_environment():
    """Set up Android environment if setup script exists."""
    system = platform.system()
    
    if system == "Windows":
        android_setup = Path("android/setup_android_env.bat")
    else:
        android_setup = Path("android/setup_android_env.sh")
    
    if android_setup.exists():
        print("Sourcing Android environment...")
        success, output = run_command([str(android_setup)])
        if not success:
            print("Warning: Android environment setup failed")
            return False
    
    return True


def main():
    parser = argparse.ArgumentParser(description="Configure MRDesktop project")
    parser.add_argument(
        "build_type", 
        nargs="?", 
        default="debug", 
        choices=["debug", "release", "Debug", "Release"],
        help="Build type (default: debug)"
    )
    
    args = parser.parse_args()
    
    # Normalize build type
    build_type = "Debug" if args.build_type.lower() == "debug" else "Release"
    
    print("Configuring MRDesktop project...")
    print(f"Build type: {build_type}")
    
    # Set environment variable for vcpkg (always use release libraries)
    os.environ["VCPKG_BUILD_TYPE"] = "release"
    
    # Initialize vcpkg submodule
    if not initialize_vcpkg_submodule():
        sys.exit(1)
    
    # Bootstrap vcpkg
    if not bootstrap_vcpkg():
        sys.exit(1)
    
    # Configure with CMake
    if not configure_cmake(build_type):
        sys.exit(1)
    
    # Setup Android environment
    setup_android_environment()
    
    # Note about Android builds
    print("Android builds will use vcpkg headers manually from extern/vcpkg/installed/*/include")
    
    print("Configuration successful!")
    
    system = platform.system()
    if system == "Windows":
        print("Run 'build.bat' or 'python build.py' to build the project")
    else:
        print("Run 'python build.py' to build the project")


if __name__ == "__main__":
    main()