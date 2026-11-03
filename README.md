# BASSBOXX by ProHoeZak

A standalone macOS synthesizer and VST3 instrument. Supports native Apple Silicon (M-series/arm64) and Intel Macs, macOS 11 or newer. Run **BASSBOXX.app** on its own, or use **BASSBOXX.vst3** in a compatible music host.

![BASSBOXX interface](docs/BASSBOXX-interface.png)

**[Download BASSBOXX standalone for Mac — ZIP](https://github.com/prohoezak-hue/bassboxx/releases/download/macos-dc58fe446ee7/BASSBOXX-macOS-Standalone.zip)**

Choose **BASSBOXX-macOS-Standalone.zip** from the newest standalone-and-VST3 release. Extract the entire folder and open **BASSBOXX.app**. No MPC or other host is required. In the app choose **Options > Audio/MIDI Settings** to select speakers/headphones and enable your MIDI controller, or play the original on-screen keyboard.

The app preserves the original panel, synthesizer, 100 presets and patch-file controls. Normal quitting saves its state. The standalone ZIP also includes a local-build helper that installs a newly compiled app in `~/Applications/BASSBOXX.app` using free Apple Command Line Tools. If macOS rejects the download, changing to standalone has not solved its trust status: follow the included local-build instructions, and stop if macOS blocks the helper or local output. Security protections and quarantine remain intact.

The separate **BASSBOXX-macOS-VST3.zip** is still available for music hosts. [Build status](https://github.com/prohoezak-hue/bassboxx/actions/workflows/build.yml) · [Validation](VALIDATION.md)

## Vintage panel and performance controls (1.1.0)

The panel now uses a Model D-inspired charcoal face, cream section markings,
silver-trimmed black knobs and walnut-style cheeks, with BASSBOXX branding.
This is a visual style; the synthesizer is not a measured Model D circuit emulation.
All 21 existing synthesis controls, four oscillator waveforms and 100 factory
patch definitions remain available.

- **POLY / MONO:** switches the existing Voices setting between one voice and your
  previous polyphonic count (2–16). Mono uses last-note priority and legato: release
  the newest key to return to a still-held older key. Glide and sustain work in
  mono. The mode is saved with patches/projects; old Voices=1 patches show Mono.
- **PITCH wheel:** bends all voices by approximately ±2 semitones and springs to
  centre on mouse/key release. Double-click also returns to centre.
- **MOD wheel:** adds free-running 5 Hz pitch vibrato, up to ±0.5 semitone. It holds
  its position on release; double-click returns to zero. MIDI CC1 controls the same
  modulation per MIDI channel; hardware pitch bend remains per-channel too.

The screen wheels are live performance controls, not saved patch parameters.
Factory patch selection keeps its original voice count and updates the mode
switch. With modulation at zero, the existing oscillator/filter/envelope sound
is unchanged. Mono overlap/held-note performance is intentionally improved.

## Standalone installation

1. Download and extract **BASSBOXX-macOS-Standalone.zip**.
2. Open **BASSBOXX.app**. To build/install locally without a paid Apple membership, run **Build BASSBOXX on this Mac.command**; if Apple's tools installer opens, finish it and rerun the helper.
3. Wait for **PASS: standalone app locally compiled, tested and installed**, then open `~/Applications/BASSBOXX.app`.
4. Select **Options > Audio/MIDI Settings**, choose your output and MIDI input, and play at low volume. Physical output/MIDI and security behavior on your own Mac still need testing.

## VST3 installation without a paid Apple membership

1. Quit MPC and extract the **entire** ZIP. Keep all its files together.
2. Double-click **Build BASSBOXX on this Mac.command**. If Apple's free Command Line Tools are missing, it opens Apple's installer: complete that installation, then double-click the builder again. No code editing, Homebrew, paid membership, or Apple signing credentials are needed. Allow several minutes and several GB of free disk space for compilation.
3. Wait for **PASS: locally compiled, tested, installed and loaded**. The helper builds the existing universal instrument, runs the engine/processor/VST3 scan/host tests, backs up an older installation, and installs in `~/Library/Audio/Plug-Ins/VST3/`. Its log location is printed in the window.
4. Restart MPC 3.10. In its plugin manager, enable and rescan that user VST3 folder, then retry BASSBOXX using MPC's own rescan/retry controls if it was previously marked failed. Menu names vary; do not delete MPC settings or projects.
5. Create a plugin track, choose BASSBOXX, enable MIDI monitoring, select your audio device, and start at low volume. Verify scanning, the interface, notes, factory presets, automation and saving/reopening a project.

**If macOS blocks the helper or locally built plugin, stop and keep the exact message.** This workflow does not remove quarantine, disable Gatekeeper/SIP, add trust exceptions, or bypass managed policy. Its bundled CMake tool must pass Apple's assessment before execution. Running the helper through Finder on your particular Mac and loading into MPC still require local verification.

## Why a valid signature can still be rejected

The precompiled VST3 is ad-hoc signed. `codesign` reporting “valid on disk” checks integrity; it does not mean Apple has approved or notarized a download. A quarantined ad-hoc plugin can therefore correctly receive `spctl: rejected`. A free Apple account cannot provide Developer ID distribution signing/notarization. We cannot make that downloaded binary universally trusted with a packaging change or local re-signing.

The no-membership option compiles **new output on your Mac from the included original source**, then signs only that local output for development. This does not change the assessment of the downloaded binary, and `spctl` may still reject the ad-hoc local output as a distribution candidate. The helper checks signature integrity, absence of quarantine on newly compiled output, and actual local VST3 loading. These checks do not override macOS policy or prove MPC compatibility.

**Install BASSBOXX.command** is only a copy/backup installer for a bundle that passes its checks. It stops if a quarantined bundle is rejected, before replacing your current installation. If macOS rejects the precompiled copy, use the local-build route above; manually copying it does not resolve its trust status. Developer ID signing/notarization would require the appropriate paid Apple membership and credentials for public distribution.

## What still needs MPC 3.10 testing

The previously reported scan **Timeout** has not been reproduced by the automated VST3 hosts or validators. Passing those tests does not establish its cause, fix it, or certify MPC 3.10. The host smoke test now logs elapsed time for factory discovery, instantiation, audio, state, editor creation/destruction and release; a 30-second limit makes hangs visible.

Inside MPC on your Mac, check initial scan/retry, live MIDI and audio, all factory programs, pitch bend, sustain, polyphony, Panic, cutoff/Bass Boost automation, custom patch dialogs, project recall, a MIDI-pattern audio export and CPU/latency. Keep the builder log and MPC's exact error if a scan still times out. MPC hardware standalone mode and VST2-only hosts cannot load a desktop VST3.

## Instrument

- Three mixed oscillators: band-limited polyBLEP saw/pulse, triangle and sine; level and ±24-semitone fine/interval tuning per oscillator. Triangle is analytic rather than fully band-limited, so extreme notes can alias.
- Driven two-pole TPT resonant low-pass; cutoff, resonance and envelope modulation. Independent amplitude and filter ADSR contours.
- Independent amplitude/filter attack, decay, sustain, release; exponential frequency glide; 1–16 voices with oldest-voice stealing. Mono mode uses last-note priority with a held-key return stack and legato; glide uses the previous voice frequency.
- Bass Boost adds a 160 Hz low-pass component before soft saturation; output gain and soft limiting bound the output. This is a virtual analog design, not a measured emulation of a particular hardware synth.
- Sample-position MIDI note events, velocity, per-channel sustain and pitch bend, all-notes/all-sound-off, plus on-screen keyboard. Audio is dual mono. No external audio input.
- 100 deterministically generated factory programs: ten variations in each of Foundation, Glide, Acid, Pluck, Sub, Fifth, Atmosphere, Vintage, Lead and Impact. These are starting patches, not a separately auditioned commercial sound library.
- Host automation for all synthesis parameters; host state stores parameter values and factory program index. Save/load custom `.bassboxx` files using the editor buttons. Factory programs remain immutable. Imported custom patches retain their saved program index but the parameter state is authoritative.
- Model D-inspired charcoal procedural panel, wood-grain side cheeks, silver-trimmed black rotary knobs and ProHoeZak branding. No external image assets are required.

## Automated builds and development

GitHub Actions runs on pushes to `main` and the repair branch, pull requests and manual workflow dispatch. It builds universal `arm64;x86_64` macOS standalone and VST3 products, runs engine/processor/editor tests, verifies both binary slices, ad-hoc signs and verifies the bundle, tests installer installation and upgrade, then extracts and verifies the ZIP again. Linux runs the portable DSP regression suite. The Mac job also tests the packaged local builder, first install/upgrade, quarantine and signature rejection, and both external validators on the locally built/installed copy. Both jobs must pass before a main/repair-branch push publishes a versioned development prerelease with the ZIP and checksum. It uses GitHub's built-in workflow token, with write permission scoped to publication; no personal token is needed on the runner.

For maintainers, the build commands are in `.github/workflows/build.yml`; packaging is in `scripts/package-macos.sh`. CMake fetches pinned JUCE 8.0.6 (commit `51a8a6d7aeae7326956d747737ccf1575e61e209`) automatically. `-DJUCE_SOURCE_DIR=/path/to/JUCE` optionally reuses that dependency. On non-macOS platforms CMake builds the shared processor code and test utilities only, without a plugin or standalone target. `-DBASSBOXX_ENGINE_ONLY=ON` builds portable engine tests without JUCE. Neither test utilities nor the audio-demo renderer are shipped as instrument applications.

`Source/Engine.h` implements portable DSP, `Plugin.cpp` handles JUCE MIDI/automation/state, and `Editor.h` implements the controls and custom patch files. The engine, 100 presets and vintage interface are shared unchanged by the standalone app and VST3.

## Distribution and licensing

Project copyright remains with its owner; no public redistribution license for original source is granted here. JUCE is a separate dependency available under AGPLv3 or commercial terms. Before distributing to others, choose and comply with the appropriate [JUCE license](https://juce.com/legal/juce-8-licence/) and bundled notices. Merely compiling or linking does not establish a commercial license. No Apple signing credentials or commercial JUCE license are included.
