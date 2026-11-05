#!/bin/bash
set -euo pipefail
cd "${FUNKBOXX_PACKAGE_INPUT:?Set approved DMG staging directory}"
kit_dir="$PWD"
finish() {
  result=${1:-$?}
  if [ "$result" -ne 0 ]; then
    echo "Build stopped. Your installed instrument and the original DMG were not changed."
  fi
  echo "Press Return to close."
  if [ -t 0 ]; then read -r _ || true; fi
  exit "$result"
}
trap finish EXIT
if [ "$(uname -s)" != Darwin ]; then
  echo "This packaging script requires macOS."
  exit 1
fi
for tool in hdiutil pkgbuild productbuild pkgutil ditto codesign shasum; do
  command -v "$tool" >/dev/null || { echo "Missing macOS tool: $tool"; exit 1; }
done
dmg="$kit_dir/Approved-TheFunkBoxx-1.3.1.dmg"
expected=b0dd509f62d8f7657f99cb4fde822b8941da4582b4394b46b234114fc3389cbc
actual=$(shasum -a 256 "$dmg" | awk '{print $1}')
[ "$actual" = "$expected" ] || { echo "Approved DMG checksum mismatch."; exit 1; }
work=$(mktemp -d "${TMPDIR:-/tmp}/thefunkboxx-pkg.XXXXXX")
mount="$work/mounted"
mkdir -p "$mount" "$work/packages"
cleanup_mount() { hdiutil detach "$mount" -quiet >/dev/null 2>&1 || true; }
trap 'result=$?; cleanup_mount; finish "$result"' EXIT
hdiutil attach "$dmg" -readonly -nobrowse -mountpoint "$mount"
plugin_dir="$mount/Install TheFunkBoxx.app/Contents/Resources/Plugins"
app="$mount/TheFunkBoxx.app"
for bundle in "$app" "$plugin_dir/TheFunkBoxx.component" "$plugin_dir/TheFunkBoxx.vst3"; do
  [ -d "$bundle" ] || { echo "Missing approved bundle: $bundle"; exit 1; }
  version=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$bundle/Contents/Info.plist")
  [ "$version" = 1.3.1 ] || { echo "Unexpected bundle version: $version"; exit 1; }
  codesign --verify --deep --strict --verbose=2 "$bundle"
done
# Explicit roots prevent Installer from relocating bundles to an existing copy.
# AU, VST3 and standalone intentionally share a bundle identifier.
package_bundle() {
  local source="$1" destination="$2" kind="$3"
  local root="$work/$kind-root" relative="$destination/$(basename "$source")"
  mkdir -p "$root/$destination"
  ditto "$source" "$root/$relative"
  /usr/bin/python3 - "$work/$kind-components.plist" "$relative" <<'PYPLIST'
import plistlib, sys
with open(sys.argv[1], 'wb') as f:
    plistlib.dump([{
        'RootRelativeBundlePath': sys.argv[2],
        'BundleIsRelocatable': False,
        'BundleHasStrictIdentifier': True,
        'BundleIsVersionChecked': False,
        'BundleOverwriteAction': 'upgrade',
    }], f)
PYPLIST
  pkgbuild --root "$root" --component-plist "$work/$kind-components.plist" \
    --install-location / --ownership recommended \
    --identifier "com.prohoezak.thefunkboxx.pkg.$kind" --version 1.3.1.1 "$work/packages/$kind.pkg"
}
package_bundle "$app" Applications app
package_bundle "$plugin_dir/TheFunkBoxx.component" Library/Audio/Plug-Ins/Components au
package_bundle "$plugin_dir/TheFunkBoxx.vst3" Library/Audio/Plug-Ins/VST3 vst3
mkdir -p "$work/preset-root/Library/Application Support/TheFunkBoxx/Presets"
ditto "$mount/ProHoeZak Presets" "$work/preset-root/Library/Application Support/TheFunkBoxx/Presets"
pkgbuild --root "$work/preset-root" --install-location / --identifier com.prohoezak.thefunkboxx.pkg.presets --version 1.3.1 --ownership recommended "$work/packages/presets.pkg"
cleanup_mount
out="$kit_dir/Output-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$out"
unsigned="$out/TheFunkBoxx-1.3.1-Installer2-UNSIGNED.pkg"
productbuild --package "$work/packages/app.pkg" --package "$work/packages/au.pkg" --package "$work/packages/vst3.pkg" --package "$work/packages/presets.pkg" "$unsigned"
pkgutil --expand-full "$unsigned" "$work/expanded-product"
for kind in app au vst3; do
  info="$work/expanded-product/$kind.pkg/PackageInfo"
  cp "$info" "$out/$kind-PackageInfo.xml"
  /usr/bin/python3 - "$info" <<'PYVERIFY'
import sys, xml.etree.ElementTree as ET
root = ET.parse(sys.argv[1]).getroot()
assert root.attrib['install-location'] == '/', 'Unexpected package install location'
# pkgbuild can emit <relocate/> even when every bundle is non-relocatable.
# Reject populated relocation sections, rather than the empty container.
for section in root.findall('.//relocate'):
    assert not list(section) and not section.attrib and not (section.text or '').strip(), 'Package contains active bundle relocation rules'
PYVERIFY
 done
codesign --verify --deep --strict "$work/expanded-product/au.pkg/Payload/Library/Audio/Plug-Ins/Components/TheFunkBoxx.component"
pkgutil --payload-files "$work/packages/au.pkg" > "$out/AU-payload.txt"
pkgutil --payload-files "$work/packages/vst3.pkg" > "$out/VST3-payload.txt"
pkgutil --payload-files "$work/packages/app.pkg" > "$out/Standalone-payload.txt"
final="$unsigned"
if [ -n "${FUNKBOXX_INSTALLER_IDENTITY:-}" ]; then
  final="$out/TheFunkBoxx-1.3.1-Installer2-SIGNED.pkg"
  productsign --sign "$FUNKBOXX_INSTALLER_IDENTITY" "$unsigned" "$final"
  pkgutil --check-signature "$final"
  if [ -n "${FUNKBOXX_NOTARY_PROFILE:-}" ]; then
    xcrun notarytool submit "$final" --keychain-profile "$FUNKBOXX_NOTARY_PROFILE" --wait
    xcrun stapler staple "$final"
    xcrun stapler validate "$final"
    spctl --assess --type install --verbose=2 "$final"
  else
    echo "Installer signed; notarization was not performed."
  fi
else
  echo "Created an UNSIGNED installer for local testing. It has not been notarized."
  echo "Public distribution requires Developer ID Installer signing and notarization."
fi
shasum -a 256 "$final" > "$final.sha256"
echo "Created: $final"
echo "Original source reference: 892e088c0beb47ae884596aa31b4724f4b279270"
echo "No instrument has been installed or replaced. Installation and host testing remain."
if [ "${CI:-}" != true ]; then open -R "$final"; fi
