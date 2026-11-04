#include "Plugin.h"
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#include <iostream>

// The existing processor and editor run in JUCE's standard audio/MIDI window.
// No standalone-specific changes are made to synthesis, presets or parameter IDs.
class BassboxxStandaloneApp final : public juce::JUCEApplication {
public:
  const juce::String getApplicationName() override { return "TheFunkBoxx"; }
  const juce::String getApplicationVersion() override { return JucePlugin_VersionString; }
  bool moreThanOneInstanceAllowed() override { return false; }
  void anotherInstanceStarted(const juce::String&) override {
    if (window) window->toFront(true);
  }

  void initialise(const juce::String& commandLine) override {
    const auto args = juce::StringArray::fromTokens(commandLine, true);
    selfTest = args.size() == 2 && args[0] == "--self-test";
    if (selfTest) report = juce::File(args[1].unquoted());
    juce::PropertiesFile::Options options;
    options.applicationName = "BASSBOXX";
    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";
    properties.setStorageParameters(options);
    window = std::make_unique<juce::StandaloneFilterWindow>(
        getApplicationName(), juce::Colour(0xff331b10),
        selfTest ? nullptr : properties.getUserSettings(), false);
    window->setVisible(true);
    if (selfTest) {
      // Exercise the actual application event loop/window, without saving test
      // notes, settings, or patches into the user's normal application state.
      juce::Timer::callAfterDelay(250, [this] {
        const bool ok = checkStandalone();
        setApplicationReturnValue(ok ? 0 : 1);
        quit();
      });
    }
  }

  void systemRequestedQuit() override {
    if (window && !selfTest) window->pluginHolder->savePluginState();
    if (juce::ModalComponentManager::getInstance()->cancelAllModalComponents()) {
      juce::Timer::callAfterDelay(100, [this] { systemRequestedQuit(); });
    } else {
      quit();
    }
  }
  void shutdown() override {
    window.reset();
    if (!selfTest) properties.saveIfNeeded();
  }

private:
  bool checkStandalone() {
    auto* processor = window->getAudioProcessor();
    auto* editor = processor->getActiveEditor();
    if (!window->isShowing() || window->getPeer() == nullptr || !editor ||
        editor->getWidth() != 1100 || editor->getHeight() != 800 ||
        processor->getNumPrograms() != bassboxx::programCount || processor->getTotalNumInputChannels() != 0) {
      std::cerr << "FAIL: standalone window, original editor, presets or output-only setup\n";
      return false;
    }
    // Detach the device callback before deterministic offline audio checks.
    window->pluginHolder->stopPlaying();
    window->getDeviceManager().closeAudioDevice();
    processor->prepareToPlay(48000, 256);
    processor->setCurrentProgram(0);
    juce::AudioBuffer<float> audio(2, 256);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 48, (juce::uint8)110), 0);
    double energy = 0;
    for (int block = 0; block < 32; ++block) {
      processor->processBlock(audio, midi);
      midi.clear();
      for (int i = 0; i < audio.getNumSamples(); ++i) {
        const float value = audio.getSample(0, i);
        if (!std::isfinite(value) || value != audio.getSample(1, i)) return false;
        energy += std::abs(value);
      }
    }
    processor->releaseResources();
    if (energy <= 1) { std::cerr << "FAIL: standalone MIDI audio was silent\n"; return false; }
    juce::DynamicObject::Ptr result = new juce::DynamicObject;
    result->setProperty("status", "passed");
    result->setProperty("programs", processor->getNumPrograms());
    result->setProperty("editorWidth", editor->getWidth());
    result->setProperty("editorHeight", editor->getHeight());
    result->setProperty("audioEnergy", energy);
    result->setProperty("windowVisible", window->isShowing());
    if (!report.replaceWithText(juce::JSON::toString(juce::var(result.get())))) return false;
    std::cout << "PASS: standalone application window, original editor, 125 presets and MIDI stereo audio\n";
    return true;
  }
  juce::ApplicationProperties properties;
  std::unique_ptr<juce::StandaloneFilterWindow> window;
  bool selfTest = false;
  juce::File report;
};

// JUCE's standalone wrapper supplies main(); only its application is replaced.
JUCE_CREATE_APPLICATION_DEFINE(BassboxxStandaloneApp)
