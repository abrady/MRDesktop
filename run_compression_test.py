#!/usr/bin/env python3
"""
Cross-platform MRDesktop H.265 Compression Test.
Replaces run_compression_test.sh with platform-independent Python implementation.
"""

import platform
import signal
import subprocess
import sys
import time
from pathlib import Path


def find_executables():
    """Find server and client executables based on platform."""
    system = platform.system()
    
    if system == "Windows":
        server_name = "MRDesktopServer.exe"
        client_name = "MRDesktopConsoleClient.exe"
        # Try debug first, then release
        debug_server = Path("build/debug/Debug") / server_name
        debug_client = Path("build/debug/Debug") / client_name
        release_server = Path("build/release/Release") / server_name
        release_client = Path("build/release/Release") / client_name
    else:  # Linux/macOS
        server_name = "MRDesktopServer"
        client_name = "MRDesktopConsoleClient"
        if system == "Linux":
            debug_server = Path("build/linux-debug") / server_name
            debug_client = Path("build/linux-debug") / client_name
            release_server = Path("build/linux-release") / server_name
            release_client = Path("build/linux-release") / client_name
        else:  # macOS
            debug_server = Path("build/debug") / server_name
            debug_client = Path("build/debug") / client_name
            release_server = Path("build/release") / server_name
            release_client = Path("build/release") / client_name
    
    # Try debug first, then release
    if debug_server.exists() and debug_client.exists():
        return debug_server, debug_client
    elif release_server.exists() and release_client.exists():
        return release_server, release_client
    else:
        return None, None


def kill_existing_processes():
    """Kill any existing MRDesktop processes to ensure clean test."""
    system = platform.system()
    
    if system == "Windows":
        subprocess.run(["taskkill", "/f", "/im", "MRDesktopServer.exe"], 
                      capture_output=True, check=False)
        subprocess.run(["taskkill", "/f", "/im", "MRDesktopConsoleClient.exe"], 
                      capture_output=True, check=False)
    else:
        subprocess.run(["pkill", "-f", "MRDesktopServer"], 
                      capture_output=True, check=False)
        subprocess.run(["pkill", "-f", "MRDesktopConsoleClient"], 
                      capture_output=True, check=False)


def main():
    """Main function to run the H.265 compression test."""
    print("MRDesktop H.265 Compression Test")
    print("==================================")
    print()
    
    # Find executables
    server_exe, client_exe = find_executables()
    
    if not server_exe or not client_exe:
        print("ERROR: Server or client executable not found!")
        print("Please build the project first:")
        print("  python configure.py")
        print("  python build.py")
        return 1
    
    print(f"Using server: {server_exe}")
    print(f"Using client: {client_exe}")
    print()
    
    # Clean up any existing processes
    kill_existing_processes()
    
    # Start server in test mode
    print("Starting server in test mode...")
    try:
        server_process = subprocess.Popen([str(server_exe), "--test"])
    except Exception as e:
        print(f"ERROR: Failed to start server: {e}")
        return 1
    
    # Wait for server to start
    print("Waiting 3 seconds for server to start...")
    time.sleep(3)
    
    # Check if server is still running
    if server_process.poll() is not None:
        print("ERROR: Server process exited unexpectedly")
        return 1
    
    # Run client with H.265 compression
    print("Starting client in test mode with H.265 compression...")
    try:
        client_result = subprocess.run([str(client_exe), "--test", "--compression=h265"], 
                                     check=False)
        client_exit_code = client_result.returncode
    except Exception as e:
        print(f"ERROR: Failed to run client: {e}")
        client_exit_code = 1
    
    print()
    print("Cleaning up server process...")
    
    # Clean up server process
    try:
        server_process.terminate()
        # Give it a moment to clean up gracefully
        try:
            server_process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            # Force kill if it doesn't terminate gracefully
            server_process.kill()
            server_process.wait()
    except:
        # Try system-specific cleanup as fallback
        kill_existing_processes()
    
    print()
    if client_exit_code == 0:
        print("H.265 COMPRESSION TEST PASSED!")
        print("Compressed frames were successfully encoded and decoded.")
    else:
        print(f"H.265 COMPRESSION TEST FAILED with exit code {client_exit_code}")
    
    print()
    return client_exit_code


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\nCompression test interrupted by user")
        kill_existing_processes()
        sys.exit(1)