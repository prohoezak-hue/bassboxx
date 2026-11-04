#include "Plugin.h"
#include "Editor.h"
using namespace juce;
AudioProcessorValueTreeState::ParameterLayout BassboxxProcessor::parameters() {
  AudioProcessorValueTreeState::ParameterLayout p;
  auto add = [&](String id, String name, float lo, float hi, float def,
                 float skew = 1.0f) {
    const int decimals = id == "cutoff"
                             ? 0
                             : (id.containsIgnoreCase("attack") ||
                                        id.containsIgnoreCase("decay") ||
                                        id.containsIgnoreCase("release")
                                    ? 3
                                    : 2);
    p.add(std::make_unique<AudioParameterFloat>(
        ParameterID{id, 1}, name, NormalisableRange<float>(lo, hi, 0, skew),
        def,
        AudioParameterFloatAttributes{}.withStringFromValueFunction(
            [decimals](float value, int) { return String(value, decimals); })));
  };
  for (int i = 0; i < 3; ++i) {
    String n = String(i + 1);
    add("level" + n, "Osc " + n + " Level", 0, 1,
        i == 0   ? .65f
        : i == 1 ? .35f
                 : .2f);
    add("tune" + n, "Osc " + n + " Tune", -24, 24, i == 2 ? -12 : 0);
    p.add(std::make_unique<AudioParameterChoice>(
        ParameterID{"wave" + n, 1}, "Osc " + n + " Wave",
        StringArray{"Saw", "Pulse", "Triangle", "Sine"}, i == 2 ? 1 : 0));
  }
  add("cutoff", "Cutoff", 20, 20000, 1800, .3f);
  add("resonance", "Resonance", 0, 1, .2f);
  add("attack", "Attack", .001f, 5, .008f, .3f);
  add("decay", "Decay", .001f, 5, .25f, .3f);
  add("sustain", "Sustain", 0, 1, .65f);
  add("release", "Release", .001f, 5, .3f, .3f);
  add("env", "Filter Envelope", 0, 1, .5f);
  add("glide", "Glide", 0, 1, 0);
  add("boost", "Bass Boost", 0, 1, 0);
  add("gain", "Output", 0, 1, .65f);
  add("filterAttack", "Filter Attack", .001f, 5, .006f, .3f);
  add("filterDecay", "Filter Decay", .001f, 5, .3f, .3f);
  add("filterSustain", "Filter Sustain", 0, 1, .2f);
  add("filterRelease", "Filter Release", .001f, 5, .25f, .3f);
  p.add(std::make_unique<AudioParameterInt>(ParameterID{"voices", 1},
                                            "Polyphony", 1, 16, 8));
  return p;
}
BassboxxProcessor::BassboxxProcessor()
    : AudioProcessor(BusesProperties().withOutput(
          "Output", AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "BASSBOXX", parameters()) {
  int index = 0;
  for (auto id :
       {"level1",       "tune1",       "wave1",         "level2",
        "tune2",        "wave2",       "level3",        "tune3",
        "wave3",        "cutoff",      "resonance",     "attack",
        "decay",        "sustain",     "release",       "env",
        "glide",        "boost",       "gain",          "voices",
        "filterAttack", "filterDecay", "filterSustain", "filterRelease"})
    values[index++] = state.getRawParameterValue(id);
  setCurrentProgram(0);
  state.addParameterListener("voices", this);
}
BassboxxProcessor::~BassboxxProcessor() { state.removeParameterListener("voices", this); }
bool BassboxxProcessor::isBusesLayoutSupported(const BusesLayout &b) const {
  return b.getMainInputChannelSet().isDisabled() &&
         (b.getMainOutputChannelSet() == AudioChannelSet::stereo() ||
          b.getMainOutputChannelSet() == AudioChannelSet::mono());
}
void BassboxxProcessor::prepareToPlay(double sr, int) {
  engine.prepare(sr);
  keyboard.reset();
  pendingPitch.store(-1); pendingMod.store(-1);
  pitchPosition.store(0); modPosition.store(0);
}
void BassboxxProcessor::setMonophonic(bool mono) {
  auto *parameter = state.getParameter("voices");
  const int current = int(values[19]->load());
  if (mono && current > 1) polyVoices.store(current);
  parameter->beginChangeGesture();
  parameter->setValueNotifyingHost(parameter->convertTo0to1(float(mono ? 1 : jlimit(2, 16, polyVoices.load()))));
  parameter->endChangeGesture();
}
void BassboxxProcessor::setPitchWheel(float value) {
  value = std::isfinite(value) ? jlimit(-1.0f, 1.0f, value) : 0;
  pitchPosition.store(value);
  pendingPitch.store(8192 + roundToInt(value * (value >= 0 ? 8191 : 8192)));
}
void BassboxxProcessor::setModWheel(float value) {
  value = std::isfinite(value) ? jlimit(0.0f, 1.0f, value) : 0;
  modPosition.store(value);
  pendingMod.store(roundToInt(value * 127));
}
void BassboxxProcessor::readPatch() {
  auto get = [&](int i) { return values[size_t(i)]->load(); };
  auto &p = engine.patch;
  for (int i = 0; i < 3; ++i) {
    p.level[i] = get(i * 3);
    p.tune[i] = get(i * 3 + 1);
    p.wave[i] = int(get(i * 3 + 2));
  }
  p.cutoff = get(9);
  p.resonance = get(10);
  p.attack = get(11);
  p.decay = get(12);
  p.sustain = get(13);
  p.release = get(14);
  p.env = get(15);
  p.glide = get(16);
  p.boost = get(17);
  p.gain = get(18);
  p.voices = int(get(19));
  p.filterAttack = get(20);
  p.filterDecay = get(21);
  p.filterSustain = get(22);
  p.filterRelease = get(23);
}
void BassboxxProcessor::midi(const MidiMessage &m) {
  int c = m.getChannel();
  if (m.isNoteOn())
    engine.noteOn(m.getNoteNumber(), m.getFloatVelocity(), c);
  else if (m.isNoteOff())
    engine.noteOff(m.getNoteNumber(), c);
  else if (m.isPitchWheel()) {
    engine.bend((m.getPitchWheelValue() - 8192) / 8192.0f * 2, c);
    const int delta = m.getPitchWheelValue() - 8192;
    pitchPosition.store(delta / float(delta >= 0 ? 8191 : 8192));
  }
  else if (m.isController() && m.getControllerNumber() == 1) {
    engine.modulate(m.getControllerValue() / 127.0f, c);
    modPosition.store(m.getControllerValue() / 127.0f);
  }
  else if (m.isController() && m.getControllerNumber() == 64)
    engine.sustain(m.getControllerValue() >= 64, c);
  else if (m.isAllNotesOff() || m.isAllSoundOff())
    engine.allOff(c);
}
void BassboxxProcessor::processBlock(AudioBuffer<float> &b,
                                     MidiBuffer &events) {
  ScopedNoDenormals noDenormals;
  b.clear();
  keyboard.processNextMidiBuffer(events, 0, b.getNumSamples(), true);
  readPatch();
  if (panicRequested.exchange(false))
    engine.allOff();
  // UI wheels address the whole instrument; incoming MIDI remains per-channel.
  const int pitch = pendingPitch.exchange(-1), modulation = pendingMod.exchange(-1);
  for (int channel = 1; channel <= 16; ++channel) {
    if (pitch >= 0) midi(MidiMessage::pitchWheel(channel, pitch));
    if (modulation >= 0) midi(MidiMessage::controllerEvent(channel, 1, modulation));
  }
  int pos = 0;
  auto render = [&](int end) {
    for (; pos < end; ++pos) {
      float s = engine.sample();
      for (int ch = 0; ch < b.getNumChannels(); ++ch)
        b.setSample(ch, pos, s);
    }
  };
  for (auto metadata : events) {
    render(jlimit(0, b.getNumSamples(), metadata.samplePosition));
    midi(metadata.getMessage());
  }
  render(b.getNumSamples());
  events.clear();
}
const String BassboxxProcessor::getProgramName(int index) {
  index = jlimit(0, bassboxx::programCount - 1, index);
  if (index < bassboxx::signatureProgramCount)
    return bassboxx::userPresets[size_t(index)].name;
  index -= bassboxx::signatureProgramCount;
  static const char *banks[] = {"Foundation", "Glide", "Acid",       "Pluck",
                                "Sub",        "Fifth", "Atmosphere", "Vintage",
                                "Lead",       "Impact"};
  return String(index + bassboxx::signatureProgramCount + 1).paddedLeft('0', 3) + " " +
         banks[jlimit(0, 99, index) / 10] + " " + String(index % 10 + 1);
}
void BassboxxProcessor::setCurrentProgram(int index) {
  const int selected = jlimit(0, bassboxx::programCount - 1, index);
  program = selected;
  auto p = bassboxx::programPatch(selected);
  auto set = [&](String id, float val) {
    auto *param = state.getParameter(id);
    param->setValueNotifyingHost(param->convertTo0to1(val));
  };
  for (int i = 0; i < 3; ++i) {
    String n = String(i + 1);
    set("level" + n, p.level[i]);
    set("tune" + n, p.tune[i]);
    set("wave" + n, float(p.wave[i]));
  }
  set("cutoff", p.cutoff);
  set("resonance", p.resonance);
  set("attack", p.attack);
  set("decay", p.decay);
  set("sustain", p.sustain);
  set("release", p.release);
  set("env", p.env);
  set("glide", p.glide);
  set("boost", p.boost);
  set("gain", p.gain);
  set("voices", float(p.voices));
  set("filterAttack", p.filterAttack);
  set("filterDecay", p.filterDecay);
  set("filterSustain", p.filterSustain);
  set("filterRelease", p.filterRelease);
  if (selected < bassboxx::signatureProgramCount)
    polyVoices.store(bassboxx::userPresets[size_t(selected)].polyVoices);
}
void BassboxxProcessor::getStateInformation(MemoryBlock &dest) {
  auto tree = state.copyState();
  tree.setProperty("program", program.load(), nullptr);
  tree.setProperty("programOrderVersion", 1, nullptr);
  const int count = int(values[19]->load());
  tree.setProperty("polyVoices", count > 1 ? count : polyVoices.load(), nullptr);
  if (auto xml = tree.createXml())
    copyXmlToBinary(*xml, dest);
}
void BassboxxProcessor::setStateInformation(const void *data, int size) {
  if (auto xml = getXmlFromBinary(data, size)) {
    auto tree = ValueTree::fromXml(*xml);
    if (tree.hasType("BASSBOXX")) {
      const int savedProgram = int(tree.getProperty("program", 0));
      program = int(tree.getProperty("programOrderVersion", 0)) == 0
          ? bassboxx::migrateLegacyProgram(savedProgram)
          : jlimit(0, bassboxx::programCount - 1, savedProgram);
      polyVoices.store(jlimit(2, 16, int(tree.getProperty("polyVoices", 8))));
      state.replaceState(tree);
    }
  }
}
AudioProcessorEditor *BassboxxProcessor::createEditor() {
  return new BassboxxEditor(*this);
}
AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new BassboxxProcessor();
}
