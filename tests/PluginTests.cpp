#include "Plugin.h"
#include <cstdlib>
#include <iostream>
static void check(bool ok, const char *msg) {
  if (!ok) {
    std::cerr << "FAIL " << msg << '\n';
    std::exit(1);
  }
}
int main(int argc, char **argv) {
  juce::ScopedJuceInitialiser_GUI gui;
  BassboxxProcessor p;
  p.prepareToPlay(48000, 256);
  juce::AudioBuffer<float> audio(2, 256);
  juce::MidiBuffer midi;
  midi.addEvent(juce::MidiMessage::noteOn(1, 48, 1.0f), 128);
  p.processBlock(audio, midi);
  for (int i = 0; i < 128; ++i)
    check(audio.getSample(0, i) == 0, "sample accurate note start");
  double energy = 0;
  for (int block = 0; block < 100; ++block) {
    p.processBlock(audio, midi);
    for (int i = 0; i < 256; ++i) {
      check(std::isfinite(audio.getSample(0, i)), "finite output");
      check(audio.getSample(0, i) == audio.getSample(1, i), "stereo output");
      energy += std::abs(audio.getSample(0, i));
    }
  }
  check(energy > 1, "processor renders MIDI audio");
  check(p.getNumPrograms() == 100, "100 factory programs");
  for (int i = 0; i < 100; ++i) {
    p.setCurrentProgram(i);
    check(p.getCurrentProgram() == i, "program selection");
  }
  juce::MemoryBlock saved;
  p.getStateInformation(saved);
  p.setCurrentProgram(2);
  p.setStateInformation(saved.getData(), int(saved.getSize()));
  check(p.getCurrentProgram() == 99, "program restored");
  check(p.state.getRawParameterValue("boost")->load() > .7f,
        "parameters restored");
  midi.addEvent(juce::MidiMessage::allSoundOff(1), 0);
  for (int block = 0; block < 100; ++block)
    p.processBlock(audio, midi);
  check(audio.getMagnitude(0, 256) < 1e-6, "MIDI panic");
  p.keyboard.reset();
  p.setCurrentProgram(0);
  std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
  int sliderCount = 0;
  for (auto *child : editor->getChildren()) {
    if (dynamic_cast<juce::Slider *>(child)) {
      ++sliderCount;
      check(!child->getBounds().isEmpty() &&
                editor->getLocalBounds().contains(child->getBounds()),
            "every synthesis knob has visible bounds");
    }
  }
  check(sliderCount == 21, "all synthesis knobs present");
  if (argc > 1) {
    check(editor->getWidth() == 1100 && editor->getHeight() == 800,
          "editor dimensions");
    juce::Image image(juce::Image::RGB, editor->getWidth(), editor->getHeight(),
                      true);
    juce::Graphics graphics(image);
    editor->paintEntireComponent(graphics, true);
    juce::FileOutputStream stream{juce::File(juce::String(argv[1]))};
    check(stream.openedOk(), "snapshot output");
    check(stream.setPosition(0), "snapshot start");
    check(stream.truncate().wasOk(), "replace previous snapshot");
    juce::PNGImageFormat png;
    check(png.writeImageToStream(image, stream), "render editor snapshot");
  }
  std::cout << "PASS: processor MIDI timing, audio, stereo, programs, state "
               "roundtrip, panic\n";
}
