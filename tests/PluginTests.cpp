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
  check(p.getName() == "TheFunkBoxx", "new product name");
  check(p.getCurrentProgram() == 0 && p.getProgramName(0) == "01 - Rap (Bass)",
        "signature bank is first and is the fresh default");
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
  check(p.getNumPrograms() == 125, "100 factory and 25 ProHoeZak programs");
  for (int i = 0; i < p.getNumPrograms(); ++i) {
    p.setCurrentProgram(i);
    check(p.getCurrentProgram() == i, "program selection");
    if (i < bassboxx::signatureProgramCount) {
      const auto presetFile = juce::File(__FILE__).getParentDirectory()
          .getParentDirectory().getChildFile("Presets/ProHoeZak")
          .getChildFile(p.getProgramName(i) + ".bassboxx");
      juce::MemoryBlock original;
      check(presetFile.loadFileAsData(original), "original user preset exists");
      auto xml = juce::AudioProcessor::getXmlFromBinary(
          original.getData(), int(original.getSize()));
      check(xml != nullptr, "original user preset decodes");
      auto tree = juce::ValueTree::fromXml(*xml);
      check(tree.getNumChildren() == 24, "complete original user preset");
      juce::MemoryBlock selected;
      p.getStateInformation(selected);
      auto selectedXml = juce::AudioProcessor::getXmlFromBinary(
          selected.getData(), int(selected.getSize()));
      check(selectedXml != nullptr, "selected preset state decodes");
      check(selectedXml->getIntAttribute("polyVoices") == int(tree["polyVoices"]),
            "user preset retains its remembered polyphony");
      for (int child = 0; child < tree.getNumChildren(); ++child) {
        auto parameter = tree.getChild(child);
        auto *actual = p.state.getRawParameterValue(parameter["id"].toString());
        check(actual != nullptr, "user preset parameter exists");
        const float expected = float(parameter["value"]);
        check(std::abs(actual->load() - expected) <= std::max(1e-5f, std::abs(expected) * 1e-6f),
              "user preset retains its original parameter values");
      }
    }
  }
  juce::MemoryBlock saved;
  p.getStateInformation(saved);
  p.setCurrentProgram(2);
  p.setStateInformation(saved.getData(), int(saved.getSize()));
  check(p.getCurrentProgram() == 124, "program restored");
  check(std::abs(p.state.getRawParameterValue("boost")->load() -
                     bassboxx::factory(99).boost) < 1e-6f,
        "parameters restored");
  check(p.getProgramName(24) == "025 - Zapp (Bass)" &&
        p.getProgramName(25) == "026 Foundation 1" &&
        p.getProgramName(124) == "125 Impact 10", "signature bank precedes all factory sounds");
  // All 125 old program indices migrate without changing saved sound settings.
  for (int old = 0; old < 125; ++old) {
    const int current = bassboxx::migrateLegacyProgram(old);
    p.setCurrentProgram(current);
    p.getStateInformation(saved);
    auto xml = juce::AudioProcessor::getXmlFromBinary(saved.getData(), int(saved.getSize()));
    xml->removeAttribute("programOrderVersion");
    xml->setAttribute("program", old);
    juce::MemoryBlock legacy;
    juce::AudioProcessor::copyXmlToBinary(*xml, legacy);
    const float expectedCutoff = p.state.getRawParameterValue("cutoff")->load();
    p.setCurrentProgram((current + 1) % 125);
    p.setStateInformation(legacy.getData(), int(legacy.getSize()));
    check(p.getCurrentProgram() == current, "legacy project restores the same named program");
    // APVTS normalises/restores floats; allow its normal rounding precision.
    check(std::abs(p.state.getRawParameterValue("cutoff")->load() - expectedCutoff)
              <= std::max(1e-5f, std::abs(expectedCutoff) * 1e-6f),
          "legacy project restores its saved sound parameters");
    p.getStateInformation(saved);
    p.setCurrentProgram((current + 1) % 125);
    p.setStateInformation(saved.getData(), int(saved.getSize()));
    check(p.getCurrentProgram() == current, "resaved project does not migrate twice");
  }
  // New signature-first states roundtrip independently at every program index.
  for (int i = 0; i < 125; ++i) {
    p.setCurrentProgram(i);
    p.getStateInformation(saved);
    p.setCurrentProgram((i + 1) % 125);
    p.setStateInformation(saved.getData(), int(saved.getSize()));
    check(p.getCurrentProgram() == i, "signature-first program state roundtrip");
  }
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
  auto* previous = dynamic_cast<juce::TextButton*>(editor->findChildWithID("preset-previous"));
  auto* next = dynamic_cast<juce::TextButton*>(editor->findChildWithID("preset-next"));
  auto* menu = dynamic_cast<juce::ComboBox*>(editor->findChildWithID("sound-menu"));
  check(previous && next && menu, "sound browsing arrows and dropdown present");
  check(previous->getRight() <= menu->getX() && next->getRight() <= menu->getX()
        && previous->getBottom() <= next->getY(), "up/down arrows immediately precede dropdown");
  for (auto* control : {static_cast<juce::Component*>(previous), static_cast<juce::Component*>(next), static_cast<juce::Component*>(menu)})
    check(control->isVisible() && editor->getLocalBounds().contains(control->getBounds()), "sound browser controls visible");
  previous->onClick();
  check(p.getCurrentProgram() == 124 && menu->getSelectedId() == 125, "up arrow wraps first sound to last and updates dropdown");
  next->onClick();
  check(p.getCurrentProgram() == 0 && menu->getSelectedId() == 1, "down arrow wraps last sound to first");
  for (int i = 1; i <= p.getNumPrograms(); ++i) {
    next->onClick();
    check(p.getCurrentProgram() == i % p.getNumPrograms() && menu->getSelectedId() == i % p.getNumPrograms() + 1, "arrows browse every sound in order");
  }
  menu->setSelectedId(43, juce::sendNotificationSync);
  check(p.getCurrentProgram() == 42, "dropdown still selects sounds directly");
  BassboxxProcessor expectedProgram; expectedProgram.setCurrentProgram(42);
  for (auto* parameter : p.getParameters())
    check(parameter->getValue() == expectedProgram.getParameters()[parameter->getParameterIndex()]->getValue(), "browser loads complete sound parameters");
  previous->onClick();
  check(p.getCurrentProgram() == 41 && menu->getSelectedId() == 42, "arrows continue from dropdown selection");
  p.setCurrentProgram(77);
  next->onClick();
  check(p.getCurrentProgram() == 78 && menu->getSelectedId() == 79, "arrows follow host program changes without stale selection");
  juce::MemoryBlock browsedSound; p.getStateInformation(browsedSound);
  previous->onClick(); p.setStateInformation(browsedSound.getData(), int(browsedSound.getSize()));
  previous->onClick();
  check(p.getCurrentProgram() == 77 && menu->getSelectedId() == 78, "arrows follow restored sound state");
  p.setCurrentProgram(0); next->onClick(); previous->onClick();
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
  mode->setToggleState(p.isMonophonic(), juce::dontSendNotification);
  mode->setButtonText(p.isMonophonic() ? "MONO" : "POLY");
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
