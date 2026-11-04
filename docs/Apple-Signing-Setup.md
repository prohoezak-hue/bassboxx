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

## 2. Export both signing identities as one encrypted .p12

1. In **My Certificates**, select both Developer ID certificate rows, including
   their associated private keys.
2. Choose **File → Export Items**. Select **Personal Information Exchange (.p12)**.
3. Save it as **BASSBOXX-signing.p12** on your Desktop. Set a strong export
   password and keep it in your password manager. macOS may request your login
   keychain password to authorize the export.
4. Keep this encrypted file secure. Do not upload it to the repository, send it
   in chat, or commit it. The two certificates must belong to the configured team.

## 3. Create an Apple app-specific password

At [account.apple.com](https://account.apple.com/), open **Sign-In and Security →
App-Specific Passwords** and generate a password named **BASSBOXX GitHub**.
This is for Apple's notarization service. It is different from both your normal
Apple Account password and the .p12 export password. Do not send any of them in chat.

## 4. Put four values in GitHub Actions secrets

Open [BASSBOXX Actions secrets](https://github.com/prohoezak-hue/bassboxx/settings/secrets/actions).
Choose **New repository secret** for each entry below. This is the secure GitHub
settings page, not an issue, source file, pull request, Actions input or chat.

| Secret name (exact spelling) | What to enter |
| --- | --- |
| `MAC_SIGNING_P12_BASE64` | The encoded encrypted .p12 copied using the command below. |
| `MAC_SIGNING_P12_PASSWORD` | The export password you chose for the .p12. |
| `APPLE_ID` | The Apple Account email associated with this developer team. |
| `APPLE_APP_SPECIFIC_PASSWORD` | The app-specific password generated in step 3. |

To copy the encoded .p12, open Terminal and run this single command:

```sh
/usr/bin/base64 -i "$HOME/Desktop/BASSBOXX-signing.p12" | /usr/bin/pbcopy
```

It copies the value to your clipboard without printing it. Paste it only into
the `MAC_SIGNING_P12_BASE64` secret's value field on GitHub and save it. Clear
or replace your clipboard afterwards. Store the encrypted .p12 securely for
future updates; it should not be part of any download or repository commit.

GitHub does not display saved secret values. Verify the four secret **names**
exist; do not send screenshots showing their entered values.

## 5. Run the prepared distribution workflow

Open [the BASSBOXX build workflow](https://github.com/prohoezak-hue/bassboxx/actions/workflows/build.yml).
Choose **Run workflow**, select branch **fix/vst3-host-load-validation**, then run.
The workflow already exists on the repository; use this branch so the new
Developer ID job is included. If Run workflow is absent, confirm you are signed
in to GitHub with repository write access. Do not merge unrelated changes just
to expose the button.

A manual run now checks for the four secrets first, runs the existing Mac and
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
