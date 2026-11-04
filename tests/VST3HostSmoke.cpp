#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>
#include <iostream>
#include <memory>
#include <chrono>

int main(int argc, char** argv)
{
    const bool scanOnly = argc == 3 && juce::String(argv[2]) == "--scan-only";
    if (argc != 2 && !scanOnly) { std::cerr << "Usage: vst3_host_smoke /path/to/TheFunkBoxx.vst3 [--scan-only]\n"; return 2; }
    const auto start = std::chrono::steady_clock::now();
    auto phase = [&](const char* name) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        std::cout << "plugin load check [" << elapsed << " ms]: " << name << std::endl;
    };
    phase("begin host GUI initialisation");
    juce::ScopedJuceInitialiser_GUI gui;
    phase("host GUI initialisation returned");
    const juce::String bundle = juce::String::fromUTF8(argv[1]);
    if (!juce::File(bundle).isDirectory()) { std::cerr << "plugin bundle missing\n"; return 2; }

   #if BASSBOXX_HOST_AU
    juce::AudioUnitPluginFormat format;
    const juce::String identifier = "AudioUnit:Synths/aumu,FnBx,PhZk";
   #else
    juce::VST3PluginFormat format;
    const juce::String identifier = bundle;
   #endif
    juce::OwnedArray<juce::PluginDescription> descriptions;
    phase("begin factory discovery");
    format.findAllTypesForFile(descriptions, identifier);
    phase("factory discovery returned");
    if (descriptions.isEmpty()) { std::cerr << "FAIL: plugin factory not discoverable\n"; return 1; }
    for (auto* description : descriptions)
    {
        if (!description->isInstrument || !description->name.containsIgnoreCase("TheFunkBoxx"))
            continue;
        if (scanOnly) {
            phase("PASS: instrument metadata scan completed without requesting an editor");
            return 0;
        }
        juce::String error;
        phase("begin instantiation");
        auto plugin = format.createInstanceFromDescription(*description, 48000.0, 256, error);
        phase("instantiation returned");
        if (!plugin) { std::cerr << "FAIL: plugin instance: " << error << "\n"; return 1; }
        plugin->setPlayConfigDetails(0, 2, 48000.0, 256);
        plugin->prepareToPlay(48000.0, 256);
        juce::AudioBuffer<float> audio(2, 256);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 48, (juce::uint8) 110), 0);
        double energy = 0.0;
        for (int block = 0; block < 32; ++block)
        {
            audio.clear();
            plugin->processBlock(audio, midi);
            midi.clear();
            for (int ch = 0; ch < audio.getNumChannels(); ++ch)
                for (int i = 0; i < audio.getNumSamples(); ++i)
                {
                    const float s = audio.getSample(ch, i);
                    if (!std::isfinite(s)) { std::cerr << "FAIL: non-finite audio\n"; return 1; }
                    energy += std::abs(s);
                }
        }
        if (energy < 0.01) { std::cerr << "FAIL: plugin produced silence\n"; return 1; }
        phase("MIDI audio rendered");
        juce::MemoryBlock state;
        plugin->getStateInformation(state);
        if (state.isEmpty()) { std::cerr << "FAIL: empty plugin state\n"; return 1; }
        plugin->setStateInformation(state.getData(), (int)state.getSize());
        phase("state restored; begin editor creation");
        if (!plugin->hasEditor()) { std::cerr << "FAIL: editor missing\n"; return 1; }
        auto editor = std::unique_ptr<juce::AudioProcessorEditor>(plugin->createEditorIfNeeded());
        if (!editor || editor->getWidth() <= 0) { std::cerr << "FAIL: editor initialization\n"; return 1; }
        editor.reset();
        phase("editor created and destroyed");
        plugin->releaseResources();
        plugin.reset();
        phase("instance released");
        std::cout << "PASS: real plugin bundle discovery, instantiation, MIDI audio, state and editor\n";
        return 0;
    }
    std::cerr << "FAIL: TheFunkBoxx plugin instrument not registered\n";
    return 1;
}
