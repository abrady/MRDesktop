# MRDesktop Test Scripts

This directory contains auxiliary test and build scripts for the MRDesktop project.

## Scripts Overview

### `run_test.bat` / `run_test.ps1`
Full-featured test scripts that support both debug and release configurations.
- **Usage**: `run_test.bat [debug|release]` or `powershell .\run_test.ps1 [-Config debug|release]`
- **Features**: 
  - Configuration selection
  - Process cleanup
  - Detailed logging
  - Error handling

### `verify_test_setup.bat`
Verification script to check if the test infrastructure is properly set up.
- **Usage**: `verify_test_setup.bat`
- **Purpose**: Verify executables exist and test mode functionality works

### `test_compile.bat`
Build verification script to ensure the project compiles without errors.
- **Usage**: `test_compile.bat`
- **Purpose**: Configure and build the project, reporting any compilation issues

## Quick Test

For a simple test, use the main script in the root directory:
```batch
cd ..
run_simple_test.bat
```

## Full Test

For advanced testing with configuration options:
```batch
# From scripts directory
run_test.bat debug

# Or PowerShell version
powershell .\run_test.ps1 -Config debug
```