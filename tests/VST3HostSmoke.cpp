#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>
#include <iostream>
#include <memory>

int main(int argc, char** argv)
{
    if (argc != 2) { std::cerr << "Usage: vst3_host_smoke /path/to/BASSBOXX.vst3\n"; return 2; }
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String bundle = juce::String::fromUTF8(argv[1]);
    if (!juce::File(bundle).isDirectory()) { std::cerr << "VST3 bundle missing\n"; return 2; }

    juce::VST3PluginFormat format;
    juce::OwnedArray<juce::PluginDescription> descriptions;
    format.findAllTypesForFile(descriptions, bundle);
    if (descriptions.isEmpty()) { std::cerr << "FAIL: VST3 factory not discoverable\n"; return 1; }
    for (auto* description : descriptions)
    {
        if (!description->isInstrument || !description->name.containsIgnoreCase("BASSBOXX"))
            continue;
        juce::String error;
        auto plugin = format.createInstanceFromDescription(*description, 48000.0, 256, error);
        if (!plugin) { std::cerr << "FAIL: VST3 instance: " << error << "\n"; return 1; }
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
        if (energy < 0.01) { std::cerr << "FAIL: VST3 produced silence\n"; return 1; }
        juce::MemoryBlock state;
        plugin->getStateInformation(state);
        if (state.isEmpty()) { std::cerr << "FAIL: empty VST3 state\n"; return 1; }
        plugin->setStateInformation(state.getData(), (int)state.getSize());
        if (!plugin->hasEditor()) { std::cerr << "FAIL: editor missing\n"; return 1; }
        auto editor = std::unique_ptr<juce::AudioProcessorEditor>(plugin->createEditorIfNeeded());
        if (!editor || editor->getWidth() <= 0) { std::cerr << "FAIL: editor initialization\n"; return 1; }
        editor.reset();
        plugin->releaseResources();
        std::cout << "PASS: real VST3 bundle discovery, instantiation, MIDI audio, state and editor\n";
        return 0;
    }
    std::cerr << "FAIL: BASSBOXX VST3 instrument not registered\n";
    return 1;
}
