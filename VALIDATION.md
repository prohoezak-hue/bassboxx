# BASSBOXX macOS standalone and VST3 validation

## Developer ID distribution preparation

A manually dispatched job is prepared for team `5QF6Q5ATG5`, after the existing
Mac and portable validation. It requires Developer ID Application and Developer
ID Installer private-key identities plus Apple notarization credentials in
repository secrets. The two encrypted identities are imported separately, with
independent passwords; Apple's G2 intermediate is verified against system trust
and imported without trust overrides. It checks identity/team, signs both plugin slices with
hardened runtime and trusted timestamp, builds/signs a native plugin package,
requires an Accepted notary result, staples/validates the package, and requires
Gatekeeper installer assessment before publishing a distinct Developer ID release.
Its temporary keychain and decoded credentials are removed on exit; no credentials
are put in release artifacts. The native installer verifies the new signed
plugins before preserving/removing per-user duplicates and does not alter the
standalone or macOS security policy. Home-folder changes run as that user.

This is prepared infrastructure, not a completed notarization: no private-key
identities or notarization credentials have been configured in this session, so
actual signing, Apple submission and native installer runtime checks remain
blocked on secure account setup. Syntax/preflight results are recorded with the
setup commit; existing development-product validations below are unchanged.
Preparation checks passed: workflow YAML and all 17 shell blocks, both script
syntax checks, team-specific identity selection (valid/wrong/missing/ambiguous),
missing-credential failure before output creation, and rejection of non-Accepted
notarization status. These fixture/preflight checks are not macOS signing or
installer runtime verification.

## 1.2.0 Audio Unit loading route

The MPC screenshot showed Ignore/Skipped for both installed BASSBOXX VST3 paths,
other visible VST3s ignored, and zero failed plugins. Use Audio Units was enabled.
The screenshot alone does not prove the specific filtering/format-support cause.
Add a native AU MusicDevice (aumu/BsBx/PhZk) using the unchanged shared processor
and editor. The standalone and VST3 remain available. No security settings or
quarantine metadata are changed. The AU installer backs up/replaces only the AU
component in the user's Components directory.

Mac checks now require universal AU compilation, signatures, extracted ZIP and
payload integrity, installer upgrade/quarantine/tamper regressions, Apple registry
discovery/auval, actual AU MIDI audio/state/editor hosting, and pluginval. Registry
and host checks also exercise the AU freshly compiled from quarantined source.
AU validations run after installation; pre-install CTest does not assume a globally
registered component. The packaged AU builder validates after installing locally.
The full existing standalone/VST3 regressions remain enabled. MPC itself remains
a required user-Mac test; a format edition is not a compatibility certification.

