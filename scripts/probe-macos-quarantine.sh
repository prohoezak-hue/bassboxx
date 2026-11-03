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
curl --fail --location --proto '=https' --proto-redir '=https' \
 https://codeload.github.com/juce-framework/JUCE/tar.gz/51a8a6d7aeae7326956d747737ccf1575e61e209 -o "$work/juce.tar.gz"
printf '%s  %s\n' 20b740351cc030086951d87614bc08b42d072f1e4db68db1ef1d988e66dbbdbe "$work/juce.tar.gz" | shasum -a 256 -c -
git archive --format=tar HEAD CMakeLists.txt Source tests | gzip -n > "$work/source.tar.gz"
xattr -w com.apple.quarantine '0083;00000000;BASSBOXX-CI;' "$work/juce.tar.gz" "$work/source.tar.gz"
mkdir "$work/bass-source" "$work/juce"
/usr/bin/tar -xzf "$work/juce.tar.gz" -C "$work/juce" --strip-components=1
/usr/bin/tar -xzf "$work/source.tar.gz" -C "$work/bass-source"
"$cmake" -S "$work/bass-source" -B "$work/bass-build" -DCMAKE_BUILD_TYPE=Release \
 '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 -DJUCE_SOURCE_DIR="$work/juce"
"$cmake" --build "$work/bass-build" --target BASSBOXX_Standalone --parallel 3
python3 - "$work/bass-build/BASSBOXX_artefacts/Release/Standalone/BASSBOXX.app" "$work/bass-build/BASSBOXX_artefacts/JuceLibraryCode" <<'PY'
import subprocess,sys
for path in sys.argv[1:]:
 data=subprocess.check_output(['xattr','-lr',path],text=True)
 lines=[line for line in data.splitlines() if 'quarantine' in line]
 if not lines:lines=['NO QUARANTINE']
 for line in lines:
  message=path+'\n'+line
  print('::notice::'+message.replace('%','%25').replace('\r','%0D').replace('\n','%0A'))
PY

app="$work/bass-build/BASSBOXX_artefacts/Release/Standalone/BASSBOXX.app"
attributes=$(xattr -lr "$app")
if [[ "$attributes" != *com.apple.quarantine:* ]]; then
 "$cmake" "-DAPP=$app" "-DREPORT=$work/probe-app.json" -P "$work/bass-source/tests/StandaloneSmoke.cmake"
 python3 - "$app" <<'PYDATA'
import subprocess,sys
s=subprocess.check_output(['xattr','-lr',sys.argv[1]],text=True)
lines=[x for x in s.splitlines() if 'quarantine' in x]
message='AFTER APP LAUNCH\n'+('\n'.join(lines) if lines else 'NO QUARANTINE')
print('::notice::'+message.replace('%','%25').replace('\r','%0D').replace('\n','%0A'))
PYDATA
fi
