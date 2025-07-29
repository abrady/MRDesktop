#!/usr/bin/env python3
"""
Cross-platform code formatting script using clang-format.
Replaces format.sh with platform-independent Python implementation.
Formats all C/C++ source files using clang-format with the repository style.
"""

import os
import shutil
import subprocess
import sys
from pathlib import Path


def find_clang_format():
    """Find clang-format executable."""
    # Try common names for clang-format
    candidates = ["clang-format", "clang-format-15", "clang-format-14", "clang-format-13"]
    
    for candidate in candidates:
        if shutil.which(candidate):
            return candidate
    
    return None


def find_source_files():
    """Find all C/C++ source files in the specified directories."""
    source_dirs = ["src", "android", "tests", "rtp-stack"]
    extensions = [".cpp", ".c", ".h", ".hpp", ".cc", ".cxx"]
    source_files = []
    
    for dir_name in source_dirs:
        dir_path = Path(dir_name)
        if dir_path.exists():
            for ext in extensions:
                # Use glob to find files with the extension
                source_files.extend(dir_path.rglob(f"*{ext}"))
    
    return source_files


def format_files(clang_format_cmd, files):
    """Format files using clang-format."""
    if not files:
        print("No source files found to format.")
        return True
    
    print(f"Found {len(files)} source files to format.")
    print(f"Using clang-format: {clang_format_cmd}")
    
    # Format files in batches to avoid command line length limits
    batch_size = 50
    success = True
    
    for i in range(0, len(files), batch_size):
        batch = files[i:i + batch_size]
        file_paths = [str(f) for f in batch]
        
        try:
            # Run clang-format with -i flag to format in-place
            result = subprocess.run([clang_format_cmd, "-i"] + file_paths, 
                                  check=True, capture_output=True, text=True)
            
            # Show progress
            end_idx = min(i + batch_size, len(files))
            print(f"Formatted files {i+1}-{end_idx} of {len(files)}")
            
        except subprocess.CalledProcessError as e:
            print(f"Error formatting batch {i//batch_size + 1}: {e}")
            if e.stderr:
                print(f"clang-format stderr: {e.stderr}")
            success = False
        except Exception as e:
            print(f"Unexpected error formatting batch {i//batch_size + 1}: {e}")
            success = False
    
    return success


def main():
    """Main function to format source files."""
    print("MRDesktop Code Formatter")
    print("========================")
    print()
    
    # Change to project root (script is in scripts/)
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    os.chdir(project_root)
    
    # Find clang-format
    clang_format_cmd = find_clang_format()
    if not clang_format_cmd:
        print("ERROR: clang-format not found!")
        print("Please install clang-format and ensure it's in your PATH.")
        print()
        print("Installation instructions:")
        print("  Windows: Install LLVM from https://llvm.org/builds/")
        print("  macOS:   brew install clang-format")
        print("  Linux:   apt install clang-format (or equivalent)")
        return 1
    
    # Find source files
    source_files = find_source_files()
    if not source_files:
        print("No C/C++ source files found in src/, android/, tests/, or rtp-stack/ directories.")
        return 0
    
    # Format files
    success = format_files(clang_format_cmd, source_files)
    
    if success:
        print()
        print("✓ All source files formatted successfully!")
        print("Code formatting completed using repository style (.clang-format)")
    else:
        print()
        print("✗ Some files failed to format. Please check the errors above.")
        return 1
    
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\nFormatting interrupted by user")
        sys.exit(1)