#!/bin/bash
# Installs an intact VST3 without changing quarantine or macOS security policy.
set -Eeuo pipefail
trap 'status=$?; echo "Installation failed at line $LINENO (exit $status): $BASH_COMMAND"; exit "$status"' ERR
cd "$(dirname "$0")"
if [[ "$(uname -s)" != Darwin ]]; then
 echo 'BASSBOXX requires macOS. This installer cannot run on this operating system.'
 exit 1
fi
plugin="$PWD/BASSBOXX.vst3"
if [[ ! -d "$plugin/Contents/MacOS" ]]; then
 echo 'BASSBOXX.vst3 is missing. Extract the complete download ZIP before installing.'
 exit 1
fi
check_bundle() {
 /usr/bin/codesign --verify --deep --strict "$1" || return 1
 local attributes
 attributes=$(/usr/bin/xattr -lr "$1") || return 1
 if [[ "$attributes" == *com.apple.quarantine:* ]]; then
  echo 'This bundle has download quarantine information. Checking macOS assessment...'
  if ! /usr/sbin/spctl --assess --type execute --verbose=2 "$1"; then
   echo 'Installation stopped: macOS did not accept this quarantined plugin.'
   echo 'Your existing installation has not been replaced.'
   echo 'For a no-membership local development build, use Build BASSBOXX on this Mac.command.'
   echo 'Do not remove quarantine, disable Gatekeeper, or re-sign this downloaded binary to evade the check.'
   return 1
  fi
 fi
}
check_bundle "$plugin"
install_dir="${BASSBOXX_PLUGIN_DIR:-$HOME/Library/Audio/Plug-Ins/VST3}"
backup_dir="${BASSBOXX_BACKUP_DIR:-$HOME/Library/Application Support/BASSBOXX/Backups}"
mkdir -p "$install_dir" "$backup_dir"
stage="$(mktemp -d "$install_dir/.bassboxx-install.XXXXXX")"
trap 'rm -rf "$stage"' EXIT
/usr/bin/ditto "$plugin" "$stage/BASSBOXX.vst3"
check_bundle "$stage/BASSBOXX.vst3"
previous=''
if [[ -e "$install_dir/BASSBOXX.vst3" ]]; then
 previous="$backup_dir/BASSBOXX-$(date +%Y%m%d-%H%M%S)-$$.vst3"
 mv "$install_dir/BASSBOXX.vst3" "$previous"
fi
if ! mv "$stage/BASSBOXX.vst3" "$install_dir/BASSBOXX.vst3"; then
 if [[ -n "$previous" ]]; then mv "$previous" "$install_dir/BASSBOXX.vst3"; fi
 echo 'Installation failed; the previous plugin was restored.'
 exit 1
fi
if ! /usr/bin/codesign --verify --deep --strict "$install_dir/BASSBOXX.vst3"; then
 mv "$install_dir/BASSBOXX.vst3" "$stage/failed-install.vst3"
 if [[ -n "$previous" ]]; then mv "$previous" "$install_dir/BASSBOXX.vst3"; fi
 echo 'Installed signature verification failed; the previous plugin was restored if present.'
 exit 1
fi
printf '\nBASSBOXX files installed and signature verified.\n'
printf 'Install location: %s/BASSBOXX.vst3\n' "$install_dir"
echo 'Restart MPC desktop and rescan this VST3 folder. MPC 3.10 compatibility is not yet verified.'
