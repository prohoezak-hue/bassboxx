#pragma once
#include "Plugin.h"
class VintageLook final : public juce::LookAndFeel_V4 {
public:
  VintageLook() {
    const juce::Colour cream(0xffe8e3cc), black(0xff151715);
    setColour(juce::Slider::textBoxTextColourId, cream);
    setColour(juce::Slider::textBoxBackgroundColourId, black);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, black);
    setColour(juce::ComboBox::textColourId, cream);
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff77796d));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333732));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffa3492c));
    setColour(juce::TextButton::textColourOffId, cream);
    setColour(juce::TextButton::textColourOnId, cream);
  }
  void drawRotarySlider(juce::Graphics &g, int x, int y, int w, int h,
                        float pos, float start, float end, juce::Slider &) override {
    auto area = juce::Rectangle<float>(float(x), float(y), float(w), float(h)).reduced(5);
    const float d = juce::jmin(area.getWidth(), area.getHeight());
    area = area.withSizeKeepingCentre(d, d);
    const auto centre = area.getCentre();
    g.setColour(juce::Colour(0xffd4d1bb));
    for (int tick = 0; tick <= 10; ++tick) {
      const float a = start + (end - start) * float(tick) / 10;
      g.drawLine(centre.x + std::sin(a) * d * .45f, centre.y - std::cos(a) * d * .45f,
                 centre.x + std::sin(a) * d * .5f, centre.y - std::cos(a) * d * .5f, 1);
    }
    auto rim = area.reduced(7);
    g.setColour(juce::Colours::black.withAlpha(.7f));
    g.fillEllipse(rim.translated(2, 3));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffd1d2cc), rim.getX(), rim.getY(),
                                          juce::Colour(0xff5a5e58), rim.getRight(), rim.getBottom(), false));
    g.fillEllipse(rim);
    auto knob = rim.reduced(3);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff343832), knob.getX(), knob.getY(),
                                          juce::Colour(0xff090b09), knob.getRight(), knob.getBottom(), false));
    g.fillEllipse(knob);
    g.setColour(juce::Colour(0xff565a51));
    for (int i = 0; i < 28; ++i) {
      const float a = float(i) * juce::MathConstants<float>::twoPi / 28;
      const float radius = knob.getWidth() * .5f;
      g.drawLine(centre.x + std::sin(a) * radius * .85f, centre.y - std::cos(a) * radius * .85f,
                 centre.x + std::sin(a) * radius, centre.y - std::cos(a) * radius, .6f);
    }
    const float angle = start + pos * (end - start);
    g.setColour(juce::Colour(0xfff4f0d8));
    g.drawLine(centre.x + std::sin(angle) * d * .09f, centre.y - std::cos(angle) * d * .09f,
               centre.x + std::sin(angle) * d * .31f, centre.y - std::cos(angle) * d * .31f, 2.4f);
  }
  void drawLinearSlider(juce::Graphics &g, int x, int y, int w, int h, float position,
                        float, float, juce::Slider::SliderStyle, juce::Slider &slider) override {
    auto trough = juce::Rectangle<float>(float(x + 8), float(y), float(w - 16), float(h));
    g.setColour(juce::Colour(0xff070807));
    g.fillRoundedRectangle(trough, 5);
    g.setColour(juce::Colour(0xff717468));
    g.drawRoundedRectangle(trough, 5, 1);
    auto rubber = trough.reduced(5, 3);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff161916), rubber.getX(), rubber.getY(),
                                          juce::Colour(0xff484c43), rubber.getCentreX(), rubber.getY(), false));
    g.fillRoundedRectangle(rubber, 3);
    for (int i = 0; i < h; i += 7) {
      g.setColour(juce::Colour(0xff090b09));
      g.drawHorizontalLine(y + i, rubber.getX(), rubber.getRight());
      g.setColour(juce::Colour(0xff606459).withAlpha(.45f));
      g.drawHorizontalLine(y + i + 1, rubber.getX(), rubber.getRight());
    }
    const float marker = juce::jlimit(trough.getY() + 4, trough.getBottom() - 4, position);
    g.setColour(juce::Colour(0xffece5cc));
    g.fillRoundedRectangle(rubber.getX(), marker - 2, rubber.getWidth(), 4, 1);
    if (slider.getComponentID() == "pitch-wheel") {
      g.setColour(juce::Colour(0xffce7d43));
      g.drawLine(trough.getX() - 5, trough.getCentreY(), trough.getX(), trough.getCentreY(), 2);
      g.drawLine(trough.getRight(), trough.getCentreY(), trough.getRight() + 5, trough.getCentreY(), 2);
    }
  }
};
class PerformanceWheel final : public juce::Slider {
public:
  explicit PerformanceWheel(bool spring) : juce::Slider(juce::Slider::LinearVertical, juce::Slider::NoTextBox), springBack(spring) {
    setDoubleClickReturnValue(true, 0);
    if (springBack) onDragEnd = [this] { setValue(0, juce::sendNotificationSync); };
  }
  bool keyStateChanged(bool down) override {
    const bool handled = juce::Slider::keyStateChanged(down);
    if (springBack && !down) { setValue(0, juce::sendNotificationSync); return true; }
    return handled;
  }
private:
  bool springBack;
};
class BassboxxEditor final : public juce::AudioProcessorEditor, private juce::Timer {
  BassboxxProcessor &processor;
  VintageLook look;
  juce::MidiKeyboardComponent keys;
  juce::ComboBox presets;
  juce::TextButton save{"Save patch"}, load{"Load patch"}, panic{"Panic"}, mode{"POLY"};
  PerformanceWheel pitchWheel{true}, modWheel{false};
  std::unique_ptr<juce::FileChooser> chooser;
  std::vector<std::unique_ptr<juce::Slider>> knobs;
  std::vector<std::unique_ptr<juce::Label>> labels;
  std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
  std::array<juce::ComboBox, 3> waves;
  std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> waveAttachments;
  void file(bool writing) {
    chooser = std::make_unique<juce::FileChooser>(
        writing ? "Save BASSBOXX patch" : "Load BASSBOXX patch", juce::File{},
        "*.bassboxx");
    chooser->launchAsync(
        (writing ? juce::FileBrowserComponent::saveMode |
                       juce::FileBrowserComponent::warnAboutOverwriting
                 : juce::FileBrowserComponent::openMode) |
            juce::FileBrowserComponent::canSelectFiles,
        [safe = juce::Component::SafePointer<BassboxxEditor>(this),
         writing](const juce::FileChooser &fc) {
          if (!safe)
            return;
          auto f = fc.getResult();
          if (f == juce::File{})
            return;
          if (writing) {
            juce::MemoryBlock block;
            safe->processor.getStateInformation(block);
            f.withFileExtension("bassboxx")
                .replaceWithData(block.getData(), block.getSize());
          } else {
            juce::MemoryBlock block;
            if (f.loadFileAsData(block))
              safe->processor.setStateInformation(block.getData(),
                                                  int(block.getSize()));
          }
        });
  }


