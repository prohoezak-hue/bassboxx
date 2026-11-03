#!/bin/bash
set -Eeuo pipefail
work="$(mktemp -d "${RUNNER_TEMP:-/tmp}/bassboxx-metadata.XXXXXX")"
curl --fail --location --proto '=https' --proto-redir '=https' \
 https://github.com/Kitware/CMake/releases/download/v3.31.10/cmake-3.31.10-macos-universal.dmg -o "$work/cmake.dmg"
printf '%s  %s\n' dfb08721dccf5a2e0a368fd006c1a3ffb8c9e063229e24bc9d882585e4ef1068 "$work/cmake.dmg" | shasum -a 256 -c -
xattr -w com.apple.quarantine '0083;00000000;BASSBOXX-CI;' "$work/cmake.dmg"
mkdir "$work/volume" "$work/source"
hdiutil attach -readonly -nobrowse -mountpoint "$work/volume" "$work/cmake.dmg" >/dev/null
trap 'hdiutil detach "$work/volume" >/dev/null || true' EXIT
codesign --verify --deep --strict "$work/volume/CMake.app"
spctl --assess --type execute --verbose=2 "$work/volume/CMake.app"
cat > "$work/source/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.22)
project(BundleMetadata LANGUAGES CXX)
add_executable(probe MACOSX_BUNDLE main.cpp)
CMAKE
echo 'int main() { return 0; }' > "$work/source/main.cpp"
cmake="$work/volume/CMake.app/Contents/bin/cmake"
"$cmake" -S "$work/source" -B "$work/build"
"$cmake" --build "$work/build"
python3 - "$work/build/probe.app" "$work/volume/CMake.app" <<'PY'
import subprocess,sys
for path in sys.argv[1:]:
 data=subprocess.check_output(['xattr','-lr',path],text=True)
 lines=[line for line in data.splitlines() if 'quarantine' in line]
 message=path+'\n'+ ('\n'.join(lines) if lines else 'NO QUARANTINE')
 message=message[-2500:].replace('%','%25').replace('\r','%0D').replace('\n','%0A')
 print('::notice::'+message)
PY
