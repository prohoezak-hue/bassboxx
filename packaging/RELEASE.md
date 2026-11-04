# The Funk Box macOS installer

The product name is The Funk Box; version 1.0.2. Internal CMake target, bundle ID,
manufacturer/plugin codes, state-tree type and .bassboxx extension remain stable.
Host recall across the rename must still be verified on a Mac before release.

## Development build on a Mac

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build --config Release --parallel 3
ctest --test-dir build -C Release --output-on-failure
bash scripts/package-pkg-macos.sh
```

Output: `dist/The Funk Box-development.pkg`, plus SHA-256 checksum.
Installs `The Funk Box.vst3` in `/Library/Audio/Plug-Ins/VST3/`.
The package includes JUCE notices. It does not delete the old BASSBOXX bundle.
Move that bundle outside host scan paths before checking migration.

## Customer build on a Mac

Requires an Apple Developer account with Developer ID Application and Developer
ID Installer certificates, including their private keys, in the signing keychain.
Create a notarytool keychain profile using Apple's documented authentication flow.
Do not commit certificates, passwords, or private keys to this repository.

After building and testing, set the following to the actual keychain identity and
profile names, then run:

```sh
export DEVELOPER_ID_APPLICATION='Developer ID Application: YOUR NAME (TEAMID)'
export DEVELOPER_ID_INSTALLER='Developer ID Installer: YOUR NAME (TEAMID)'
export NOTARY_PROFILE='your-notary-profile'
RELEASE_MODE=customer bash scripts/package-pkg-macos.sh
```

The script verifies both architectures, signs the plugin with hardened runtime,
signs the installer, submits for notarization, staples the ticket, checks Gatekeeper,
and verifies the expanded payload. It produces `dist/The Funk Box.pkg` and checksum
only after those steps pass. CI uses the six existing repository secrets to import certificates into a temporary
keychain, derive and match their team IDs, and sign/notarize the package. Pull requests
produce development packages without accessing secrets. Pushes to main or
funk-box-installer and manual runs produce signed installers. These are workflow
artifacts; no customer release is automatically published.

Before selling, verify clean installation, upgrade, and recall of an existing
BASSBOXX MPC project on a Mac with only the renamed plugin in the scan paths.
Also confirm the selected JUCE licensing terms cover the intended distribution.