The complete workflow passed for source commit
`d6b56037c6ec7597d9a483ef6c47d17bd3287da0` in
[CI run 41711400363](https://github.com/prohoezak-hue/bassboxx/actions/runs/41711400363)
(23 minutes 36 seconds overall; Mac job 23 minutes 1 second). This includes
Apple auval and actual AU host discovery/instantiation/MIDI audio/state/editor
checks on the installed precompiled AU and the AU freshly built from the
quarantined source payload, followed by pluginval on the registered local AU.
The existing standalone/VST3 suites, both VST3 validators, installers and
source-build checks passed too. No existing test was disabled. Local portable
engine CTest passed in 4.39 seconds; workflow YAML, shell blocks and scripts
passed syntax checks. Shared engine, processor and editor sources are unchanged.

**[Download verified AU 1.2.0](https://github.com/prohoezak-hue/bassboxx/releases/download/macos-d6b56037c6ec/BASSBOXX-macOS-AU.zip)**.
The public archive was independently downloaded and verified. SHA-256:
`0fb5db6f9d1c77183452e0e737ebfc83f9e05a1bdcd7c79321b058d78e16cad6`.
ZIP CRCs, payload hashes, executable helpers, AudioComponents identity/factory,
version 1.2.0, both arm64/x86_64 bundle slices and signature resources passed
inspection. Packaged source matches the tested commit exactly. The runner used
its native architecture; Intel runtime, Finder helper launch, real audio/MIDI and
MPC's actual AU scan, playback and project recall remain user-Mac checks.

## 1.1.1 reference skin and preset arrows

The editor now uses a cached dark wood face, orange-brown wood cheeks, brass
borders and knob rims, warm ivory keys and lettering, an amber power lamp and
colourful ProHoeZak wordmark styled after the supplied reference. All controls
remain live JUCE components. The original DSP, factory patch definitions,
parameter IDs, patch save/load and performance wheels are unchanged.

Stacked up/down buttons immediately before the sound dropdown load previous/next
factory sounds, wrapping at the ends. Holding a button repeats. Both buttons
read the processor's current program, so browsing continues from dropdown,
host and restored-state selections. The dropdown and voice mode refresh together.
Processor tests cover every factory sound, boundary wrapping, direct dropdown
selection, complete parameter loading, host/state changes and visible placement.
The full Mac/portable workflow passed for source commit
`25145632ff6d803a2221aca53688b98c86b89d9b` in
[CI run 41701917930](https://github.com/prohoezak-hue/bassboxx/actions/runs/41701917930)
(24 minutes 41 seconds overall, including the queue; Mac job 19 minutes 22 seconds).
All project suites, repeated standalone/VST3 launch checks, Steinberg Validator,
pluginval, installer/security regressions and the packaged local build route
passed. The real Mac editor snapshot was inspected for legibility and clipping.
Local Linux engine and native processor/editor CTest suites also passed (2/2,
4.18 seconds), including every factory sound and all new browsing behavior.

**[Download standalone 1.1.1](https://github.com/prohoezak-hue/bassboxx/releases/download/macos-25145632ff6d/BASSBOXX-macOS-Standalone.zip)**
([actual Mac panel preview](https://github.com/prohoezak-hue/bassboxx/releases/download/macos-25145632ff6d/BASSBOXX-interface.png)).
The public ZIP was independently downloaded and verified. SHA-256:
`be5408fb11e84cd680c8a5aad524204b8437c983a4af6e29b58fcc1c13c847c8`.
ZIP CRCs, payload checksums, executable launchers, version/bundle identity,
arm64/x86_64 executable slices and signature resources passed inspection.
Packaged source matches the tested commit byte for byte. Engine, factory
patch definitions and processor source are unchanged from verified 1.1.0.
The runner executed its native architecture; physical audio/MIDI on the user's
Mac and MPC 3.10 remain local checks. macOS security protections remain intact.

## 1.1.0 vintage skin, voice mode and wheels

This update adds the requested Model D-inspired panel, a Poly/Mono button tied to
existing Voices, last-note mono/legato with held-key return, and onscreen pitch
and modulation wheels. The pitch wheel returns to centre; modulation holds its
position and adds 5 Hz vibrato with a maximum ±0.5-semitone depth. Hardware CC1 and
pitch bend are handled at MIDI event sample positions, per channel. UI wheel
requests use atomics and are applied to all channels on the next audio block.
The wheels are transient controls; existing patch/parameter IDs remain unchanged.

Local engine checks cover mono note priority/return, release, sustain, panic,
poly/mono transitions, per-channel modulation, finite audio and vibrato depth.
A separate before/after audio comparison against the previous engine rendered all
100 presets at 44.1/48/96 kHz with wheels at rest: maximum sample difference was 0
for single notes and the tested polyphonic sequences. Mono overlapping-note
behavior is intentionally improved rather than compared with the old behavior.

Processor/editor tests now drive the actual mode button and wheels, compare the
rendered wheel-controlled audio with equivalent hardware MIDI, check spring-back,
held modulation, patch mode recall, visible bounds and all 21 original knobs.
The existing standalone app, VST3 host/scan, external validator, quarantine and
local-build/installer checks remain required for the new release. The new skin
snapshot is rendered by the real editor and published with the release.

The full Mac and portable workflow passed for source commit
`dc58fe446ee7624cb295ed6363bce6995d3ae339` in
[CI run 41690506224](https://github.com/prohoezak-hue/bassboxx/actions/runs/41690506224)
(26 minutes 32 seconds overall; Mac job 24 minutes 59 seconds). This includes
all project tests, repeated standalone/VST3 launch checks, both external
validators, installer/security regressions and the packaged local build route.
No tests were disabled. The real editor screenshot was visually inspected for
legibility, control placement and clipping.

**[Download updated standalone 1.1.0](https://github.com/prohoezak-hue/bassboxx/releases/download/macos-dc58fe446ee7/BASSBOXX-macOS-Standalone.zip)**
([actual panel preview](https://github.com/prohoezak-hue/bassboxx/releases/download/macos-dc58fe446ee7/BASSBOXX-interface.png)).
The published ZIP was independently downloaded and verified. SHA-256:
`8a68368c3457064434ea3f1ac098bacbf4fabcd16e9c907e0da60bb4f05c4683`.
ZIP CRCs, payload hashes, executable launchers, version/bundle identity,
arm64/x86_64 executable slices and signature resources passed inspection.
Packaged instrument/test source matches the tested commit byte for byte.
Physical MIDI hardware, live audio on the user's Mac and MPC 3.10 remain local
checks; no MPC compatibility claim is made. macOS protections remain intact.

## Standalone application

The new standalone target adds `BASSBOXX.app` alongside the VST3 at the user's
request. The earlier VST3-only restriction is superseded. Both formats use the
same original processor, editor and engine; the new application host supplies
JUCE's audio/MIDI device settings and normal-quit state persistence.

New checks launch the actual app event loop/window, verify the original editor,
100 programs and non-silent finite stereo MIDI audio, and require a fresh JSON
report. Packaging checks both universal slices, signatures, app install/upgrade,
quarantine/tamper rejection, and startup after ZIP extraction. The source helper
is exercised from a quarantined app payload and launches the locally installed
app in self-test mode. These checks passed for source commit
`87603214259b38da9bd1cdca274b1c4c0dcf3212` in
[CI run 41607310512](https://github.com/prohoezak-hue/bassboxx/actions/runs/41607310512),
including publication (22 minutes 23 seconds overall; Mac job 20 minutes 29 seconds).
The five project tests, repeated standalone/VST3 scans and loads, both external
VST3 validators, both product installers, extracted app startup, and the packaged
local app build/install route completed successfully. No tests were disabled.

**[Download BASSBOXX-macOS-Standalone.zip](https://github.com/prohoezak-hue/bassboxx/releases/download/macos-87603214259b/BASSBOXX-macOS-Standalone.zip)**
([release](https://github.com/prohoezak-hue/bassboxx/releases/tag/macos-87603214259b)).
The published ZIP was downloaded independently and matched its published SHA-256:
`39a5afa4743635288b3e4333306138cc3a47cb1847231a132365ba8ca9346cdf`.
ZIP CRCs, payload hashes, launcher permissions, APPL bundle identity, signature
resources and both arm64/x86_64 Mach-O executable slices passed inspection. The
original engine, editor and processor source remained byte-for-byte unchanged.
The unused nib is absent from the app. The older results below refer to the
VST3-only commits unless stated otherwise.

A focused Mac probe confirmed that JUCE's optional precompiled
`RecentFilesMenuTemplate.nib` inherited quarantine when copied from the downloaded
JUCE archive into the app. BASSBOXX does not use a native Recent Files menu;
its original patch save/load controls are independent. The standalone target now
omits that unused resource. No quarantine attribute is removed from any source,
download or binary. The helper also checks the app before executing local tests.
The full quarantined-payload build/install regression passed after that fix; the source archive retains its quarantine metadata.

A standalone format does not confer Apple notarization. Downloads remain ad-hoc
signed development builds. The app runs without MPC; physical audio output,
hardware MIDI and local Apple policy still require testing on the user's Mac.

## Security rejection and local development installation

The user's Mac reported quarantine present, `spctl: rejected`, and an intact
codesign signature. The distributed plugin is ad-hoc signed, not Developer ID
signed/notarized. These observations are consistent; signature integrity is not
distribution approval. No paid Apple membership or signing credentials are
available, and no change to quarantine or security policy is authorized.

The installer now stops before replacement when a quarantined bundle is rejected,
rather than reporting a successful installation from an integrity check alone.
The ZIP includes a local-build helper, exact-commit original source, pinned JUCE
8.0.6 source and an official CMake 3.31.10 disk image. All payload checksums are
checked. CMake itself must pass codesign verification and Apple's assessment.
The helper uses free Apple Command Line Tools, compiles a new universal VST3,
runs four project tests, signs only newly compiled output, refuses quarantined
local output, installs with backup, and checks actual installed VST3 loading.
A local development build does not turn `spctl` rejection into distribution
approval and does not override managed policy. Finder launch of the helper and
MPC loading on the user's Mac remain separate, required local checks.

New CI coverage includes metadata-only scanning, repeated scan/load cycles,
phase timing and 30-second host deadlines, installer first-install/upgrade,
root/nested quarantine rejection with previous-install preservation, signature
tamper rejection, ZIP payload verification, a full build from a simulated
quarantined extracted payload, and both external validators on the locally
built/installed VST3. These checks passed for repair commit
`f7b6961d6e0573a7067963391ac7cff43a63fc01` in
[CI run 41592649183](https://github.com/prohoezak-hue/bassboxx/actions/runs/41592649183)
(20 minutes 35 seconds overall; Mac job 20 minutes 12 seconds). The run completed
successfully, including publication. No project test or validator was disabled.
The Mac runner executed its native architecture; both universal slices were
compiled/verified, but Intel runtime/MPC testing remains separate.

**[Download the new local-development ZIP](https://github.com/prohoezak-hue/bassboxx/releases/download/macos-f7b6961d6e05/BASSBOXX-macOS-VST3.zip)**
([release notes](https://github.com/prohoezak-hue/bassboxx/releases/tag/macos-f7b6961d6e05)).
The published ZIP was independently downloaded and checked against its published
SHA-256: `beb60e9357f8626eede09f2275ce92e792532394b66ee2bb6f990d348e59f19e`.
ZIP CRCs, every payload checksum, executable launcher permissions, bundle identity,
signature resources and both 64-bit Mach-O slices passed inspection. The included
instrument source matched this commit byte for byte. The original `Source/` files
are unchanged from the previous repair baseline.

For installation, follow `packaging/Installation.txt` or the same file inside the
ZIP. Use **Build BASSBOXX on this Mac.command** for the free local-development
route. The precompiled plugin remains ad-hoc signed and can still be rejected;
no Apple distribution approval or MPC compatibility is claimed. A user's Finder
launch, actual Mac policy and MPC 3.10 scan/playback remain unverified. The
historical results below apply only to their stated source commits.

## MPC 3.10 scan timeout investigation

MPC 3.10 has been reported to time out while scanning BASSBOXX. The previous
offline test results below do not establish compatibility with that host.
The repair branch adds the official Steinberg VST3 Validator (SDK 3.7.14 build
55) and pluginval 1.0.4 at strictness level 5, with editor tests enabled, to the
macOS workflow. Both tools are built from pinned source commits and their
pinned submodules. They validate the actual universal VST3 bundle using the
runner's native architecture; this alone does not test both architecture slices.

The validator processes have 180-second and 600-second wall-clock limits,
respectively. pluginval also retains its 30-second inactivity timeout. A timeout,
crash or nonzero exit fails the job. Both validators are attempted independently
after successful builds, even if a preceding test fails. Their output and CTest
results are retained in the test-evidence artifact on failure. The Mac ZIP is
packaged and uploaded only after all checks pass; repair-branch builds do not
publish a release or replace the existing download.

Repair commit `3c421ee` passed the [macOS validation and packaging run](https://github.com/prohoezak-hue/bassboxx/actions/runs/41586405381).
The workflow completed successfully in 10 minutes 25 seconds, including both
external validators, the existing engine/processor/real-VST3-host tests, editor
rendering, universal architecture and signature verification, and installer/ZIP
checks. The run produced the `BASSBOXX-macOS-VST3` artifact (7.2 MB), containing
the new ZIP and its checksum. Download it from that run's Artifacts section while
signed in to GitHub. This repair-branch artifact does not replace the public
release linked below.

Local verification also passed: portable engine CTest (1/1), validator runner
success/failure/timeout/missing-executable/signal behavior, and the pinned
Steinberg validator's 43 internal self-tests. No validator failure or timeout
was reproduced. No DSP, presets, interface, or plugin features were changed by
this CI integration. The reported MPC 3.10 scan timeout remains unconfirmed and
unfixed; an actual MPC rescan is still required before claiming compatibility.

## Finished download

[Download BASSBOXX-macOS-VST3.zip](https://github.com/prohoezak-hue/bassboxx/releases/download/macos-00805dd4f48d/BASSBOXX-macOS-VST3.zip)

- Compiled source commit: `00805dd4f48d9681873fcd80a34a6e3950466281`.
- Version: 1.0.1. Universal macOS VST3 with `arm64` (Apple Silicon/M-series) and `x86_64` (Intel) slices; deployment target macOS 11.
- [Successful Mac build, tests, packaging and publication](https://github.com/prohoezak-hue/bassboxx/actions/runs/41501027868).
- Release ZIP downloaded again from its public URL and independently verified in the cloud.
- SHA-256: `bd64550fcd443fb0eb631eba02afd22aa7efa3a46e3ddc484ef367176a5db455`.

The only instrument product is `BASSBOXX.vst3`. There is no standalone application target or app in the ZIP. Non-macOS builds produce shared processor code and test utilities only. The engine, presets, keyboard and metallic-red/wood/black-knob interface source are unchanged in this packaging update.

## Checks that passed

- macOS universal VST3 compilation and both engine/processor CTest suites; offscreen rendering of the actual editor.
- Engine: audio generation, release to silence, sustain, voice stealing, 100 presets at 44.1/48/96 kHz, panic, Bass Boost energy, oscillator pitch and pitch bend.
- Processor: sample-position MIDI, finite stereo audio, 100 factory programs, parameter/program state roundtrip, MIDI panic and visible bounds of all 21 synthesis knobs.
- Mac packaging: verification of both architecture slices; ad-hoc signing and strict signature verification; installer first-install and upgrade in temporary directories; ZIP creation, extraction and re-verification of signatures, architectures and installer executable permissions.
- GitHub portable DSP regression job, plus local Debian 13/GCC 14.2 engine-only and shared-processor builds/tests (2/2 CTest suites in the latter). No tests were disabled.
- Workflow YAML parsing and Bash syntax checks for all workflow shell blocks and both packaging/installation scripts.
- Pinned JUCE 8.0.6 source archive SHA-256 and essential contents verified. CMake checks that hash when automatically downloading JUCE.
- Independent release download: matching published SHA-256, CRC integrity of every ZIP member, required installer/instructions/notices, exact source-commit metadata, signature resources, real 64-bit Mach-O slices for arm64 and x86_64, and absence of a standalone `.app`.

GitHub Actions publishes versioned ZIP releases only after its Mac and portable engine jobs pass. Documentation-only updates do not rebuild the unchanged binary; the compiled source commit above identifies this release exactly.

## Still requires MPC on a Mac

Actual MPC desktop scanning, MIDI hardware, live audio, automation, custom patch file dialogs, project recall, CPU/latency and listening tests have not been performed here. Use an MPC desktop version with VST3 instrument support. This plugin is for the desktop software, not MPC hardware standalone mode.

The plugin is ad-hoc signed, not Apple Developer ID signed/notarized. macOS or managed security policy may block downloaded development plugins and require a Developer ID signed/notarized release. No Apple signing credentials or commercial JUCE license are supplied.

The repair branch subsequently added pluginval, Steinberg validator and real VST3 host checks (see results above). Other DAWs and MPC were not tested. These results demonstrate offline synthesis and build/package readiness; they do not certify MPC compatibility.
