#!/usr/bin/env python3
"""
Cross-platform compilation test script.
Replaces test_compile.bat with platform-independent Python implementation.
"""

import os
import subprocess
import sys
from pathlib import Path


def run_command(cmd, description):
    """Run a command and return success status."""
    print(f"{description}...")
    try:
        result = subprocess.run(cmd, check=True, capture_output=False)
        return True
    except subprocess.CalledProcessError as e:
        print(f"ERROR: {description} failed with exit code {e.returncode}")
        return False
    except Exception as e:
        print(f"ERROR: Failed to run {description}: {e}")
        return False


def main():
    """Main function to test compilation."""
    print("Testing if the code compiles without errors...")
    print()
    
    # Change to parent directory (script is in scripts/)
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    os.chdir(project_root)
    
    # Check if build directory exists, if not run configure
    build_debug_dir = Path("build/debug")
    build_linux_debug_dir = Path("build/linux-debug")
    
    # Check for either Windows or Linux build directory
    if not build_debug_dir.exists() and not build_linux_debug_dir.exists():
        print("Build directory not found. Running configure...")
        if not run_command([sys.executable, "configure.py", "debug"], "Configuration"):
            return 1
    
    # Run build
    if not run_command([sys.executable, "build.py", "debug"], "Build"):
        return 1
    
    # Success message
    print("Build successful! Test infrastructure is ready.")
    print()
    print("You can now run the test with:")
    print("  python scripts/run_test.py [debug|release]")
    
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\nCompilation test interrupted by user")
        sys.exit(1)