#!/bin/bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
CONFIGURATION="${CONFIGURATION:-Release}"

if ! command -v cmake >/dev/null 2>&1; then
    echo "Error: cmake is required to build Arolloa." >&2
    exit 1
fi

cmake -S . -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${CONFIGURATION}" "$@"
cmake --build "${BUILD_DIR}"

cat <<INFO

Build complete! Executables are located in ${BUILD_DIR}

To launch the compositor:
  ${BUILD_DIR}/launch-arolloa.sh [--debug]

The --debug flag runs Arolloa nested inside your current Wayland session.
INFO
