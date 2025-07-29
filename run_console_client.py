#!/usr/bin/env python3
"""
Cross-platform script to run the MRDesktop Console Client.
Replaces run_console_client.bat with platform-independent Python implementation.
"""

import argparse
import platform
import subprocess
import sys
from pathlib import Path


def find_client_executable():
    """Find the console client executable based on platform and build configuration."""
    system = platform.system()
    
    # Platform-specific executable names and paths
    if system == "Windows":
        exe_name = "MRDesktopConsoleClient.exe"
        debug_path = Path("build/debug/Debug") / exe_name
        release_path = Path("build/release/Release") / exe_name
    else:  # Linux/macOS
        exe_name = "MRDesktopConsoleClient"
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
    """Main function to run the MRDesktop Console Client."""
    parser = argparse.ArgumentParser(
        description="Run MRDesktop Console Client",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python run_console_client.py                    # Connect to localhost:8080
  python run_console_client.py 192.168.1.100     # Connect to specific IP
  python run_console_client.py --ip 192.168.1.50 # Connect using --ip flag
        """
    )
    parser.add_argument('server_ip', nargs='?', help='Server IP address (default: localhost)')
    parser.add_argument('--ip', dest='server_ip_flag', help='Server IP address (alternative syntax)')
    
    args = parser.parse_args()
    
    # Determine server IP - prioritize positional argument over flag
    server_ip = args.server_ip or args.server_ip_flag
    
    # Find the client executable
    client_exe = find_client_executable()
    
    if not client_exe:
        print("ERROR: MRDesktopConsoleClient executable not found!")
        print("Please build the project first:")
        print("  python configure.py")
        print("  python build.py")
        return 1
    
    # Prepare command and display info
    cmd = [str(client_exe)]
    
    if server_ip:
        print(f"Starting MRDesktop Console Client - {server_ip}:8080")
        cmd.append(f"--ip={server_ip}")
    else:
        print("Starting MRDesktop Console Client - localhost:8080")
    
    print("This client provides keyboard-only control (WASD/arrows for mouse)")
    print(f"Using client: {client_exe}")
    
    # Run the client
    try:
        result = subprocess.run(cmd, check=False)
        return result.returncode
    except KeyboardInterrupt:
        print("\nClient stopped by user")
        return 0
    except Exception as e:
        print(f"Error running client: {e}")
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