  void timerCallback() override {
    presets.setSelectedId(processor.getCurrentProgram() + 1, juce::dontSendNotification);
    const bool mono = processor.isMonophonic();
    const bool changed = mode.getToggleState() != mono;
    mode.setToggleState(mono, juce::dontSendNotification);
    mode.setButtonText(mono ? "MONO" : "POLY");
    if (changed) repaint(903, 520, 146, 20);
    if (!pitchWheel.isMouseButtonDown()) pitchWheel.setValue(processor.getPitchWheel(), juce::dontSendNotification);
    if (!modWheel.isMouseButtonDown()) modWheel.setValue(processor.getModWheel(), juce::dontSendNotification);
  }
public:
  explicit BassboxxEditor(BassboxxProcessor &p)
      : AudioProcessorEditor(p), processor(p), keys(p.keyboard, juce::MidiKeyboardComponent::horizontalKeyboard) {
    setLookAndFeel(&look);
    addAndMakeVisible(keys);
    keys.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xffebe5d2));
    keys.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff121512));
    keys.setAvailableRange(48, 83);
    keys.setLowestVisibleKey(48);
    keys.setScrollButtonsVisible(false);
    keys.setKeyWidth(850.0f / 21);
    addAndMakeVisible(presets);
    for (int i = 0; i < 100; ++i) presets.addItem(p.getProgramName(i), i + 1);
    presets.setSelectedId(p.getCurrentProgram() + 1, juce::dontSendNotification);
    presets.onChange = [this] { processor.setCurrentProgram(presets.getSelectedId() - 1); };
    for (auto *button : {&save, &load, &panic, &mode}) addAndMakeVisible(button);
    save.onClick = [this] { file(true); };
    load.onClick = [this] { file(false); };
    panic.onClick = [this] {
      processor.panicRequested.store(true);
      for (int channel = 1; channel <= 16; ++channel) processor.keyboard.allNotesOff(channel);
    };
    mode.setComponentID("voice-mode");
    mode.setClickingTogglesState(true);
    mode.setTooltip("Mono: one voice, last-note priority and legato. Poly: restores your polyphonic voice count.");
    mode.onClick = [this] { processor.setMonophonic(mode.getToggleState()); repaint(903, 520, 146, 20); };
    pitchWheel.setComponentID("pitch-wheel");
    modWheel.setComponentID("mod-wheel");
    pitchWheel.setName("Pitch bend"); modWheel.setName("Modulation");
    pitchWheel.setRange(-1, 1); modWheel.setRange(0, 1, 1.0 / 127);
    pitchWheel.setTooltip("Pitch bend for all voices: ±2 semitones. Springs back to centre on release.");
    modWheel.setTooltip("Vibrato for all voices: 5 Hz, up to ±0.5 semitone. Stays where you leave it. MIDI CC1 supported.");
    pitchWheel.onValueChange = [this] { processor.setPitchWheel(float(pitchWheel.getValue())); };
    modWheel.onValueChange = [this] { processor.setModWheel(float(modWheel.getValue())); };
    addAndMakeVisible(pitchWheel); addAndMakeVisible(modWheel);
    auto knob = [&](juce::String id, juce::String name) {
      auto slider = std::make_unique<juce::Slider>();
      slider->setComponentID(id);
      slider->setName(name);
      slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
      slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 66, 18);
      addAndMakeVisible(*slider);
      auto label = std::make_unique<juce::Label>();
      label->setText(name, juce::dontSendNotification);
      label->setJustificationType(juce::Justification::centred);
      label->setFont(juce::Font(juce::FontOptions(11.0f)));
      label->setColour(juce::Label::textColourId, juce::Colour(0xffe8e3cc));
      addAndMakeVisible(*label);
      attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state, id, *slider));
      knobs.push_back(std::move(slider)); labels.push_back(std::move(label));
    };
    for (int i = 0; i < 3; ++i) {
      const auto n = juce::String(i + 1);
      knob("level" + n, "OSC " + n + " LEVEL");
      knob("tune" + n, "FREQUENCY");
      addAndMakeVisible(waves[size_t(i)]);
      waves[size_t(i)].addItemList({"Saw", "Pulse", "Triangle", "Sine"}, 1);
      waveAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state, "wave" + n, waves[size_t(i)]));
    }
    for (auto pair : std::initializer_list<std::pair<const char *, const char *>>{
      {"cutoff", "CUTOFF"}, {"resonance", "EMPHASIS"}, {"env", "CONTOUR AMOUNT"},
      {"attack", "ATTACK"}, {"decay", "DECAY"}, {"sustain", "SUSTAIN"}, {"release", "RELEASE"},
      {"glide", "GLIDE"}, {"voices", "VOICES"}, {"boost", "BASS BOOST"}, {"gain", "VOLUME"},
      {"filterAttack", "ATTACK"}, {"filterDecay", "DECAY"}, {"filterSustain", "SUSTAIN"}, {"filterRelease", "RELEASE"}})
      knob(pair.first, pair.second);
    setSize(1100, 800);
    timerCallback();
    startTimerHz(20);
  }
  ~BassboxxEditor() override {
    stopTimer();
    if (pitchWheel.isMouseButtonDown()) processor.setPitchWheel(0);
    setLookAndFeel(nullptr);
  }
  void resized() override {
    presets.setBounds(420, 24, 280, 28);
    save.setBounds(711, 24, 108, 28); load.setBounds(828, 24, 108, 28); panic.setBounds(945, 24, 108, 28);
    const std::array<juce::Rectangle<int>, 21> bounds{{
      {450,150,82,100}, {100,150,82,100}, {450,300,82,100}, {100,300,82,100}, {450,450,82,100}, {100,450,82,100},
      {568,150,92,100}, {672,150,92,100}, {776,150,92,100},
      {566,450,74,100}, {646,450,74,100}, {726,450,74,100}, {806,450,74,100},
      {902,300,72,100}, {980,300,72,100}, {980,150,72,100}, {902,150,72,100},
      {566,300,74,100}, {646,300,74,100}, {726,300,74,100}, {806,300,74,100}}};
    for (size_t i = 0; i < knobs.size(); ++i) {
      knobs[i]->setBounds(bounds[i]);
      labels[i]->setBounds(bounds[i].getX() - 9, bounds[i].getY() - 21, bounds[i].getWidth() + 18, 20);
    }
    for (int i = 0; i < 3; ++i) waves[size_t(i)].setBounds(208, 190 + i * 150, 194, 28);
    mode.setBounds(912, 475, 130, 34);
    pitchWheel.setBounds(48, 648, 55, 130); modWheel.setBounds(118, 648, 55, 130);
    keys.setBounds(202, 645, 850, 135);
  }
  void paint(juce::Graphics &g) override {
    g.fillAll(juce::Colour(0xff694128));
    for (int x = 0; x < getWidth(); ++x) {
      if (x >= 31 && x < getWidth() - 31) continue;
      g.setColour(juce::Colour(0xffc59460).withAlpha(.1f + .1f * std::sin(float(x) * 1.5f)));
      g.drawVerticalLine(x, 0, float(getHeight()));
    }
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2b2f29), 31, 0,
                                          juce::Colour(0xff101310), 1069, 800, false));
    g.fillRect(31, 0, 1038, 800);
    const juce::Colour cream(0xffe8e3cc);
    g.setColour(cream); g.setFont(30);
    g.drawText("BASSBOXX", 49, 17, 300, 37, juce::Justification::centredLeft);
    g.setFont(13);
    g.drawText("by ProHoeZak", 51, 58, 250, 20, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffd48244)); g.fillEllipse(329, 34, 8, 8);
    g.setColour(cream); g.setFont(10);
    g.drawText("POWER", 315, 48, 42, 16, juce::Justification::centred);
    auto section = [&](juce::Rectangle<int> r, const juce::String &name) {
      g.setColour(cream.withAlpha(.65f)); g.drawRoundedRectangle(r.toFloat(), 3, 1);
      g.setColour(juce::Colour(0xff20251f)); g.fillRect(r.getX() + 12, r.getY() - 8, r.getWidth() - 24, 17);
      g.setColour(cream); g.setFont(13);
      g.drawText(name, r.getX() + 8, r.getY() - 10, r.getWidth() - 16, 20, juce::Justification::centred);
    };
    section({44, 107, 378, 478}, "OSCILLATOR BANK");
    section({432, 107, 118, 478}, "MIXER");
    section({560, 107, 326, 478}, "MODIFIERS");
    section({896, 107, 159, 478}, "OUTPUT");
    g.setFont(12); g.setColour(cream);
    for (int i = 0; i < 3; ++i) {
      g.drawText("OSCILLATOR " + juce::String(i + 1), 207, 140 + 150 * i, 194, 22, juce::Justification::centred);
      g.setFont(10); g.drawText("WAVEFORM", 208, 168 + 150 * i, 194, 18, juce::Justification::centred); g.setFont(12);
      g.drawText("-24              0              +24", 78, 250 + 150 * i, 122, 17, juce::Justification::centred);
    }
    g.drawText("FILTER CONTOUR", 570, 260, 305, 16, juce::Justification::centred);
    g.drawText("LOUDNESS CONTOUR", 570, 410, 305, 16, juce::Justification::centred);
    g.drawText("VOICE MODE", 905, 443, 142, 22, juce::Justification::centred);
    g.setFont(10);
    g.drawText(processor.isMonophonic() ? "LAST NOTE / LEGATO" : "UP TO 16 VOICES", 903, 520, 146, 20, juce::Justification::centred);
    g.setColour(cream.withAlpha(.6f)); g.drawRoundedRectangle(42, 612, 145, 177, 3, 1);
    g.setColour(cream); g.setFont(11);
    g.drawText("PITCH", 45, 622, 62, 20, juce::Justification::centred);
    g.drawText("MOD", 115, 622, 62, 20, juce::Justification::centred);
    g.setFont(13);
    g.drawText("THREE OSCILLATOR VIRTUAL ANALOG SYNTHESIZER", 220, 611, 816, 23, juce::Justification::centred);
    for (int x : {38, 1062}) for (int y : {11, 788}) {
      g.setColour(juce::Colour(0xffb7b9ad)); g.fillEllipse(float(x - 3), float(y - 3), 6, 6);
      g.setColour(juce::Colours::black); g.drawLine(float(x - 2), float(y), float(x + 2), float(y), 1);
    }
  }
};
