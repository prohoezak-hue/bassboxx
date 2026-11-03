# BASSBOXX macOS standalone and VST3 validation

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
app in self-test mode. These additions require a new passing Mac CI run;
results below refer to the older VST3-only commits unless stated otherwise.

A focused Mac probe confirmed that JUCE's optional precompiled
`RecentFilesMenuTemplate.nib` inherited quarantine when copied from the downloaded
JUCE archive into the app. BASSBOXX does not use a native Recent Files menu;
its original patch save/load controls are independent. The standalone target now
omits that unused resource. No quarantine attribute is removed from any source,
download or binary. The helper also checks the app before executing local tests.
The full quarantined-payload build/install regression remains required.

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
