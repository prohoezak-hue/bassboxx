#!/bin/bash
# Invoked only by the manual distribution job after the existing validators pass.
# No security settings, quarantine, or global trust exceptions are changed.
set -euo pipefail
set +x
readonly team_id='5QF6Q5ATG5'
[[ $# == 2 ]] || { echo 'Usage: package-developer-id.sh validated-artifacts output-directory'; exit 2; }
for name in MAC_APPLICATION_P12_BASE64 MAC_APPLICATION_P12_PASSWORD MAC_INSTALLER_P12_BASE64 MAC_INSTALLER_P12_PASSWORD APPLE_ID APPLE_APP_SPECIFIC_PASSWORD GITHUB_SHA; do
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
/usr/bin/security create-keychain -p "$keychain_password" "$keychain"
/usr/bin/security set-keychain-settings -lut 21600 "$keychain"
/usr/bin/security unlock-keychain -p "$keychain_password" "$keychain"
for kind in APPLICATION INSTALLER; do
 encoded_name="MAC_${kind}_P12_BASE64"
 password_name="MAC_${kind}_P12_PASSWORD"
 printf '%s' "${!encoded_name}" | /usr/bin/base64 --decode > "$work/identity.p12"
 /usr/bin/security import "$work/identity.p12" -k "$keychain" -P "${!password_name}" -T /usr/bin/codesign -T /usr/bin/pkgbuild -T /usr/bin/productsign
 rm "$work/identity.p12"
done
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
for label,kind in [('application','Developer ID Application'),('installer','Developer ID Installer')]:
 matches=re.findall(r'\b([0-9A-Fa-f]{40})\s+"('+re.escape(kind)+r': [^"\n]+ \('+re.escape(team)+r'\))"',text)
 if len(matches)!=1: raise SystemExit(f'Expected exactly one valid {kind} identity with its private key for team {team}. Check the exported .p12 and certificate activation.')
 selected[label]={'hash':matches[0][0],'name':matches[0][1]}
json.dump(selected,open(sys.argv[2],'w'))
PY
application_identity="$(python3 -c 'import json,sys;print(json.load(open(sys.argv[1]))["application"]["hash"])' "$work/selected.json")"
installer_identity="$(python3 -c 'import json,sys;print(json.load(open(sys.argv[1]))["installer"]["name"])' "$work/selected.json")"
# Store credentials only inside the temporary keychain, which is deleted on exit.
xcrun notarytool store-credentials BASSBOXX-distribution --keychain "$keychain" --apple-id "$APPLE_ID" --team-id "$team_id" --password "$APPLE_APP_SPECIFIC_PASSWORD" >/dev/null
for format in AU VST3; do
 (cd "$artifacts" && /usr/bin/shasum -a 256 -c "BASSBOXX-macOS-$format.zip.sha256")
 /usr/bin/ditto -x -k "$artifacts/BASSBOXX-macOS-$format.zip" "$work/extracted"
done
au="$work/extracted/BASSBOXX-AU/BASSBOXX.component"
vst3="$work/extracted/BASSBOXX-macOS/BASSBOXX.vst3"
for folder in BASSBOXX-AU BASSBOXX-macOS; do
 /usr/bin/grep -Fx "Source commit: $GITHUB_SHA" "$work/extracted/$folder/Build-info.txt" >/dev/null
done
payload="$work/payload"
mkdir -p "$payload/Library/Audio/Plug-Ins/Components" "$payload/Library/Audio/Plug-Ins/VST3"
/usr/bin/ditto "$au" "$payload/Library/Audio/Plug-Ins/Components/BASSBOXX.component"
/usr/bin/ditto "$vst3" "$payload/Library/Audio/Plug-Ins/VST3/BASSBOXX.vst3"
for product in "$payload/Library/Audio/Plug-Ins/Components/BASSBOXX.component" "$payload/Library/Audio/Plug-Ins/VST3/BASSBOXX.vst3"; do
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
pkg="$output/BASSBOXX-macOS-Plugins.pkg"
/usr/bin/pkgbuild --root "$payload" --install-location / --identifier com.prohoezak.bassboxx.plugins --version "$version" --scripts packaging/developer-id-scripts --sign "$installer_identity" --keychain "$keychain" --timestamp "$pkg"
/usr/sbin/pkgutil --check-signature "$pkg" > "$output/package-signature.txt"
grep -F "($team_id)" "$output/package-signature.txt" >/dev/null
grep -F 'Developer ID Installer:' "$output/package-signature.txt" >/dev/null
submit_result=0
xcrun notarytool submit "$pkg" --keychain-profile BASSBOXX-distribution --keychain "$keychain" --wait --timeout 30m --output-format json > "$output/notarization.json" || submit_result=$?
submission_id="$(python3 -c 'import json,sys;print(json.load(open(sys.argv[1])).get("id",""))' "$output/notarization.json" 2>/dev/null || true)"
if [[ "$submission_id" =~ ^[0-9a-fA-F-]{36}$ ]]; then
 xcrun notarytool log "$submission_id" --keychain-profile BASSBOXX-distribution --keychain "$keychain" "$output/notarization-log.json" || true
fi
[[ "$submit_result" == 0 ]] || { echo 'Apple notarization submission failed. See notarization evidence.'; exit 1; }
python3 - "$output/notarization.json" <<'PY'
import json,sys
result=json.load(open(sys.argv[1]))
if result.get('status')!='Accepted': raise SystemExit('Apple did not accept this submission; no signed release will be published.')
PY
xcrun stapler staple -v "$pkg"
xcrun stapler validate -v "$pkg"
/usr/sbin/spctl --assess --type install --verbose=2 "$pkg"
(cd "$output" && /usr/bin/shasum -a 256 BASSBOXX-macOS-Plugins.pkg > BASSBOXX-macOS-Plugins.pkg.sha256)
python3 - "$output/Build-info.json" "$GITHUB_SHA" "$team_id" "$version" "$submission_id" <<'PY'
import json,sys
json.dump(dict(source_commit=sys.argv[2],team_id=sys.argv[3],version=sys.argv[4],notarization_status='Accepted',submission_id=sys.argv[5],products=['AU','VST3'],mpc_compatibility='Unverified'),open(sys.argv[1],'w'),indent=2)
PY
echo 'PASS: Developer ID signed plugins and installer, Apple notarization accepted, ticket stapled/validated, and Gatekeeper installer assessment passed.'
