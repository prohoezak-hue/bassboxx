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
lipo "$binary" -verify_arch arm64 x86_64
echo "::notice::Verified plugin architectures: $(lipo -archs "$binary")"
codesign --force --sign - --timestamp=none "$plugin"
codesign --verify --deep --strict "$plugin"
app="$PWD/build/BASSBOXX_artefacts/Release/Standalone/BASSBOXX.app"
lipo "$app/Contents/MacOS/BASSBOXX" -verify_arch arm64 x86_64
codesign --force --sign - --timestamp=none "$app"
codesign --verify --deep --strict "$app"
au="$PWD/build/BASSBOXX_artefacts/Release/AU/BASSBOXX.component"
lipo "$au/Contents/MacOS/BASSBOXX" -verify_arch arm64 x86_64
codesign --force --sign - --timestamp=none "$au"
codesign --verify --deep --strict "$au"
mkdir -p dist
stage="$(mktemp -d "$PWD/dist/.package.XXXXXX")"
trap 'rm -rf "$stage"' EXIT
bundle="$stage/BASSBOXX-macOS"
mkdir -p "$bundle"
ditto "$plugin" "$bundle/BASSBOXX.vst3"
cp 'packaging/Install BASSBOXX.command' packaging/Installation.txt "$bundle/"
chmod +x "$bundle/Install BASSBOXX.command"
{
 echo 'BASSBOXX by ProHoeZak — macOS AU, VST3 and standalone synthesizer'
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
# Include a tested no-membership local development workflow, not a trust bypass.
bash scripts/prepare-local-build-payload.sh "$bundle"
bash tests/InstallerTests.sh "$bundle" "$stage/installer-tests"
standalone="$stage/BASSBOXX-Standalone"
mkdir "$standalone"
ditto "$app" "$standalone/BASSBOXX.app"
cp 'packaging/Install BASSBOXX.command' "$standalone/"
chmod +x "$standalone/Install BASSBOXX.command"
cp packaging/Standalone-Installation.txt "$standalone/Installation.txt"
cp "$bundle/Build-info.txt" "$bundle/Source.txt" "$bundle/JUCE-LICENSE.md" "$standalone/"
bash scripts/prepare-local-build-payload.sh "$standalone"
bash tests/InstallerTests.sh "$standalone" "$stage/app-installer-tests" BASSBOXX.app
audio_unit="$stage/BASSBOXX-AU"
mkdir "$audio_unit"
ditto "$au" "$audio_unit/BASSBOXX.component"
cp 'packaging/Install BASSBOXX.command' "$audio_unit/"
chmod +x "$audio_unit/Install BASSBOXX.command"
cp packaging/AU-Installation.txt "$audio_unit/Installation.txt"
cp "$bundle/Build-info.txt" "$bundle/Source.txt" "$bundle/JUCE-LICENSE.md" "$audio_unit/"
bash scripts/prepare-local-build-payload.sh "$audio_unit"
bash tests/InstallerTests.sh "$audio_unit" "$stage/au-installer-tests" BASSBOXX.component
au_archive="$PWD/dist/BASSBOXX-macOS-AU.zip"
rm -f "$au_archive"
(cd "$stage" && ditto -c -k --sequesterRsrc --keepParent BASSBOXX-AU "$au_archive")
archive="$PWD/dist/BASSBOXX-macOS-VST3.zip"
rm -f "$archive"
(cd "$stage" && ditto -c -k --sequesterRsrc --keepParent BASSBOXX-macOS "$archive")
standalone_archive="$PWD/dist/BASSBOXX-macOS-Standalone.zip"
rm -f "$standalone_archive"
(cd "$stage" && ditto -c -k --sequesterRsrc --keepParent BASSBOXX-Standalone "$standalone_archive")
# Confirm the archive itself preserves executable bits, signatures and both slices.
mkdir "$stage/extracted"
ditto -x -k dist/BASSBOXX-macOS-VST3.zip "$stage/extracted"
ditto -x -k dist/BASSBOXX-macOS-Standalone.zip "$stage/extracted"
ditto -x -k dist/BASSBOXX-macOS-AU.zip "$stage/extracted"
extracted_au="$stage/extracted/BASSBOXX-AU"
codesign --verify --deep --strict "$extracted_au/BASSBOXX.component"
lipo "$extracted_au/BASSBOXX.component/Contents/MacOS/BASSBOXX" -verify_arch arm64 x86_64
[[ -x "$extracted_au/Install BASSBOXX.command" && -x "$extracted_au/Build BASSBOXX on this Mac.command" ]]
(cd "$extracted_au" && shasum -a 256 -c Payload-SHA256.txt)
# Register only this product in the ephemeral runner's user AU directory.
bash "$extracted_au/Install BASSBOXX.command"
python3 scripts/run-validator.py 360 build/validator-logs/au-validation.log \
 cmake "-DPLUGIN=$HOME/Library/Audio/Plug-Ins/Components/BASSBOXX.component" "-DHOST_CHECK=$PWD/build/au_host_smoke" -P tests/ValidateAU.cmake
extracted_app="$stage/extracted/BASSBOXX-Standalone"
codesign --verify --deep --strict "$extracted_app/BASSBOXX.app"
lipo "$extracted_app/BASSBOXX.app/Contents/MacOS/BASSBOXX" -verify_arch arm64 x86_64
(cd "$extracted_app" && shasum -a 256 -c Payload-SHA256.txt)
cmake "-DAPP=$extracted_app/BASSBOXX.app" "-DREPORT=$PWD/build/standalone-extracted-report.json" -P tests/StandaloneSmoke.cmake
codesign --verify --deep --strict "$stage/extracted/BASSBOXX-macOS/BASSBOXX.vst3"
lipo "$stage/extracted/BASSBOXX-macOS/BASSBOXX.vst3/Contents/MacOS/BASSBOXX" -verify_arch arm64 x86_64
[[ -x "$stage/extracted/BASSBOXX-macOS/Install BASSBOXX.command" ]]
[[ -x "$stage/extracted/BASSBOXX-macOS/Build BASSBOXX on this Mac.command" ]]
(cd "$stage/extracted/BASSBOXX-macOS" && shasum -a 256 -c Payload-SHA256.txt)
python3 scripts/run-validator.py 30 build/validator-logs/extracted-host-load.log \
 build/vst3_host_smoke "$stage/extracted/BASSBOXX-macOS/BASSBOXX.vst3"
# Model a downloaded payload, retain its quarantine, and build fresh local output.
# CI invokes its own reviewed helper directly; Finder launching on a user's Mac
# is a separate, unverified interaction and must respect local policy.
local_root="$(mktemp -d "$PWD/build/local-build-test-run.XXXXXX")"
xattr -r -w com.apple.quarantine '0083;00000000;BASSBOXX-CI;' "$extracted_app"
BASSBOXX_PLUGIN_DIR="$PWD/build/local-installed" \
 BASSBOXX_APP_DIR="$PWD/build/local-app-installed" \
 BASSBOXX_BACKUP_DIR="$PWD/build/local-backups" \
 BASSBOXX_BUILD_ROOT="$local_root" \
 /bin/bash "$extracted_app/Build BASSBOXX on this Mac.command"
xattr -p com.apple.quarantine "$extracted_app" >/dev/null
# Reuse this freshly compiled source build to test the VST3 installation too.
local_plugins=("$local_root"/build.*/build/BASSBOXX_artefacts/Release/VST3/BASSBOXX.vst3)
[[ ${#local_plugins[@]} == 1 && -d "${local_plugins[0]}" ]]
mkdir "$stage/local-vst3-source"
codesign --force --sign - --timestamp=none "${local_plugins[0]}"
ditto "${local_plugins[0]}" "$stage/local-vst3-source/BASSBOXX.vst3"
cp 'packaging/Install BASSBOXX.command' "$stage/local-vst3-source/"
BASSBOXX_PLUGIN_DIR="$PWD/build/local-installed" BASSBOXX_BACKUP_DIR="$PWD/build/local-backups" \
 bash "$stage/local-vst3-source/Install BASSBOXX.command"
python3 scripts/run-validator.py 30 build/validator-logs/local-installed-host.log \
 build/vst3_host_smoke "$PWD/build/local-installed/BASSBOXX.vst3"
# Also validate the AU compiled from the quarantined source payload above.
local_units=("$local_root"/build.*/build/BASSBOXX_artefacts/Release/AU/BASSBOXX.component)
local_hosts=("$local_root"/build.*/build/au_host_smoke)
[[ ${#local_units[@]} == 1 && -d "${local_units[0]}" && ${#local_hosts[@]} == 1 ]]
mkdir "$stage/local-au-source"
codesign --force --sign - --timestamp=none "${local_units[0]}"
ditto "${local_units[0]}" "$stage/local-au-source/BASSBOXX.component"
cp 'packaging/Install BASSBOXX.command' "$stage/local-au-source/"
bash "$stage/local-au-source/Install BASSBOXX.command"
python3 scripts/run-validator.py 360 build/validator-logs/local-au-validation.log \
 cmake "-DPLUGIN=$HOME/Library/Audio/Plug-Ins/Components/BASSBOXX.component" "-DHOST_CHECK=${local_hosts[0]}" -P tests/ValidateAU.cmake
(cd dist && shasum -a 256 BASSBOXX-macOS-AU.zip > BASSBOXX-macOS-AU.zip.sha256)
(cd dist && shasum -a 256 BASSBOXX-macOS-VST3.zip > BASSBOXX-macOS-VST3.zip.sha256)
(cd dist && shasum -a 256 BASSBOXX-macOS-Standalone.zip > BASSBOXX-macOS-Standalone.zip.sha256)
echo 'Packaged dist/BASSBOXX-macOS-VST3.zip'
echo 'Packaged dist/BASSBOXX-macOS-Standalone.zip'
