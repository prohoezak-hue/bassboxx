#!/bin/bash
# Development packages are unsigned. Customer packages require Developer ID + notarization.
set -Eeuo pipefail
cd "$(dirname "$0")/.."
[[ "$(uname -s)" == Darwin ]] || { echo 'Packaging requires macOS' >&2; exit 1; }
mode="${RELEASE_MODE:-development}"
[[ "$mode" == development || "$mode" == customer ]] || { echo 'Invalid RELEASE_MODE' >&2; exit 1; }
if [[ "$mode" == customer ]]; then
 : "${DEVELOPER_ID_APPLICATION:?Set the Developer ID Application signing identity}"
 : "${DEVELOPER_ID_INSTALLER:?Set the Developer ID Installer signing identity}"
 : "${NOTARY_PROFILE:?Set the notarytool keychain profile name}"
fi
plugin="${1:-build/BASSBOXX_artefacts/Release/VST3/The Funk Box.vst3}"
[[ -d "$plugin" ]] || { echo "Plugin not found: $plugin" >&2; exit 1; }
version="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$plugin/Contents/Info.plist")"
mkdir -p dist
stage="$(mktemp -d "$PWD/dist/.pkg.XXXXXX")"
trap 'rm -rf "$stage"' EXIT
root="$stage/root"
install_path='Library/Audio/Plug-Ins/VST3'
mkdir -p "$root/$install_path"
ditto "$plugin" "$root/$install_path/The Funk Box.vst3"
staged_plugin="$root/$install_path/The Funk Box.vst3"
lipo "$staged_plugin/Contents/MacOS/The Funk Box" -verify_arch arm64 x86_64
if [[ "$mode" == customer ]]; then
 codesign --force --options runtime --timestamp --sign "$DEVELOPER_ID_APPLICATION" "$staged_plugin"
else
 codesign --force --sign - --timestamp=none "$staged_plugin"
fi
codesign --verify --deep --strict "$staged_plugin"
# Disable bundle relocation so Installer uses the specified system VST3 folder.
# VST3 bundles may not be discovered by pkgbuild --analyze. Describe this
# component explicitly rather than assuming an inferred entry exists.
python3 - "$stage/components.plist" "$install_path/The Funk Box.vst3" <<'PYPLIST'
import plistlib, sys
component = {
    "RootRelativeBundlePath": sys.argv[2],
    "BundleIsRelocatable": False,
    "BundleHasStrictIdentifier": True,
    "BundleIsVersionChecked": True,
    "BundleOverwriteAction": "upgrade",
}
with open(sys.argv[1], "wb") as output:
    plistlib.dump([component], output)
PYPLIST
pkgbuild --root "$root" --component-plist "$stage/components.plist" \
 --identifier com.prohoezak.bassboxx.installer --version "$version" \
 --install-location / --ownership recommended "$stage/component.pkg"
mkdir -p "$stage/resources"
cp packaging/Installation.txt "$stage/resources/Installation.txt"
juce_source="${JUCE_SOURCE_DIR:-build/_deps/juce-src}"
[[ -f "$juce_source/LICENSE.md" ]] || { echo "Missing JUCE notices: $juce_source/LICENSE.md" >&2; exit 1; }
cp "$juce_source/LICENSE.md" "$stage/resources/JUCE-LICENSE.txt"
cat > "$stage/distribution.xml" <<'XML'
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
 <title>The Funk Box</title>
 <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>
 <domains enable_localSystem="true" enable_currentUserHome="false" enable_anywhere="false"/>
 <allowed-os-versions><os-version min="11.0"/></allowed-os-versions>
 <welcome file="Installation.txt" mime-type="text/plain"/>
 <license file="JUCE-LICENSE.txt" mime-type="text/plain"/>
 <choices-outline><line choice="plugin"/></choices-outline>
 <choice id="plugin" title="The Funk Box VST3" visible="false"><pkg-ref id="com.prohoezak.bassboxx.installer"/></choice>
 <pkg-ref id="com.prohoezak.bassboxx.installer">component.pkg</pkg-ref>
</installer-gui-script>
XML
if [[ "$mode" == customer ]]; then
 output="$PWD/dist/The Funk Box.pkg"
 productbuild --distribution "$stage/distribution.xml" --resources "$stage/resources" \
  --package-path "$stage" --sign "$DEVELOPER_ID_INSTALLER" --timestamp "$stage/product.pkg"
 pkgutil --check-signature "$stage/product.pkg"
 notary_args=(--keychain-profile "$NOTARY_PROFILE")
 if [[ -n "${NOTARY_KEYCHAIN:-}" ]]; then
  notary_args+=(--keychain "$NOTARY_KEYCHAIN")
 fi
 xcrun notarytool submit "$stage/product.pkg" "${notary_args[@]}" --wait
 xcrun stapler staple "$stage/product.pkg"
 xcrun stapler validate "$stage/product.pkg"
 spctl --assess --type install --verbose=2 "$stage/product.pkg"
else
 output="$PWD/dist/The Funk Box-development.pkg"
 productbuild --distribution "$stage/distribution.xml" --resources "$stage/resources" \
  --package-path "$stage" "$stage/product.pkg"
fi
# Expand the finished product and verify the exact payload before publishing it.
pkgutil --expand-full "$stage/product.pkg" "$stage/expanded"
packaged_plugin="$stage/expanded/component.pkg/Payload/$install_path/The Funk Box.vst3"
codesign --verify --deep --strict "$packaged_plugin"
lipo "$packaged_plugin/Contents/MacOS/The Funk Box" -verify_arch arm64 x86_64
[[ "$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$packaged_plugin/Contents/Info.plist")" == "$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$plugin/Contents/Info.plist")" ]]
[[ "$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$packaged_plugin/Contents/Info.plist")" == "$version" ]]
mv "$stage/product.pkg" "$output"
(cd dist && shasum -a 256 "$(basename "$output")" > "$(basename "$output").sha256")
printf 'Packaged %s (%s)\n' "$output" "$mode"
