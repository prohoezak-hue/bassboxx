#!/bin/bash
# GitHub-hosted macOS runner only. Secret values must never be logged.
set -Eeuo pipefail
set +x
for name in MAC_APPLICATION_P12_BASE64 MAC_APPLICATION_P12_PASSWORD MAC_INSTALLER_P12_BASE64 MAC_INSTALLER_P12_PASSWORD APPLE_ID APPLE_APP_SPECIFIC_PASSWORD; do
 [[ -n "${!name:-}" ]] || { echo "::error::Missing repository secret: $name"; exit 1; }
done
signing_dir="$(mktemp -d "${RUNNER_TEMP:?}/funkbox-signing.XXXXXX")"
keychain="$signing_dir/signing.keychain-db"
cleanup() {
 security delete-keychain "$keychain" >/dev/null 2>&1 || true
 rm -rf "$signing_dir"
}
trap cleanup EXIT
umask 077
keychain_password="$(openssl rand -hex 32)"
security create-keychain -p "$keychain_password" "$keychain"
security set-keychain-settings -lut 21600 "$keychain"
security unlock-keychain -p "$keychain_password" "$keychain"
security list-keychains -d user -s "$keychain" "$HOME/Library/Keychains/login.keychain-db"
printf '%s' "$MAC_APPLICATION_P12_BASE64" | base64 --decode > "$signing_dir/application.p12"
printf '%s' "$MAC_INSTALLER_P12_BASE64" | base64 --decode > "$signing_dir/installer.p12"
security import "$signing_dir/application.p12" -k "$keychain" -P "$MAC_APPLICATION_P12_PASSWORD" -T /usr/bin/codesign >/dev/null
security import "$signing_dir/installer.p12" -k "$keychain" -P "$MAC_INSTALLER_P12_PASSWORD" -T /usr/bin/productbuild -T /usr/bin/productsign >/dev/null
security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "$keychain_password" "$keychain" >/dev/null
rm -f "$signing_dir/application.p12" "$signing_dir/installer.p12"
app_names="$(security find-identity -v -p codesigning "$keychain" | sed -n 's/.*"\(Developer ID Application:.*\)".*/\1/p')"
installer_names="$(security find-identity -v -p basic "$keychain" | sed -n 's/.*"\(Developer ID Installer:.*\)".*/\1/p')"
[[ -n "$app_names" && "$app_names" != *$'\n'* ]] || { echo '::error::Expected one valid Developer ID Application identity'; exit 1; }
[[ -n "$installer_names" && "$installer_names" != *$'\n'* ]] || { echo '::error::Expected one valid Developer ID Installer identity'; exit 1; }
team="$(printf '%s' "$app_names" | sed -n 's/.*(\([A-Z0-9]*\))$/\1/p')"
installer_team="$(printf '%s' "$installer_names" | sed -n 's/.*(\([A-Z0-9]*\))$/\1/p')"
[[ -n "$team" && "$team" == "$installer_team" ]] || { echo '::error::Signing certificates must belong to the same Apple team'; exit 1; }
export DEVELOPER_ID_APPLICATION="$app_names"
export DEVELOPER_ID_INSTALLER="$installer_names"
export NOTARY_PROFILE=funkbox-ci
export NOTARY_KEYCHAIN="$keychain"
xcrun notarytool store-credentials "$NOTARY_PROFILE" --keychain "$keychain" \
 --apple-id "$APPLE_ID" --team-id "$team" --password "$APPLE_APP_SPECIFIC_PASSWORD" >/dev/null
# Pass the temporary keychain explicitly to notarization.
RELEASE_MODE=customer bash scripts/package-pkg-macos.sh
