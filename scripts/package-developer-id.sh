#!/bin/bash
# Invoked only by the authorized distribution job after the existing validators pass.
# No security settings, quarantine, or global trust exceptions are changed.
set -euo pipefail
set +x
stage="Check signing configuration"
readonly team_id='5QF6Q5ATG5'
[[ $# == 2 ]] || { echo 'Usage: package-developer-id.sh validated-artifacts output-directory'; exit 2; }
for name in MAC_APPLICATION_P12_BASE64 MAC_APPLICATION_P12_PASSWORD APPLE_ID APPLE_APP_SPECIFIC_PASSWORD GITHUB_SHA; do
 if [[ -z "${!name:-}" ]]; then echo "Required GitHub Actions secret/context missing: $name"; exit 2; fi
done
[[ "$(uname -s)" == Darwin ]] || { echo 'Developer ID distribution requires macOS.'; exit 2; }
[[ "$GITHUB_SHA" =~ ^[0-9a-f]{40}$ ]] || { echo 'Invalid source commit.'; exit 2; }
artifacts="$(cd "$1" && pwd)"
mkdir -p "$2"
output="$(cd "$2" && pwd)"
work="$(mktemp -d "${RUNNER_TEMP:-${TMPDIR:-/tmp}}/bassboxx-signing.XXXXXX")"
chmod 700 "$work"
keychain="$work/distribution.keychain-db"
keychain_password="$(/usr/bin/openssl rand -hex 32)"
echo "::add-mask::$keychain_password"
old_keychains=()
while IFS= read -r path; do
 [[ -z "$path" ]] || old_keychains+=("$path")
done < <(/usr/bin/security list-keychains -d user | /usr/bin/sed -E 's/^[[:space:]]*"(.*)"$/\1/')
cleanup() {
 local status=$?
 trap - EXIT
 if [[ ${#old_keychains[@]} -gt 0 ]]; then /usr/bin/security list-keychains -d user -s "${old_keychains[@]}" >/dev/null 2>&1 || true; fi
 /usr/bin/security delete-keychain "$keychain" >/dev/null 2>&1 || true
 rm -rf "$work"
 exit "$status"
}
trap cleanup EXIT
on_error() {
 local status=$?
 echo "::error::BASSBOXX distribution failed during: $stage (exit $status). No signed release was published."
}
trap on_error ERR
set -E
/usr/bin/security create-keychain -p "$keychain_password" "$keychain"
/usr/bin/security set-keychain-settings -lut 21600 "$keychain"
/usr/bin/security unlock-keychain -p "$keychain_password" "$keychain"
stage="Decode and import Application signing identity"
python3 - "$work/identity.p12" <<'PY_DECODE'
import base64,binascii,os,sys
value=''.join(os.environ['MAC_APPLICATION_P12_BASE64'].split())
try:
 data=base64.b64decode(value,validate=True)
 if not data or data[0]!=0x30: raise ValueError('not DER')
except (ValueError,binascii.Error):
 raise SystemExit('MAC_APPLICATION_P12_BASE64 is not an encoded PKCS12 file. Secret contents were not logged.')
with open(sys.argv[1],'wb') as f: f.write(data)
os.chmod(sys.argv[1],0o600)
PY_DECODE
/usr/bin/security import "$work/identity.p12" -k "$keychain" -P "$MAC_APPLICATION_P12_PASSWORD" -T /usr/bin/codesign
rm "$work/identity.p12"
# The user's identity became valid after adding Apple's G2 intermediate. Import
# that public intermediate here too, after verifying it against system trust.
# This does not add a trust override or import a new root certificate.
/usr/bin/curl --fail --location --proto '=https' --proto-redir '=https' https://www.apple.com/certificateauthority/DeveloperIDG2CA.cer -o "$work/DeveloperIDG2CA.cer"
/usr/bin/security verify-cert -c "$work/DeveloperIDG2CA.cer"
/usr/bin/security import "$work/DeveloperIDG2CA.cer" -k "$keychain"
/usr/bin/security list-keychains -d user -s "$keychain" "${old_keychains[@]}"
/usr/bin/security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "$keychain_password" "$keychain" >/dev/null
/usr/bin/security find-identity -v "$keychain" > "$work/identities.txt"
python3 - "$work/identities.txt" "$work/selected.json" "$team_id" <<'PY'
import json,re,sys
text=open(sys.argv[1]).read();team=sys.argv[3];selected={}
for label,kind in [('application','Developer ID Application')]:
 matches=re.findall(r'\b([0-9A-Fa-f]{40})\s+"('+re.escape(kind)+r': [^"\n]+ \('+re.escape(team)+r'\))"',text)
 if len(matches)!=1: raise SystemExit(f'Expected exactly one valid {kind} identity with its private key for team {team}. Check the exported .p12 and certificate activation.')
 selected[label]={'hash':matches[0][0],'name':matches[0][1]}
json.dump(selected,open(sys.argv[2],'w'))
PY
application_identity="$(python3 -c 'import json,sys;print(json.load(open(sys.argv[1]))["application"]["hash"])' "$work/selected.json")"
# Store credentials only inside the temporary keychain, which is deleted on exit.
stage="Authenticate Apple notarization account"
if ! xcrun notarytool store-credentials BASSBOXX-distribution --keychain "$keychain" --apple-id "$APPLE_ID" --team-id "$team_id" --password "$APPLE_APP_SPECIFIC_PASSWORD" > "$work/authentication.log" 2>&1; then
 echo '::error::Apple notarization authentication failed. Check APPLE_ID and APPLE_APP_SPECIFIC_PASSWORD in GitHub; no certificate files need changing.'
 exit 1
fi
stage="Verify validated build artifacts"
validated_source="${BASSBOXX_VALIDATED_SOURCE_SHA:-$GITHUB_SHA}"
[[ "$validated_source" =~ ^[0-9a-f]{40}$ ]] || { echo 'Invalid validated instrument source commit'; exit 1; }
for archive in BASSBOXX-macOS-AU.zip BASSBOXX-macOS-VST3.zip BASSBOXX-native-installer-build.zip; do
 (cd "$artifacts" && /usr/bin/shasum -a 256 -c "$archive.sha256")
 /usr/bin/ditto -x -k "$artifacts/$archive" "$work/extracted"
done
au="$work/extracted/BASSBOXX-AU/BASSBOXX.component"
vst3="$work/extracted/BASSBOXX-macOS/BASSBOXX.vst3"
for folder in BASSBOXX-AU BASSBOXX-macOS; do
 /usr/bin/grep -Fx "Source commit: $validated_source" "$work/extracted/$folder/Build-info.txt" >/dev/null
done
app="$work/extracted/Install BASSBOXX.app"
/usr/bin/codesign --verify --deep --strict "$app"
[[ "$(/usr/libexec/PlistBuddy -c 'Print :BASSBOXXSourceCommit' "$app/Contents/Info.plist")" == "$GITHUB_SHA" ]] || { echo 'Installer source commit mismatch'; exit 1; }
plugins="$app/Contents/Resources/Plugins"
mkdir -p "$plugins"
/usr/bin/ditto "$au" "$plugins/BASSBOXX.component"
/usr/bin/ditto "$vst3" "$plugins/BASSBOXX.vst3"
stage="Developer ID signing of plugins"
for product in "$plugins/BASSBOXX.component" "$plugins/BASSBOXX.vst3"; do
 /usr/bin/codesign --verify --deep --strict "$product"
 /usr/bin/lipo "$product/Contents/MacOS/BASSBOXX" -verify_arch arm64 x86_64
 /usr/bin/codesign --force --sign "$application_identity" --keychain "$keychain" --options runtime --timestamp "$product"
 /usr/bin/codesign --verify --deep --strict "$product"
 metadata="$(/usr/bin/codesign -dv --verbose=4 "$product" 2>&1)"
 printf '%s\n' "$metadata" | /usr/bin/grep -Fx "TeamIdentifier=$team_id" >/dev/null
 printf '%s\n' "$metadata" | /usr/bin/grep -F 'Authority=Developer ID Application:' >/dev/null
 printf '%s\n' "$metadata" | /usr/bin/grep -F 'Timestamp=' >/dev/null
 printf '%s\n' "$metadata" | /usr/bin/grep -F '(runtime)' >/dev/null
done
version="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$au/Contents/Info.plist")"
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo 'Invalid product version'; exit 1; }
stage="Test installation of actual signed products"
xcrun swiftc -swift-version 5 -D INSTALLER_TEST packaging/native-installer/InstallerCore.swift tests/NativeInstallerTests.swift -o "$work/signed-installer-tests"
python3 scripts/run-validator.py 90 "$output/installer-integration.log" "$work/signed-installer-tests" --signed-products "$plugins"
stage="Developer ID signing of native installer app"
/usr/bin/codesign --force --sign "$application_identity" --keychain "$keychain" --options runtime --timestamp "$app"
/usr/bin/codesign --verify --deep --strict "$app"
/usr/bin/lipo "$app/Contents/MacOS/InstallBASSBOXX" -verify_arch arm64 x86_64
notarize() {
 local product="$1" label="$2" result=0 submission_id
 stage="Apple notarization: $label"
 xcrun notarytool submit "$product" --keychain-profile BASSBOXX-distribution --keychain "$keychain" --wait --timeout 30m --output-format json > "$output/notarization-$label.json" || result=$?
 submission_id="$(python3 -c 'import json,sys;print(json.load(open(sys.argv[1])).get("id",""))' "$output/notarization-$label.json" 2>/dev/null || true)"
 if [[ "$submission_id" =~ ^[0-9a-fA-F-]{36}$ ]]; then
  xcrun notarytool log "$submission_id" --keychain-profile BASSBOXX-distribution --keychain "$keychain" "$output/notarization-$label-log.json" || true
 fi
 [[ "$result" == 0 ]] || { echo "::error::Apple notarization for $label did not complete successfully; see notarization evidence."; return 1; }
 python3 - "$output/notarization-$label.json" <<'PY_ACCEPT'
import json,re,sys
result=json.load(open(sys.argv[1]))
if result.get('status')!='Accepted' or not re.fullmatch(r'[0-9a-fA-F-]{36}',result.get('id','')):
 raise SystemExit('Apple has not accepted this submission. No signed release will be published.')
print('Apple notarization accepted.')
PY_ACCEPT
}
/usr/bin/ditto -c -k --sequesterRsrc --keepParent "$app" "$work/installer-app.zip"
notarize "$work/installer-app.zip" app
stage="Staple and assess native installer app"
xcrun stapler staple -v "$app"
xcrun stapler validate -v "$app"
/usr/sbin/spctl --assess --type execute --verbose=2 "$app"
mkdir "$work/dmg-root"
/usr/bin/ditto "$app" "$work/dmg-root/Install BASSBOXX.app"
cp "$work/extracted/BASSBOXX-AU/JUCE-LICENSE.md" "$work/dmg-root/"
cat > "$work/dmg-root/Read Me.txt" <<EOF_README
BASSBOXX by ProHoeZak — notarized plugin installer
Open Install BASSBOXX.app and click Install. No certificate setup or local build
is required. It installs AU/VST3 into your own Library/Audio/Plug-Ins folders,
backs up previous copies, and leaves the working standalone app unchanged.
Restart MPC, keep Use Audio Units enabled, and choose Rescan All.
MPC compatibility still requires a successful scan, playback and project recall.
Source: https://github.com/prohoezak-hue/bassboxx/tree/$GITHUB_SHA
EOF_README
stage="Create and sign disk image"
dmg="$output/BASSBOXX-macOS-Installer.dmg"
/usr/bin/hdiutil create -volname 'Install BASSBOXX' -srcfolder "$work/dmg-root" -fs HFS+ -format UDZO "$dmg"
/usr/bin/codesign --sign "$application_identity" --keychain "$keychain" --timestamp "$dmg"
/usr/bin/codesign --verify --strict "$dmg"
notarize "$dmg" dmg
stage="Staple and assess disk image"
xcrun stapler staple -v "$dmg"
xcrun stapler validate -v "$dmg"
/usr/sbin/spctl --assess --type open --context context:primary-signature --verbose=2 "$dmg"
(cd "$output" && /usr/bin/shasum -a 256 BASSBOXX-macOS-Installer.dmg > BASSBOXX-macOS-Installer.dmg.sha256)
python3 - "$output/Build-info.json" "$GITHUB_SHA" "$team_id" "$version" "$output/notarization-app.json" "$output/notarization-dmg.json" "$validated_source" <<'PY_MANIFEST'
import json,sys
json.dump(dict(source_commit=sys.argv[2],instrument_source_commit=sys.argv[7],team_id=sys.argv[3],version=sys.argv[4],notarization_status='Accepted',app_submission=json.load(open(sys.argv[5]))['id'],dmg_submission=json.load(open(sys.argv[6]))['id'],products=['AU','VST3'],distribution='native-installer-app-dmg',mpc_compatibility='Unverified'),open(sys.argv[1],'w'),indent=2)
PY_MANIFEST
stage="Complete"
echo 'PASS: native installer app and plugins signed, app and DMG accepted by Apple, tickets stapled/validated, Gatekeeper assessments passed.'
