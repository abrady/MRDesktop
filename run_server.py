#!/usr/bin/env python3
"""
Cross-platform script to run the MRDesktop Server.
Replaces run_server.bat with platform-independent Python implementation.
"""

import platform
import subprocess
import sys
from pathlib import Path


def find_server_executable():
    """Find the server executable based on platform and build configuration."""
    system = platform.system()
    
    # Platform-specific executable names and paths
    if system == "Windows":
        exe_name = "MRDesktopServer.exe"
        debug_path = Path("build/debug/Debug") / exe_name
        release_path = Path("build/release/Release") / exe_name
    else:  # Linux/macOS
        exe_name = "MRDesktopServer"
        debug_path = Path("build/linux-debug") / exe_name if system == "Linux" else Path("build/debug") / exe_name
        release_path = Path("build/linux-release") / exe_name if system == "Linux" else Path("build/release") / exe_name
    
    # Try debug first, then release
    if debug_path.exists():
        return debug_path
    elif release_path.exists():
        return release_path
    else:
        return None


def main():
    """Main function to run the MRDesktop Server."""
    print("Starting MRDesktop Server...")
    
    # Find the server executable
    server_exe = find_server_executable()
    
    if not server_exe:
        print("ERROR: MRDesktopServer executable not found!")
        print("Please build the project first:")
        print("  python configure.py")
        print("  python build.py")
        return 1
    
    print(f"Using server: {server_exe}")
    
    # Run the server
    try:
        result = subprocess.run([str(server_exe)], check=False)
        return result.returncode
    except KeyboardInterrupt:
        print("\nServer stopped by user")
        return 0
    except Exception as e:
        print(f"Error running server: {e}")
        return 1


if __name__ == "__main__":
    try:
        exit_code = main()
        
        # On Windows, pause like the original batch file
        if platform.system() == "Windows":
            input("Press Enter to continue...")
        
        sys.exit(exit_code)
    except KeyboardInterrupt:
        print("\nExiting...")
        sys.exit(0)