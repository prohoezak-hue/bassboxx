#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
if ! xcode-select -p >/dev/null 2>&1; then
 echo 'Install Apple Command Line Tools with: xcode-select --install, then run this again.'
 exit 1
fi
if ! command -v cmake >/dev/null; then
 echo 'CMake is required. Install it from https://cmake.org/download/ (enable command-line tools), or run: brew install cmake'
 exit 1
fi
cmake -S . -B build-mac -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build-mac --config Release --parallel 2
ctest --test-dir build-mac -C Release --output-on-failure
mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3"
ditto build-mac/BASSBOXX_artefacts/Release/VST3/BASSBOXX.vst3 "$HOME/Library/Audio/Plug-Ins/VST3/BASSBOXX.vst3"
codesign --force --deep --sign - "$HOME/Library/Audio/Plug-Ins/VST3/BASSBOXX.vst3"
echo 'BASSBOXX installed. Restart MPC and scan the user VST3 folder. See README.md.'
