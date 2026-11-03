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
  int sliderCount = 0, wheelCount = 0;
  for (auto *child : editor->getChildren()) {
    if (dynamic_cast<juce::Slider *>(child)) {
      if (child->getComponentID() == "pitch-wheel" || child->getComponentID() == "mod-wheel") ++wheelCount;
      else ++sliderCount;
      check(!child->getBounds().isEmpty() &&
                editor->getLocalBounds().contains(child->getBounds()),
            "every synthesis knob has visible bounds");
    }
  }
  check(sliderCount == 21 && wheelCount == 2, "21 synthesis knobs and both performance wheels present");
  auto* mode = dynamic_cast<juce::TextButton*>(editor->findChildWithID("voice-mode"));
  auto* pitchWheel = dynamic_cast<juce::Slider*>(editor->findChildWithID("pitch-wheel"));
  auto* modWheel = dynamic_cast<juce::Slider*>(editor->findChildWithID("mod-wheel"));
  check(mode && pitchWheel && modWheel, "mode switch and wheels are accessible UI controls");
  auto* voices = p.state.getParameter("voices");
  voices->setValueNotifyingHost(voices->convertTo0to1(12));
  mode->setToggleState(true, juce::dontSendNotification); mode->onClick();
  check(p.isMonophonic() && p.state.getRawParameterValue("voices")->load() == 1, "UI Mono switch sets single voice");
  juce::MemoryBlock monoState; p.getStateInformation(monoState);
  mode->setToggleState(false, juce::dontSendNotification); mode->onClick();
  check(p.state.getRawParameterValue("voices")->load() == 12, "UI Poly switch restores chosen voice count");
  p.setStateInformation(monoState.getData(), int(monoState.getSize()));
  check(p.isMonophonic(), "mono state persists in patch/project");
  p.setMonophonic(false);
  check(p.state.getRawParameterValue("voices")->load() == 12, "saved mono state remembers poly voice count");
  p.setCurrentProgram(0);
  p.prepareToPlay(48000, 256);
  BassboxxProcessor hardware, neutral;
  hardware.prepareToPlay(48000, 256); neutral.prepareToPlay(48000, 256);
  pitchWheel->setValue(1, juce::sendNotificationSync);
  modWheel->setValue(1, juce::sendNotificationSync);
  juce::AudioBuffer<float> hardwareAudio(2, 256), neutralAudio(2, 256);
  juce::MidiBuffer hardwareMidi, neutralMidi;
  midi.clear();
  midi.addEvent(juce::MidiMessage::noteOn(1, 57, 1.0f), 0);
  hardwareMidi.addEvent(juce::MidiMessage::pitchWheel(1, 16383), 0);
  hardwareMidi.addEvent(juce::MidiMessage::controllerEvent(1, 1, 127), 0);
  hardwareMidi.addEvent(juce::MidiMessage::noteOn(1, 57, 1.0f), 0);
  neutralMidi.addEvent(juce::MidiMessage::noteOn(1, 57, 1.0f), 0);
  double wheelDifference = 0;
  for (int block = 0; block < 100; ++block) {
    p.processBlock(audio, midi); hardware.processBlock(hardwareAudio, hardwareMidi); neutral.processBlock(neutralAudio, neutralMidi);
    for (int i = 0; i < 256; ++i) {
      check(audio.getSample(0, i) == hardwareAudio.getSample(0, i), "screen wheels match actual MIDI pitch bend and CC1 audio");
      wheelDifference += std::abs(audio.getSample(0, i) - neutralAudio.getSample(0, i));
    }
  }
  check(wheelDifference > 1, "wheel movement changes rendered sound");
  check(p.getPitchWheel() > .99f && p.getModWheel() == 1, "hardware and screen controller positions synchronized");
  check(bool(pitchWheel->onDragEnd), "pitch wheel has release behavior");
  pitchWheel->onDragEnd();
  p.processBlock(audio, midi);
  check(p.getPitchWheel() == 0 && pitchWheel->getValue() == 0, "pitch wheel springs to centre after release");
  check(p.getModWheel() == 1 && modWheel->getValue() == 1, "modulation wheel holds its position");
  modWheel->setValue(0, juce::sendNotificationSync);
  p.processBlock(audio, midi);
  midi.addEvent(juce::MidiMessage::allSoundOff(1), 0); p.processBlock(audio, midi);
  p.setCurrentProgram(0);
  mode->setToggleState(false, juce::dontSendNotification);
  mode->setButtonText("POLY");
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
