# BASSBOXX signed native installer

Team: **5QF6Q5ATG5**. The application identity has already imported successfully
in GitHub Actions. The Installer identity import failed, so distribution has been
changed to a notarized native installer app/disk image. **No Installer certificate
or Installer secret is required or read by the new workflow. Do not re-export
or paste another private signing file.**

## Receiving Mac

**[Download the accepted notarized installer](https://github.com/prohoezak-hue/bassboxx/releases/download/developer-id-09399faec62b/BASSBOXX-macOS-Installer.dmg)**.
Apple accepted both the native app and disk image, tickets were stapled/validated,
and Gatekeeper assessments passed in [run 41797870831](https://github.com/prohoezak-hue/bassboxx/actions/runs/41797870831).
No additional signing setup is needed to install this release.

After an accepted signed release exists, download BASSBOXX-macOS-Installer.dmg,
open it, open **Install BASSBOXX.app**, and click **Install** with MPC quit. It
installs AU/VST3 into your user Library/Audio/Plug-Ins folders, verifies signatures,
backs up previous copies and rolls back a failed upgrade. It preserves the working
standalone. No administrator password, unsigned command helper, certificate setup
or local compilation is required. Restart MPC, leave **Use Audio Units** checked
and choose **Rescan All**. Actual MPC scanning, playback and project recall remain
required local checks; Apple notarization is not MPC certification.

## Existing owner credentials

Only these four repository Actions secrets are used:

| Name | Purpose |
| --- | --- |
| MAC_APPLICATION_P12_BASE64 | Encoded encrypted replacement Application signing identity. |
| MAC_APPLICATION_P12_PASSWORD | Its export password. |
| APPLE_ID | Developer account email. |
| APPLE_APP_SPECIFIC_PASSWORD | Apple's app-specific password for notarization. |

The existing Installer secrets may be left alone; this workflow does not load
or use them. Private files/passwords must never be pasted into chat, issues or
source control. The old exposed encrypted Application archive was replaced;
retain the replacement identity securely and retire the old certificate through
Apple's certificate-management/support process when appropriate. No credentials
are included in the DMG or release artifacts.

## Automation

The native app is built for Apple Silicon and Intel with Apple Swift/AppKit.
Mac tests cover the real window, first installation, backups, signature refusal,
rollback after a partially committed upgrade and unrelated-product preservation.
The signing job additionally installs real signed payloads in a temporary home
using the production verifier and checks refusal of unsigned payloads.

Existing synth/standalone/AU/VST3 regressions and validators remain enabled before
signing. The Application identity is imported in a temporary keychain; Apple's
G2 intermediate is verified against system trust and imported without trust
overrides. Plugins and the installer app are Developer ID signed with hardened
runtime/timestamp. Apple must accept the app, its ticket is stapled/validated,
and Gatekeeper's execute assessment must pass. The DMG is then built/signed,
submitted, accepted, stapled/validated and assessed by Gatekeeper as a disk image.
The temporary keychain and decoded identity are removed on exit.

Only authorized main/repair-branch runs can sign: manual workflow dispatch or a
push explicitly marked **[notarize]** in the commit message. Pull requests do not
receive credentials. The explicit push trigger allows the agent to perform the
remaining cloud build/submission without another user button-click. Failures
report their non-sensitive phase in public annotations; no rejected/pending
submission is published as an accepted signed release. Existing unsigned ZIP
releases remain development builds; the separate developer-id release contains
the accepted native DMG and checksum.
