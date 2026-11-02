# BASSBOXX VST3-only validation

The instrument product is macOS VST3 only. The standalone target and local-build requirement for end users have been removed. On non-macOS machines, CMake provides shared processor code and test utilities only; it creates no instrument plugin or standalone target. Legacy generated Linux plugin/application outputs were removed from the cloud checkout.

## Cloud checks

Debian 13 x86-64, GCC 14.2, CMake 4.4.4, pinned JUCE 8.0.6.

- Portable engine-only CMake build and tests pass.
- Full shared processor build succeeded; engine and processor CTest suites passed (2/2), including the editor bounds checks. Separate offscreen editor rendering also passed. Tests were not disabled.
- Engine assertions cover audio generation, release to silence, sustain, voice stealing, 100 presets at 44.1/48/96 kHz, panic, Bass Boost energy, oscillator pitch and pitch bend.
- Processor assertions cover sample-position MIDI, finite stereo audio, 100 factory programs, parameter/program state roundtrip, MIDI panic and visible bounds of all 21 synthesis knobs. The actual editor can also render to PNG.
- Both macOS shell scripts and every inline workflow shell block pass Bash syntax checks; workflow YAML parses.
- Pinned JUCE source archive SHA-256 and essential file contents verified. CMake automatically verifies that checksum when downloading JUCE.
- No changes were made to the engine, presets or interface source in this packaging update.

## macOS automation

The initial project's macOS Actions run succeeded (run 41495246841). The new VST3-only workflow separately builds a universal arm64/x86_64 VST3 and runs all existing tests. Its packaging step verifies architecture slices, applies and verifies an ad-hoc signature, exercises first installation and upgrade in temporary folders, creates the ZIP, extracts it again and verifies signatures, architecture and installer permissions. Publication requires both Mac and portable engine jobs to succeed.

See the [current Actions runs](https://github.com/prohoezak-hue/bassboxx/actions/workflows/build.yml) for the current commit's result and [Releases](https://github.com/prohoezak-hue/bassboxx/releases) for completed downloads. A configured workflow alone is not evidence of a successful build. A failed/pending workflow has no verified finished ZIP.

## Still needs MPC on a Mac

MPC scanning, actual MIDI hardware and real-time audio, automation, live preset changes, custom patch file dialogs, project recall, CPU/latency and listening tests have not been verified in MPC here. Use a desktop version with VST3 instrument support. The plugin is ad-hoc signed; Developer ID signing/notarization and a commercial JUCE license are not provided. A Mac security policy may require a notarized build.

External pluginval/VST3-host validation and other DAWs were not tested. These results do not certify MPC compatibility.
