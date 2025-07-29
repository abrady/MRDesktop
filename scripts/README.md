# MRDesktop Test Scripts

This directory contains auxiliary test and build scripts for the MRDesktop project.

## Scripts Overview

### `run_test.py`
Full-featured test script that supports both debug and release configurations.
- **Usage**: `python run_test.py [debug|release]`
- **Features**: 
  - Configuration selection
  - Process cleanup
  - Detailed logging
  - Error handling

### `verify_test_setup.bat`
Verification script to check if the test infrastructure is properly set up.
- **Usage**: `verify_test_setup.bat`
- **Purpose**: Verify executables exist and test mode functionality works

### `test_compile.py`
Build verification script to ensure the project compiles without errors.
- **Usage**: `python test_compile.py`
- **Purpose**: Configure and build the project, reporting any compilation issues

## Quick Test

For a simple test, use the main script in the root directory:
```batch
cd ..
run_simple_test.bat
```

## Full Test

For advanced testing with configuration options:
```bash
# From scripts directory
python run_test.py debug
```