#!/bin/bash
set -Eeuo pipefail
trap 'status=$?; echo "::error file=scripts/package-macos.sh,line=$LINENO::Packaging failed (exit $status): $BASH_COMMAND"; exit "$status"' ERR
cd "$(dirname "$0")/.."
[[ "$(uname -s)" == Darwin ]] || { echo 'Packaging requires macOS'; exit 1; }
plugin="${1:-build/BASSBOXX_artefacts/Release/VST3/BASSBOXX.vst3}"
binary="$plugin/Contents/MacOS/BASSBOXX"
if [[ ! -f "$binary" ]]; then
 echo "::error::Expected plugin executable not found: $binary"
 find build/BASSBOXX_artefacts -maxdepth 6 -type f
 exit 1
fi
# Fail rather than accidentally shipping an Intel-only or Linux build.
lipo -verify_arch arm64 x86_64 "$binary"
codesign --force --sign - --timestamp=none "$plugin"
codesign --verify --deep --strict "$plugin"
mkdir -p dist
stage="$(mktemp -d "$PWD/dist/.package.XXXXXX")"
trap 'rm -rf "$stage"' EXIT
bundle="$stage/BASSBOXX-macOS"
mkdir -p "$bundle"
ditto "$plugin" "$bundle/BASSBOXX.vst3"
cp 'packaging/Install BASSBOXX.command' packaging/Installation.txt "$bundle/"
chmod +x "$bundle/Install BASSBOXX.command"
{
 echo 'BASSBOXX by ProHoeZak — macOS VST3 only'
 printf 'Source commit: %s\n' "$(git rev-parse HEAD)"
 printf 'Architectures: %s\n' "$(lipo -archs "$binary")"
 echo 'Minimum macOS: 11.0'
 echo 'JUCE: 8.0.6; ad-hoc signed, not Developer ID signed/notarized.'
 echo 'Offline tests must pass before CI publishes this archive. MPC host testing is separate.'
} > "$bundle/Build-info.txt"
# Include the dependency's required notices and an explicit source location.
printf '%s\n' "https://github.com/prohoezak-hue/bassboxx/tree/$(git rev-parse HEAD)" 'JUCE source: https://github.com/juce-framework/JUCE/tree/51a8a6d7aeae7326956d747737ccf1575e61e209' > "$bundle/Source.txt"
juce_source="${JUCE_SOURCE_DIR:-build/_deps/juce-src}"
if [[ ! -f "$juce_source/LICENSE.md" ]]; then
 echo "::error::JUCE license not found at $juce_source/LICENSE.md"
 find build -name LICENSE.md
 exit 1
fi
cp "$juce_source/LICENSE.md" "$bundle/JUCE-LICENSE.md"
# Exercise installation and upgrade from the exact directory being packaged.
BASSBOXX_PLUGIN_DIR="$stage/test-install" BASSBOXX_BACKUP_DIR="$stage/backups" bash "$bundle/Install BASSBOXX.command"
BASSBOXX_PLUGIN_DIR="$stage/test-install" BASSBOXX_BACKUP_DIR="$stage/backups" bash "$bundle/Install BASSBOXX.command"
codesign --verify --deep --strict "$stage/test-install/BASSBOXX.vst3"
archive="$PWD/dist/BASSBOXX-macOS-VST3.zip"
rm -f "$archive"
(cd "$stage" && ditto -c -k --sequesterRsrc --keepParent BASSBOXX-macOS "$archive")
# Confirm the archive itself preserves executable bits, signatures and both slices.
mkdir "$stage/extracted"
ditto -x -k dist/BASSBOXX-macOS-VST3.zip "$stage/extracted"
codesign --verify --deep --strict "$stage/extracted/BASSBOXX-macOS/BASSBOXX.vst3"
lipo -verify_arch arm64 x86_64 "$stage/extracted/BASSBOXX-macOS/BASSBOXX.vst3/Contents/MacOS/BASSBOXX"
[[ -x "$stage/extracted/BASSBOXX-macOS/Install BASSBOXX.command" ]]
(cd dist && shasum -a 256 BASSBOXX-macOS-VST3.zip > BASSBOXX-macOS-VST3.zip.sha256)
echo 'Packaged dist/BASSBOXX-macOS-VST3.zip'
