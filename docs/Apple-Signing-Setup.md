# BASSBOXX Apple signing setup — one-time setup

Team ID configured in the distribution workflow: **5QF6Q5ATG5**.

The goal is a normal **BASSBOXX-macOS-Plugins.pkg**: open it with Apple's Installer,
without a downloaded command helper or compiling on your Mac. It installs AU and
VST3. It leaves your working standalone app alone. Signing/notarization addresses
Apple's distribution trust checks; a successful MPC AU scan and playback still
need testing on your Mac. No signed/notarized release exists until this setup is
complete and Apple accepts an actual submission.

## 1. Create the two Developer ID certificates

Use the Mac you control. The private keys should be generated there, not in chat.

1. Open **Keychain Access** using Spotlight. (This is the Keychain Access app,
   separate from the newer Passwords app.)
2. From the **Keychain Access** menu choose **Certificate Assistant → Request a
   Certificate From a Certificate Authority**.
3. Enter your email and name, leave the CA email blank, choose **Saved to disk**,
   and save the request. This is a `.certSigningRequest` file. The corresponding
   private key remains in your login keychain. If asked for key details, use RSA,
   2048 bits.
4. Open [Apple Developer Certificates](https://developer.apple.com/account/resources/certificates/list).
   Select team **5QF6Q5ATG5** if Apple offers a team selector. Click **+** and select
   **Developer ID Application**. Upload the request, generate and download the
   certificate, then double-click the downloaded `.cer` file to add it to Keychain
   Access.
5. Repeat the request/certificate steps for **Developer ID Installer**. Use a
   separate request/name so each certificate has its own matching private key.
6. In Keychain Access, select your **login** keychain and **My Certificates**.
   Both Developer ID certificates should appear, each expandable to show its
   private key. An Apple Development, Apple Distribution, Mac App Distribution or
   Mac Installer Distribution certificate is not a substitute for Developer ID.

If Developer ID choices are unavailable, check that paid membership is active,
you have accepted Apple's agreements, and you are using the account-holder team.
Do not purchase membership again or change certificate trust settings to bypass
an invalid certificate. A `.cer` without its matching private key is insufficient.

## 2. Keep the two encrypted identities separate

The current setup uses **Application.p12** and **Installer.p12** in
`Desktop/BASSBOXX-Apple-Signing`. They were created from the matching private
keys and Apple certificates, imported in the login keychain, and both identities
were confirmed valid. If you already completed this, do not regenerate keys or
export them again. The workflow imports these two files separately; their export
passwords may differ.

If starting from Keychain Access instead, export each Developer ID identity
separately from **My Certificates**, including its associated private key, as
**Personal Information Exchange (.p12)**. Save each in the Desktop folder above,
using the names Application.p12 and Installer.p12. Set strong export passwords
and retain them privately. Do not export unrelated identities.

Keep private keys, encrypted .p12 files and passwords out of chat and source
control. No certificate trust overrides are required. The workflow verifies and
imports Apple's official G2 intermediate to complete the signing chain.

## 3. Create an Apple app-specific password

At [account.apple.com](https://account.apple.com/), open **Sign-In and Security →
App-Specific Passwords** and generate a password named **BASSBOXX GitHub**.
This is for Apple's notarization service. It is different from both your normal
Apple Account password and each .p12 export password. Do not send any of them in chat.

## 4. Put six values in GitHub Actions secrets

Open [BASSBOXX Actions secrets](https://github.com/prohoezak-hue/bassboxx/settings/secrets/actions).
Choose **New repository secret** for each entry below. This is the secure GitHub
settings page, not an issue, source file, pull request, Actions input or chat.

| Secret name (exact spelling) | What to enter |
| --- | --- |
| `MAC_APPLICATION_P12_BASE64` | The encoded Application.p12 copied using the first command below. |
| `MAC_APPLICATION_P12_PASSWORD` | The export password for Application.p12 (not its original .key password). |
| `MAC_INSTALLER_P12_BASE64` | The encoded Installer.p12 copied using the second command below. |
| `MAC_INSTALLER_P12_PASSWORD` | The export password for Installer.p12 (not its original .key password). |
| `APPLE_ID` | The Apple Account email associated with this developer team. |
| `APPLE_APP_SPECIFIC_PASSWORD` | The app-specific password generated in step 3. |

Copy **Application.p12** into the clipboard using Terminal:

```sh
/usr/bin/base64 -i "$HOME/Desktop/BASSBOXX-Apple-Signing/Application.p12" | /usr/bin/pbcopy
```

Paste it only into the `MAC_APPLICATION_P12_BASE64` secret's value field and save.
Then copy **Installer.p12**:

```sh
/usr/bin/base64 -i "$HOME/Desktop/BASSBOXX-Apple-Signing/Installer.p12" | /usr/bin/pbcopy
```

Paste this second value only into `MAC_INSTALLER_P12_BASE64` and save. The commands
copy without printing the private material. Clear or replace the clipboard after
saving. Enter each export password in its corresponding password secret. These
may be different passwords. Store both encrypted files securely for future updates.

GitHub does not display saved secret values. Verify the six secret **names**
exist; do not send screenshots showing their entered values.

## 5. Run the prepared distribution workflow

Open [the BASSBOXX build workflow](https://github.com/prohoezak-hue/bassboxx/actions/workflows/build.yml).
Choose **Run workflow**, select branch **fix/vst3-host-load-validation**, then run.
The workflow already exists on the repository; use this branch so the new
Developer ID job is included. If Run workflow is absent, confirm you are signed
in to GitHub with repository write access. Do not merge unrelated changes just
to expose the button.

A manual run now checks for the six secrets first, runs the existing Mac and
portable validation, then runs **Sign and notarize native Mac installer**. Normal
push/PR development runs do not receive the signing credentials and do not run
that distribution job. The signing step requires both valid identities for your
team and authenticates with Apple's service; it has no ad-hoc fallback.

The workflow signs the plugins with Developer ID Application, signs the native
package with Developer ID Installer, submits the package to Apple, requires
**Accepted**, staples and validates the ticket, then requires a successful
Gatekeeper installer assessment. It uses a temporary keychain and removes it
and the decoded credentials on exit. Only a successful run publishes a separate
**BASSBOXX notarized Mac plugins** prerelease and its package checksum.

## 6. Install and test the notarized package

After a successful signed release exists, download **BASSBOXX-macOS-Plugins.pkg**,
quit MPC, and open it with Apple's Installer. Administrator authorization installs
AU/VST3 into the system plugin folders. After verifying the new signatures, the
installer preserves older per-user BASSBOXX plugin copies under your home
`Library/Application Support/BASSBOXX/Backups` folder to avoid duplicate discovery.
It does not replace the standalone app or change macOS security/quarantine policy.

Restart MPC, leave **Use Audio Units** checked and choose **Rescan All**. Select
BASSBOXX as an AU instrument. Test scanning, notes, interface, presets, wheels,
and saving/reopening an MPC project. Apple notarization is not MPC certification.
