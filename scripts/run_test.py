#!/usr/bin/env python3
"""
Cross-platform MRDesktop Integration Test Suite.
Replaces run_test.bat with platform-independent Python implementation.
"""

import argparse
import platform
import signal
import subprocess
import sys
import time
from pathlib import Path


def find_executables(config):
    """Find server and client executables based on platform and configuration."""
    system = platform.system()
    
    if system == "Windows":
        server_name = "MRDesktopServer.exe"
        client_name = "MRDesktopConsoleClient.exe"
        if config.lower() == "debug":
            server_path = Path("../build/debug/Debug") / server_name
            client_path = Path("../build/debug/Debug") / client_name
        else:
            server_path = Path("../build/release/Release") / server_name  
            client_path = Path("../build/release/Release") / client_name
    else:  # Linux/macOS
        server_name = "MRDesktopServer"
        client_name = "MRDesktopConsoleClient"
        if system == "Linux":
            if config.lower() == "debug":
                server_path = Path("../build/linux-debug") / server_name
                client_path = Path("../build/linux-debug") / client_name
            else:
                server_path = Path("../build/linux-release") / server_name
                client_path = Path("../build/linux-release") / client_name
        else:  # macOS
            if config.lower() == "debug":
                server_path = Path("../build/debug") / server_name
                client_path = Path("../build/debug") / client_name
            else:
                server_path = Path("../build/release") / server_name
                client_path = Path("../build/release") / client_name
    
    return server_path, client_path


def kill_existing_processes():
    """Kill any existing MRDesktop processes to ensure clean test."""
    system = platform.system()
    
    if system == "Windows":
        # Use taskkill on Windows
        subprocess.run(["taskkill", "/f", "/im", "MRDesktopServer.exe"], 
                      capture_output=True, check=False)
        subprocess.run(["taskkill", "/f", "/im", "MRDesktopConsoleClient.exe"], 
                      capture_output=True, check=False)
    else:
        # Use pkill on Unix systems
        subprocess.run(["pkill", "-f", "MRDesktopServer"], 
                      capture_output=True, check=False)
        subprocess.run(["pkill", "-f", "MRDesktopConsoleClient"], 
                      capture_output=True, check=False)


def main():
    """Main function to run the integration test suite."""
    parser = argparse.ArgumentParser(description="MRDesktop Integration Test Suite")
    parser.add_argument('config', nargs='?', default='debug',
                       choices=['debug', 'release', 'Debug', 'Release'],
                       help='Build configuration (default: debug)')
    
    args = parser.parse_args()
    config = args.config.lower()
    
    print("MRDesktop Integration Test Suite")
    print("=================================")
    print()
    print(f"Configuration: {config}")
    print()
    
    # Find executables
    server_exe, client_exe = find_executables(config)
    
    # Check if binaries exist
    if not server_exe.exists():
        print(f"ERROR: Server executable not found at {server_exe}")
        print(f"Please build the project first using: python build.py {config}")
        return 1
    
    if not client_exe.exists():
        print(f"ERROR: Client executable not found at {client_exe}")
        print(f"Please build the project first using: python build.py {config}")
        return 1
    
    print(f"Found server: {server_exe}")
    print(f"Found client: {client_exe}")
    print()
    
    # Kill any existing processes to ensure clean test
    kill_existing_processes()
    
    # Start test server in background
    print("Starting test server in background...")
    try:
        server_process = subprocess.Popen([str(server_exe), "--test"])
    except Exception as e:
        print(f"ERROR: Failed to start server: {e}")
        return 1
    
    # Wait a moment for server to start
    print("Waiting for server to initialize...")
    time.sleep(2)
    
    # Check if server is still running
    if server_process.poll() is not None:
        print("ERROR: Server process exited unexpectedly")
        return 1
    
    # Run test client
    print("Starting test client...")
    try:
        client_result = subprocess.run([str(client_exe), "--test"], check=False)
        client_exit_code = client_result.returncode
    except Exception as e:
        print(f"ERROR: Failed to run client: {e}")
        client_exit_code = 1
    
    # Clean up server process
    print("Cleaning up server process...")
    try:
        server_process.terminate()
        # Give it a moment to clean up
        try:
            server_process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            # Force kill if it doesn't terminate gracefully
            server_process.kill()
    except:
        # Try system-specific cleanup as fallback
        kill_existing_processes()
    
    # Display results
    print()
    print("=================================")
    if client_exit_code == 0:
        print("TEST RESULT: PASSED")
        print("All frames were transmitted and validated successfully!")
    else:
        print("TEST RESULT: FAILED")
        print(f"Frame transmission or validation failed (exit code: {client_exit_code})")
    print("=================================")
    
    return client_exit_code


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
        kill_existing_processes()
        sys.exit(1)