# BASSBOXX by ProHoeZak

A macOS VST3 synthesizer instrument for Akai MPC desktop. Supports native Apple Silicon (M-series/arm64) and Intel Macs, macOS 11 or newer. The only instrument product is `BASSBOXX.vst3`; no standalone application is built or packaged.

![BASSBOXX interface](docs/BASSBOXX-interface.png)

[Plugin downloads](https://github.com/prohoezak-hue/bassboxx/releases) · [Automatic build status](https://github.com/prohoezak-hue/bassboxx/actions/workflows/build.yml) · [Audio demo](examples/BASSBOXX-demo.wav) · [Validation details](VALIDATION.md)

## Download and install — no programming required

1. Open **Plugin downloads** above and choose the newest completed **BASSBOXX macOS** release. Download **BASSBOXX-macOS-VST3.zip** under **Assets**, rather than the source-code archive. A release appears only after the Mac build, tests and packaging checks pass. GitHub sign-in may be required for a private repository. If there is no release yet, inspect **Automatic build status**; a failed or pending build does not provide a finished plugin.
2. Quit MPC desktop and extract the entire ZIP.
3. Double-click **Install BASSBOXX.command** in the extracted folder. It installs the precompiled plugin into your user VST3 folder and keeps an older installation as a backup. No JUCE, Xcode, administrator password or code editing is needed. If Finder asks, right-click → Open, if your Mac's security policy permits it.
4. Restart MPC desktop and rescan the user VST3 folder in its plugin preferences/manager:
   `~/Library/Audio/Plug-Ins/VST3/`
5. Create a plugin program/track, choose **BASSBOXX**, enable monitoring and route your MIDI controller/pads to the track. Select an audio device and begin at 44.1 or 48 kHz, 256 samples. You can also click the three-octave keyboard. Start with low monitor volume.

For manual installation, copy **BASSBOXX.vst3** into the same folder using Finder → Go → Go to Folder. Create the folder if needed. The ZIP includes detailed installation instructions, build/source information and a signature-verified universal plugin. Each release also supplies a SHA-256 checksum file.

### Compatibility and remaining Mac testing

Use an MPC desktop version supporting third-party **VST3 instruments**. VST2-only MPC versions cannot load this plugin. MPC menu names vary by version. MPC hardware standalone mode cannot load desktop VST3 plugins.

The plugin is ad-hoc signed automatically in CI. It is not Apple Developer ID signed/notarized; macOS or managed security policy may block downloaded development plugins. If blocked, a Developer ID signed/notarized build is required for that policy. Do not disable system security protections. Developer ID signing requires the owner's Apple credentials, which are not supplied here.

After installation, verify MPC scanning and live playback, switch factory presets, automate cutoff/Bass Boost, and save/reopen an MPC project to check parameter recall. Test pitch bend (±2 semitones), sustain CC64, polyphony, Panic and an exported MIDI-pattern bounce. These host-specific and listening checks need MPC on a Mac. Offline CI tests are not MPC certification. Download availability and the last observed validation results are recorded in **VALIDATION.md**.

## Instrument

- Three mixed oscillators: band-limited polyBLEP saw/pulse, triangle and sine; level and ±24-semitone fine/interval tuning per oscillator. Triangle is analytic rather than fully band-limited, so extreme notes can alias.
- Driven two-pole TPT resonant low-pass; cutoff, resonance and envelope modulation. Independent amplitude and filter ADSR contours.
- Independent amplitude/filter attack, decay, sustain, release; exponential frequency glide; 1–16 voices with oldest-voice stealing. Mono mode glides from the previous voice; it does not implement a held-note priority stack.
- Bass Boost adds a 160 Hz low-pass component before soft saturation; output gain and soft limiting bound the output. This is a virtual analog design, not a measured emulation of a particular hardware synth.
- Sample-position MIDI note events, velocity, per-channel sustain and pitch bend, all-notes/all-sound-off, plus on-screen keyboard. Audio is dual mono. No external audio input.
- 100 deterministically generated factory programs: ten variations in each of Foundation, Glide, Acid, Pluck, Sub, Fifth, Atmosphere, Vintage, Lead and Impact. These are starting patches, not a separately auditioned commercial sound library.
- Host automation for all synthesis parameters; host state stores parameter values and factory program index. Save/load custom `.bassboxx` files using the editor buttons. Factory programs remain immutable. Imported custom patches retain their saved program index but the parameter state is authoritative.
- Metallic red procedural panel, wood-grain side cheeks, black rotary knobs and ProHoeZak branding. No external image assets are required.

## Automated builds and development

GitHub Actions runs on pushes to `main`, pull requests and manual workflow dispatch. It builds a universal `arm64;x86_64` macOS VST3, runs engine/processor/editor tests, verifies both binary slices, ad-hoc signs and verifies the bundle, tests installer installation and upgrade, then extracts and verifies the ZIP again. Linux runs the portable DSP regression suite. Both jobs must pass before the publication job creates a versioned development release with the ZIP and checksum. It uses GitHub's built-in workflow token, with write permission scoped to publication; no personal token is needed on the runner.

For maintainers, the build commands are in `.github/workflows/build.yml`; packaging is in `scripts/package-macos.sh`. CMake fetches pinned JUCE 8.0.6 (commit `51a8a6d7aeae7326956d747737ccf1575e61e209`) automatically. `-DJUCE_SOURCE_DIR=/path/to/JUCE` optionally reuses that dependency. On non-macOS platforms CMake builds the shared processor code and test utilities only, without a plugin or standalone target. `-DBASSBOXX_ENGINE_ONLY=ON` builds portable engine tests without JUCE. Neither test utilities nor the audio-demo renderer are shipped as instrument applications.

`Source/Engine.h` implements portable DSP, `Plugin.cpp` handles JUCE MIDI/automation/state, and `Editor.h` implements the controls and custom patch files. The existing engine, 100 presets and vintage interface remain unchanged in the VST3-only packaging update.

## Distribution and licensing

Project copyright remains with its owner; no public redistribution license for original source is granted here. JUCE is a separate dependency available under AGPLv3 or commercial terms. Before distributing to others, choose and comply with the appropriate [JUCE license](https://juce.com/legal/juce-8-licence/) and bundled notices. Merely compiling or linking does not establish a commercial license. No Apple signing credentials or commercial JUCE license are included.
