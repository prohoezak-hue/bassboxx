#!/bin/bash
# Installs a precompiled VST3. No JUCE, Xcode, admin access or compilation needed.
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
/usr/bin/codesign --verify --deep --strict "$plugin"
install_dir="${BASSBOXX_PLUGIN_DIR:-$HOME/Library/Audio/Plug-Ins/VST3}"
backup_dir="${BASSBOXX_BACKUP_DIR:-$HOME/Library/Application Support/BASSBOXX/Backups}"
mkdir -p "$install_dir" "$backup_dir"
stage="$(mktemp -d "$install_dir/.bassboxx-install.XXXXXX")"
trap 'rm -rf "$stage"' EXIT
/usr/bin/ditto "$plugin" "$stage/BASSBOXX.vst3"
/usr/bin/codesign --verify --deep --strict "$stage/BASSBOXX.vst3"
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
printf '\nBASSBOXX installed successfully.\nRestart MPC desktop, rescan VST3 plugins and select BASSBOXX on a plugin track.\n'
