#!/bin/bash
# Exercise the real signed bundle and macOS tools; no mocked policy decisions.
set -Eeuo pipefail
bundle="$1"
root="$2"
name="${3:-TheFunkBoxx.vst3}"
mkdir -p "$root"
export BASSBOXX_PLUGIN_DIR="$root/installed"
export BASSBOXX_APP_DIR="$root/installed"
export BASSBOXX_AU_DIR="$root/installed"
export BASSBOXX_BACKUP_DIR="$root/backups"
installer="$bundle/Install TheFunkBoxx.command"
/bin/bash "$installer"
/bin/bash "$installer"
[[ "$(find "$BASSBOXX_BACKUP_DIR" -type d \( -name '*.vst3' -o -name '*.app' -o -name '*.component' \) -prune | wc -l | tr -d ' ')" == 1 ]]
before="$(shasum -a 256 "$BASSBOXX_PLUGIN_DIR/$name/Contents/MacOS/TheFunkBoxx")"
for location in root nested; do
 candidate="$root/quarantined-$location"
 mkdir "$candidate"
 ditto "$bundle/$name" "$candidate/$name"
 cp "$installer" "$candidate/Install TheFunkBoxx.command"
 target="$candidate/$name"
 if [[ "$location" == nested ]]; then target="$target/Contents/MacOS/TheFunkBoxx"; fi
 xattr -w com.apple.quarantine '0083;00000000;TheFunkBoxx-CI;' "$target"
 codesign --verify --deep --strict "$candidate/$name"
 if /bin/bash "$candidate/Install TheFunkBoxx.command" > "$root/quarantine-$location.log" 2>&1; then
  echo 'FAIL: quarantined, unnotarized bundle was not refused'; exit 1
 fi
 grep -F 'Installation stopped: macOS did not accept this quarantined bundle.' "$root/quarantine-$location.log"
 xattr -p com.apple.quarantine "$target" >/dev/null
 [[ "$before" == "$(shasum -a 256 "$BASSBOXX_PLUGIN_DIR/$name/Contents/MacOS/TheFunkBoxx")" ]]
 [[ "$(find "$BASSBOXX_BACKUP_DIR" -type d \( -name '*.vst3' -o -name '*.app' -o -name '*.component' \) -prune | wc -l | tr -d ' ')" == 1 ]]
done
mkdir "$root/tampered"
ditto "$bundle/$name" "$root/tampered/$name"
cp "$installer" "$root/tampered/Install TheFunkBoxx.command"
printf 'tamper' >> "$root/tampered/$name/Contents/MacOS/TheFunkBoxx"
if /bin/bash "$root/tampered/Install TheFunkBoxx.command" > "$root/tampered.log" 2>&1; then
 echo 'FAIL: invalid signature was accepted'; exit 1
fi
[[ "$before" == "$(shasum -a 256 "$BASSBOXX_PLUGIN_DIR/$name/Contents/MacOS/TheFunkBoxx")" ]]
echo 'PASS: first install, upgrade/backup, root and nested quarantine rejection, signature tampering rejection; previous installation preserved.'
