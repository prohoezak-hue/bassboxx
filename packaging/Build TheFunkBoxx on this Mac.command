#!/bin/bash
# Build the included, original source on this Mac using free Apple tools.
# Never changes quarantine, Gatekeeper, SIP, or trust settings.
set -Eeuo pipefail
cd "$(dirname "$0")"
bundle="$PWD"
[[ "$(uname -s)" == Darwin ]] || { echo 'This builder requires macOS.'; exit 1; }
if ! /usr/bin/xcrun --find clang >/dev/null 2>&1; then
 echo 'Apple Command Line Tools are needed once (free; no paid membership).'
 echo 'Install them in the Apple dialog, then run this builder again.'
 /usr/bin/xcode-select --install || true
 exit 1
fi
root="${BASSBOXX_BUILD_ROOT:-$HOME/Library/Application Support/TheFunkBoxx/Local Builds}"
mkdir -p "$root"
work="$(mktemp -d "$root/build.XXXXXX")"
exec > >(tee "$work/Build.log") 2>&1
log_pid=$!
mounted=0
cleanup() {
 local status=$?
 if [[ "$mounted" == 1 ]]; then /usr/bin/hdiutil detach "$work/cmake-volume" >/dev/null || true; fi
 # Wait for the logging process, so the caller receives the final failure too.
 exec 1>&- 2>&-
 wait "$log_pid" || true
 exit "$status"
}
trap cleanup EXIT
trap 'status=$?; echo "Build/install stopped at line $LINENO (exit $status). Log: $work/Build.log"; exit "$status"' ERR
echo 'TheFunkBoxx local development build — original interface, engine and 100 presets.'
echo "Build and log folder: $work"
echo 'Quit TheFunkBoxx and MPC before installation. Compilation can take several minutes.'
cd "$bundle"
/usr/bin/shasum -a 256 -c Payload-SHA256.txt
# Independently pinned, official upstream artifacts; keep TLS and checksums intact.
printf '%s  %s\n' dfb08721dccf5a2e0a368fd006c1a3ffb8c9e063229e24bc9d882585e4ef1068 LocalBuild/cmake-3.31.10-macos-universal.dmg | /usr/bin/shasum -a 256 -c -
printf '%s  %s\n' 20b740351cc030086951d87614bc08b42d072f1e4db68db1ef1d988e66dbbdbe LocalBuild/JUCE-8.0.6.tar.gz | /usr/bin/shasum -a 256 -c -
mkdir "$work/cmake-volume" "$work/source" "$work/juce"
/usr/bin/hdiutil attach -readonly -nobrowse -mountpoint "$work/cmake-volume" "$bundle/LocalBuild/cmake-3.31.10-macos-universal.dmg" >/dev/null
mounted=1
cmake_app="$work/cmake-volume/CMake.app"
# The bundled build tool must itself pass Apple's assessment before execution.
/usr/bin/codesign --verify --deep --strict "$cmake_app"
/usr/sbin/spctl --assess --type execute --verbose=2 "$cmake_app"
cmake="$cmake_app/Contents/bin/cmake"
ctest="$cmake_app/Contents/bin/ctest"
/usr/bin/tar -xzf "$bundle/LocalBuild/TheFunkBoxx-source.tar.gz" -C "$work/source"
/usr/bin/tar -xzf "$bundle/LocalBuild/JUCE-8.0.6.tar.gz" -C "$work/juce" --strip-components=1
"$cmake" -S "$work/source" -B "$work/build" -G 'Unix Makefiles' \
 -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' \
 -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 -DJUCE_SOURCE_DIR="$work/juce"
"$cmake" --build "$work/build" --config Release --parallel 2
# Check the new app before executing it, including in the test suite.
local_app="$work/build/TheFunkBoxx_artefacts/Release/Standalone/TheFunkBoxx.app"
if [[ -d "$local_app" ]]; then
 attributes=$(/usr/bin/xattr -lr "$local_app")
 if [[ "$attributes" == *com.apple.quarantine:* ]]; then
  echo 'New local app output is quarantined. Stopping without changing attributes.'
  printf '%s\n' "$attributes" | /usr/bin/awk '/com.apple.quarantine:/ { sub(/: com.apple.quarantine:.*/, ""); print "Quarantined file: " $0 }'
  exit 1
 fi
