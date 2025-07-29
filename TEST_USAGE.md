# MRDesktop Testing

## Quick Test
```batch
run_simple_test.bat
```
This runs a simple integration test that validates the server-client communication with predetermined test frames.

## Advanced Testing
For more testing options, see the scripts in the `scripts/` directory:
- `python scripts/run_test.py` - Full-featured test with debug/release options
- `scripts/run_test.ps1` - PowerShell version
- `scripts/verify_test_setup.bat` - Setup verification
- `python scripts/test_compile.py` - Build verification

## What the Test Does
1. **Server Test Mode**: Generates 640x480 test frames with red-green gradient pattern
2. **Client Test Mode**: Validates received frames (dimensions, data size, pixel values)
3. **Result**: Returns success/failure based on whether all 3 test frames were properly transmitted and validated

The test confirms your desktop streaming pipeline works end-to-end without requiring actual desktop capture.