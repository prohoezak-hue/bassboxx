#!/bin/bash
# Exercise the real signed bundle and macOS tools; no mocked policy decisions.
set -Eeuo pipefail
bundle="$1"
root="$2"
name="${3:-BASSBOXX.vst3}"
mkdir -p "$root"
export BASSBOXX_PLUGIN_DIR="$root/installed"
export BASSBOXX_APP_DIR="$root/installed"
export BASSBOXX_BACKUP_DIR="$root/backups"
installer="$bundle/Install BASSBOXX.command"
/bin/bash "$installer"
/bin/bash "$installer"
[[ "$(find "$BASSBOXX_BACKUP_DIR" -type d \( -name '*.vst3' -o -name '*.app' \) -prune | wc -l | tr -d ' ')" == 1 ]]
before="$(shasum -a 256 "$BASSBOXX_PLUGIN_DIR/$name/Contents/MacOS/BASSBOXX")"
for location in root nested; do
 candidate="$root/quarantined-$location"
 mkdir "$candidate"
 ditto "$bundle/$name" "$candidate/$name"
 cp "$installer" "$candidate/Install BASSBOXX.command"
 target="$candidate/$name"
 if [[ "$location" == nested ]]; then target="$target/Contents/MacOS/BASSBOXX"; fi
 xattr -w com.apple.quarantine '0083;00000000;BASSBOXX-CI;' "$target"
 codesign --verify --deep --strict "$candidate/$name"
 if /bin/bash "$candidate/Install BASSBOXX.command" > "$root/quarantine-$location.log" 2>&1; then
  echo 'FAIL: quarantined, unnotarized bundle was not refused'; exit 1
 fi
 grep -F 'Installation stopped: macOS did not accept this quarantined bundle.' "$root/quarantine-$location.log"
 xattr -p com.apple.quarantine "$target" >/dev/null
 [[ "$before" == "$(shasum -a 256 "$BASSBOXX_PLUGIN_DIR/$name/Contents/MacOS/BASSBOXX")" ]]
 [[ "$(find "$BASSBOXX_BACKUP_DIR" -type d \( -name '*.vst3' -o -name '*.app' \) -prune | wc -l | tr -d ' ')" == 1 ]]
done
mkdir "$root/tampered"
ditto "$bundle/$name" "$root/tampered/$name"
cp "$installer" "$root/tampered/Install BASSBOXX.command"
printf 'tamper' >> "$root/tampered/$name/Contents/MacOS/BASSBOXX"
if /bin/bash "$root/tampered/Install BASSBOXX.command" > "$root/tampered.log" 2>&1; then
 echo 'FAIL: invalid signature was accepted'; exit 1
fi
[[ "$before" == "$(shasum -a 256 "$BASSBOXX_PLUGIN_DIR/$name/Contents/MacOS/BASSBOXX")" ]]
echo 'PASS: first install, upgrade/backup, root and nested quarantine rejection, signature tampering rejection; previous installation preserved.'