fi
"$ctest" --test-dir "$work/build" -C Release --output-on-failure --timeout 120
if [[ -d "$bundle/TheFunkBoxx.app" ]]; then
 app="$work/build/TheFunkBoxx_artefacts/Release/Standalone/TheFunkBoxx.app"
 /usr/bin/lipo "$app/Contents/MacOS/TheFunkBoxx" -verify_arch arm64 x86_64
 /usr/bin/codesign --force --sign - --timestamp=none "$app"
 /usr/bin/codesign --verify --deep --strict "$app"
 attributes=$(/usr/bin/xattr -lr "$app")
 if [[ "$attributes" == *com.apple.quarantine:* ]]; then
  echo 'New local app output is quarantined. Stopping without changing attributes.'
  exit 1
 fi
 mkdir "$work/install-source"
 /usr/bin/ditto "$app" "$work/install-source/TheFunkBoxx.app"
 cp "$bundle/Install TheFunkBoxx.command" "$work/install-source/Install TheFunkBoxx.command"
 /bin/bash "$work/install-source/Install TheFunkBoxx.command"
 installed="${BASSBOXX_APP_DIR:-$HOME/Applications}/TheFunkBoxx.app"
 "$cmake" "-DAPP=$installed" "-DREPORT=$work/installed-standalone.json" -P "$work/source/tests/StandaloneSmoke.cmake"
 echo 'PASS: standalone app locally compiled, tested and installed.'
 echo "Open $installed. MPC is not required."
 echo 'Use Options > Audio/MIDI Settings to choose your speakers/headphones and MIDI input.'
 echo "Build log: $work/Build.log"
 exit 0
fi
if [[ -d "$bundle/TheFunkBoxx.component" ]]; then
 plugin="$work/build/TheFunkBoxx_artefacts/Release/AU/TheFunkBoxx.component"
 name='TheFunkBoxx.component'
 installed="${BASSBOXX_AU_DIR:-$HOME/Library/Audio/Plug-Ins/Components}/$name"
else
 plugin="$work/build/TheFunkBoxx_artefacts/Release/VST3/TheFunkBoxx.vst3"
 name='TheFunkBoxx.vst3'
 installed="${BASSBOXX_PLUGIN_DIR:-$HOME/Library/Audio/Plug-Ins/VST3}/$name"
fi
/usr/bin/lipo "$plugin/Contents/MacOS/TheFunkBoxx" -verify_arch arm64 x86_64
# Sign only the newly compiled local output, never the downloaded precompiled VST3.
/usr/bin/codesign --force --sign - --timestamp=none "$plugin"
/usr/bin/codesign --verify --deep --strict "$plugin"
attributes=$(/usr/bin/xattr -lr "$plugin")
if [[ "$attributes" == *com.apple.quarantine:* ]]; then
 echo 'The newly built output is quarantined. Stopping without changing its attributes.'
 exit 1
fi
echo 'Local output was compiled here. An ad-hoc signature is not Developer ID or notarization.'
echo 'A distribution spctl assessment may still reject ad-hoc local code; no acceptance is claimed.'
echo 'Checking actual local VST3 loading, not changing macOS assessment policy.'
mkdir "$work/install-source"
/usr/bin/ditto "$plugin" "$work/install-source/$name"
cp "$bundle/Install TheFunkBoxx.command" "$work/install-source/Install TheFunkBoxx.command"
# Calling the same installer preserves backup/upgrade behavior and its checks.
/bin/bash "$work/install-source/Install TheFunkBoxx.command"
if [[ "$name" == TheFunkBoxx.component ]]; then
 "$cmake" "-DPLUGIN=$installed" "-DHOST_CHECK=$work/build/au_host_smoke" -P "$work/source/tests/ValidateAU.cmake"
 echo 'PASS: locally compiled, tested, installed and validated Audio Unit.'
 echo "Build log: $work/Build.log"
 echo 'Restart MPC, leave Use Audio Units checked, and choose Rescan All. MPC compatibility remains unverified.'
 exit 0
fi
# CMake provides a bounded process runner without a Python/Homebrew dependency.
cat > "$work/check-installed.cmake" <<'CMAKE'
execute_process(COMMAND "${HOST_CHECK}" "${PLUGIN}" TIMEOUT 30 RESULT_VARIABLE result)
if(NOT "${result}" STREQUAL "0")
 message(FATAL_ERROR "Installed VST3 load check failed or timed out: ${result}")
endif()
CMAKE
"$cmake" "-DHOST_CHECK=$work/build/vst3_host_smoke" "-DPLUGIN=$installed" -P "$work/check-installed.cmake"
echo 'PASS: locally compiled, tested, installed and loaded through the VST3 host smoke test.'
echo 'This is not a successful MPC 3.10 scan or an Apple notarization.'
echo "Keep this log if MPC reports a problem: $work/Build.log"
echo 'Next: restart MPC, rescan the user VST3 folder, and test TheFunkBoxx in a plugin track.'
