#!/bin/bash
# Installs an intact macOS app or VST3 without changing quarantine or macOS security policy.
set -Eeuo pipefail
trap 'status=$?; echo "Installation failed at line $LINENO (exit $status): $BASH_COMMAND"; exit "$status"' ERR
cd "$(dirname "$0")"
if [[ "$(uname -s)" != Darwin ]]; then
 echo 'BASSBOXX requires macOS. This installer cannot run on this operating system.'
 exit 1
fi
name='BASSBOXX.vst3'
install_dir="${BASSBOXX_PLUGIN_DIR:-$HOME/Library/Audio/Plug-Ins/VST3}"
if [[ -d "$PWD/BASSBOXX.component" ]]; then
 name='BASSBOXX.component'
 install_dir="${BASSBOXX_AU_DIR:-$HOME/Library/Audio/Plug-Ins/Components}"
elif [[ -d "$PWD/BASSBOXX.app" ]]; then
 name='BASSBOXX.app'
 install_dir="${BASSBOXX_APP_DIR:-$HOME/Applications}"
fi
plugin="$PWD/$name"
if [[ ! -d "$plugin/Contents/MacOS" ]]; then
 echo 'BASSBOXX bundle is missing. Extract the complete download ZIP before installing.'
 exit 1
fi
check_bundle() {
 /usr/bin/codesign --verify --deep --strict "$1" || return 1
 local attributes
 attributes=$(/usr/bin/xattr -lr "$1") || return 1
 if [[ "$attributes" == *com.apple.quarantine:* ]]; then
  echo 'This bundle has download quarantine information. Checking macOS assessment...'
  if ! /usr/sbin/spctl --assess --type execute --verbose=2 "$1"; then
   echo 'Installation stopped: macOS did not accept this quarantined bundle.'
   echo 'Your existing installation has not been replaced.'
   echo 'For a no-membership local development build, use Build BASSBOXX on this Mac.command.'
   echo 'Do not remove quarantine, disable Gatekeeper, or re-sign this downloaded binary to evade the check.'
   return 1
  fi
 fi
}
check_bundle "$plugin"
backup_dir="${BASSBOXX_BACKUP_DIR:-$HOME/Library/Application Support/BASSBOXX/Backups}"
mkdir -p "$install_dir" "$backup_dir"
stage="$(mktemp -d "$install_dir/.bassboxx-install.XXXXXX")"
trap 'rm -rf "$stage"' EXIT
/usr/bin/ditto "$plugin" "$stage/$name"
check_bundle "$stage/$name"
previous=''
if [[ -e "$install_dir/$name" ]]; then
 previous="$backup_dir/BASSBOXX-$(date +%Y%m%d-%H%M%S)-$$.${name##*.}"
 mv "$install_dir/$name" "$previous"
fi
if ! mv "$stage/$name" "$install_dir/$name"; then
 if [[ -n "$previous" ]]; then mv "$previous" "$install_dir/$name"; fi
 echo 'Installation failed; the previous plugin was restored.'
 exit 1
fi
if ! /usr/bin/codesign --verify --deep --strict "$install_dir/$name"; then
 mv "$install_dir/$name" "$stage/failed-install.${name##*.}"
 if [[ -n "$previous" ]]; then mv "$previous" "$install_dir/$name"; fi
 echo 'Installed signature verification failed; the previous plugin was restored if present.'
 exit 1
fi
printf '\nBASSBOXX files installed and signature verified.\n'
printf 'Install location: %s/%s\n' "$install_dir" "$name"
if [[ "$name" == BASSBOXX.app ]]; then
 echo 'Open BASSBOXX.app from your Applications folder. Choose Options > Audio/MIDI Settings for your output and controller.'
elif [[ "$name" == BASSBOXX.component ]]; then
 echo 'Restart MPC, keep Use Audio Units checked, and choose Rescan All.'
 echo 'Look for BASSBOXX as an AU instrument. MPC loading still needs local verification.'
else
 echo 'Restart MPC desktop and rescan this VST3 folder. MPC 3.10 compatibility is not yet verified.'
fi
