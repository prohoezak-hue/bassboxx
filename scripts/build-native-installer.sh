#!/bin/bash
set -euo pipefail
[[ "$(uname -s)" == Darwin ]] || { echo 'Native installer build requires macOS'; exit 2; }
cd "$(dirname "$0")/.."
build="${1:-build/native-installer}"
mkdir -p "$build"
build="$(cd "$build" && pwd)"
core=packaging/native-installer/InstallerCore.swift
xcrun swiftc -swift-version 5 -D INSTALLER_TEST "$core" tests/NativeInstallerTests.swift -o "$build/installer-tests"
"$build/installer-tests"
app="$build/Install TheFunkBoxx.app"
mkdir -p "$app/Contents/MacOS" "$app/Contents/Resources"
sdk="$(xcrun --sdk macosx --show-sdk-path)"
for arch in arm64 x86_64; do
 xcrun swiftc -swift-version 5 -O -sdk "$sdk" -target "$arch-apple-macosx11.0" "$core" packaging/native-installer/InstallerApp.swift -o "$build/InstallTheFunkBoxx-$arch"
done
/usr/bin/lipo -create "$build/InstallTheFunkBoxx-arm64" "$build/InstallTheFunkBoxx-x86_64" -output "$app/Contents/MacOS/InstallTheFunkBoxx"
python3 - "$app/Contents/Info.plist" <<'PY'
import plistlib,re,sys,os,subprocess
version=re.search(r'project\(TheFunkBoxx VERSION ([0-9.]+)',open('CMakeLists.txt').read())[1]
with open(sys.argv[1],'wb') as f:
 plistlib.dump(dict(CFBundleIdentifier='com.prohoezak.thefunkboxx.installer',CFBundleName='Install TheFunkBoxx',CFBundleDisplayName='Install TheFunkBoxx',CFBundleExecutable='InstallTheFunkBoxx',CFBundlePackageType='APPL',CFBundleShortVersionString=version,CFBundleVersion=version,LSMinimumSystemVersion='11.0',NSPrincipalClass='NSApplication',NSHighResolutionCapable=True,TheFunkBoxxSourceCommit=os.environ.get('GITHUB_SHA') or subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()),f)
PY
/usr/bin/codesign --force --sign - --timestamp=none "$app"
/usr/bin/codesign --verify --deep --strict "$app"
/usr/bin/lipo "$app/Contents/MacOS/InstallTheFunkBoxx" -verify_arch arm64 x86_64
python3 scripts/run-validator.py 30 "$build/smoke.log" "$app/Contents/MacOS/InstallTheFunkBoxx" --smoke-test "$build/smoke.json"
python3 - "$build/smoke.json" <<'PY'
import json,sys
report=json.load(open(sys.argv[1]));assert report['visible'] and report['button']=='Install',report
print('PASS: actual native installer window opened with Install control')
PY
