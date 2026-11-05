#!/bin/bash
# Repackage the approved release without recompiling or changing its bundles.
set -euo pipefail
set +x
export FUNKBOXX_PACKAGE_INPUT="$PWD/approved-input"
present=0
for name in MAC_INSTALLER_P12_BASE64 MAC_INSTALLER_P12_PASSWORD APPLE_ID APPLE_APP_SPECIFIC_PASSWORD; do
  if [ -n "${!name:-}" ]; then present=$((present + 1)); fi
done
if [ "$present" -eq 0 ]; then
  echo '::warning::Installer signing secrets are unavailable. Building a clearly labeled unsigned package for testing only.'
  exec bash scripts/package-approved-dmg.sh
fi
if [ "$present" -ne 4 ]; then
  for name in MAC_INSTALLER_P12_BASE64 MAC_INSTALLER_P12_PASSWORD APPLE_ID APPLE_APP_SPECIFIC_PASSWORD; do
    [ -n "${!name:-}" ] || echo "::error::Missing repository secret: $name"
  done
  exit 1
fi
work=$(mktemp -d "$RUNNER_TEMP/approved-pkg-signing.XXXXXX")
keychain="$work/signing.keychain-db"
cleanup() {
  result=$?
  trap - EXIT
  security delete-keychain "$keychain" >/dev/null 2>&1 || true
  rm -rf "$work"
  exit "$result"
}
trap cleanup EXIT
umask 077
password=$(openssl rand -hex 32)
echo "::add-mask::$password"
security create-keychain -p "$password" "$keychain"
security unlock-keychain -p "$password" "$keychain"
security set-keychain-settings -lut 7200 "$keychain"
security list-keychains -d user -s "$keychain" "$HOME/Library/Keychains/login.keychain-db"
printf '%s' "$MAC_INSTALLER_P12_BASE64" | base64 --decode > "$work/installer.p12"
security import "$work/installer.p12" -k "$keychain" -P "$MAC_INSTALLER_P12_PASSWORD" -T /usr/bin/productsign >/dev/null
rm "$work/installer.p12"
curl --fail --location --proto '=https' --proto-redir '=https' https://www.apple.com/certificateauthority/DeveloperIDG2CA.cer -o "$work/intermediate.cer"
security verify-cert -c "$work/intermediate.cer"
security import "$work/intermediate.cer" -k "$keychain"
security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "$password" "$keychain" >/dev/null
identity=$(security find-identity -v -p basic "$keychain" | sed -n 's/.*"\(Developer ID Installer:.*\)".*/\1/p')
[ -n "$identity" ] && [[ "$identity" != *$'\n'* ]] || { echo '::error::Expected exactly one valid Developer ID Installer identity'; exit 1; }
[[ "$identity" == *'(5QF6Q5ATG5)' ]] || { echo '::error::Installer identity must match the approved application team'; exit 1; }
export FUNKBOXX_INSTALLER_IDENTITY="$identity"
export FUNKBOXX_NOTARY_PROFILE=approved-funkboxx-pkg
xcrun notarytool store-credentials "$FUNKBOXX_NOTARY_PROFILE" --apple-id "$APPLE_ID" --team-id 5QF6Q5ATG5 --password "$APPLE_APP_SPECIFIC_PASSWORD" >/dev/null
bash scripts/package-approved-dmg.sh
