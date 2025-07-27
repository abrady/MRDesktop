#!/bin/bash
# Formats all C/C++ source files using clang-format with the repository style.
set -e
find src android tests \( -name '*.cpp' -o -name '*.h' \) -print0 | xargs -0 clang-format -i
