#pragma once
#include "Plugin.h"
// All artwork is rendered locally: no downloaded images or runtime resources.
class VintageLook final : public juce::LookAndFeel_V4 {
public:
  static juce::Colour cream() { return juce::Colour(0xffffe4ac); }
  static juce::Colour gold() { return juce::Colour(0xffbc8847); }
  static juce::Font lettering(float height, bool bold = true) {
    return juce::Font(juce::FontOptions(juce::Font::getDefaultSerifFontName(), height,
                                      bold ? juce::Font::bold : juce::Font::plain));
  }
  VintageLook() {
    const juce::Colour black(0xff12100d);
    setColour(juce::Slider::textBoxTextColourId, cream());
    setColour(juce::Slider::textBoxBackgroundColourId, black);
    setColour(juce::Slider::textBoxOutlineColourId, gold());
    setColour(juce::ComboBox::backgroundColourId, black);
    setColour(juce::ComboBox::textColourId, cream());
    setColour(juce::ComboBox::outlineColourId, gold());
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff29231b));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff674021));
    setColour(juce::TextButton::textColourOffId, cream());
    setColour(juce::TextButton::textColourOnId, cream());
    setColour(juce::PopupMenu::backgroundColourId, black);
    setColour(juce::PopupMenu::textColourId, cream());
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff614626));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
  }
  static void bezel(juce::Graphics &g, juce::Rectangle<float> r, float radius = 4) {
    g.setColour(juce::Colours::black.withAlpha(.75f));
    g.drawRoundedRectangle(r.translated(0, 2), radius, 3);
    g.setColour(gold()); g.drawRoundedRectangle(r, radius, 1.3f);
    g.setColour(cream().withAlpha(.3f));
    g.drawRoundedRectangle(r.reduced(1.5f), radius - 1, .6f);
  }
  void drawButtonBackground(juce::Graphics &g, juce::Button &button, const juce::Colour &,
                            bool hover, bool down) override {
    auto r = button.getLocalBounds().toFloat().reduced(1.5f);
    const bool active = down || button.getToggleState();
    g.setGradientFill(juce::ColourGradient(juce::Colour(active ? 0xff664023 : 0xff383027), 0, 0,
                      juce::Colour(0xff100e0b), 0, r.getBottom(), false));
    g.fillRoundedRectangle(r, 4);
    bezel(g, r);
    if (hover || button.hasKeyboardFocus(true)) {
      g.setColour(cream().withAlpha(.7f)); g.drawRoundedRectangle(r.reduced(2), 2, 1);
    }
  }
  juce::Font getTextButtonFont(juce::TextButton &, int height) override {
    return lettering(juce::jmin(17.0f, float(height) * .65f));
  }
  void drawButtonText(juce::Graphics &g, juce::TextButton &button, bool, bool down) override {
    const auto id = button.getComponentID();
    if (id == "preset-previous" || id == "preset-next") {
      const float x = float(button.getWidth()) / 2, y = float(button.getHeight()) / 2 + (down ? 1 : 0);
      const float direction = id == "preset-previous" ? -1.0f : 1.0f;
      juce::Path arrow;
      arrow.startNewSubPath(x - 4, y - direction * 2);
      arrow.lineTo(x, y + direction * 2); arrow.lineTo(x + 4, y - direction * 2);
      g.setColour(cream()); g.strokePath(arrow, juce::PathStrokeType(1.8f));
      return;
    }
    g.setColour(cream()); g.setFont(getTextButtonFont(button, button.getHeight()));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(5, 1), juce::Justification::centred, 1);
  }
  void drawComboBox(juce::Graphics &g, int w, int h, bool, int, int, int, int, juce::ComboBox &box) override {
    auto r = juce::Rectangle<float>(1.5f, 1.5f, float(w) - 3, float(h) - 3);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff26221b), 0, 0,
                      juce::Colour(0xff090807), 0, float(h), false));
    g.fillRoundedRectangle(r, 4); bezel(g, r);
    juce::Path arrow; arrow.startNewSubPath(float(w - 25), float(h) * .43f);
    arrow.lineTo(float(w - 19), float(h) * .63f); arrow.lineTo(float(w - 13), float(h) * .43f);
    g.setColour(box.isEnabled() ? cream() : gold()); g.strokePath(arrow, juce::PathStrokeType(2));
  }
  juce::Font getComboBoxFont(juce::ComboBox &) override { return lettering(16); }
  void positionComboBoxText(juce::ComboBox &box, juce::Label &label) override {
    label.setBounds(9, 1, box.getWidth() - 41, box.getHeight() - 2);
    label.setFont(getComboBoxFont(box));
  }
  void drawRotarySlider(juce::Graphics &g, int x, int y, int w, int h,
                        float pos, float start, float end, juce::Slider &) override {
    auto area = juce::Rectangle<float>(float(x), float(y), float(w), float(h)).reduced(4);
    const float d = juce::jmin(area.getWidth(), area.getHeight());
    area = area.withSizeKeepingCentre(d, d);
    const auto centre = area.getCentre();
    g.setColour(cream());
    for (int tick = 0; tick <= 10; ++tick) {
      const float a = start + (end - start) * float(tick) / 10;
      g.drawLine(centre.x + std::sin(a) * d * .45f, centre.y - std::cos(a) * d * .45f,
                 centre.x + std::sin(a) * d * .5f, centre.y - std::cos(a) * d * .5f, 1.2f);
    }
    auto rim = area.reduced(7);
    g.setColour(juce::Colours::black.withAlpha(.8f)); g.fillEllipse(rim.translated(1, 3));
    juce::ColourGradient brass(cream(), rim.getX(), rim.getY(), juce::Colour(0xff5c4327), rim.getRight(), rim.getBottom(), false);
    brass.addColour(.4, gold()); brass.addColour(.55, juce::Colour(0xfff3d39a));
    g.setGradientFill(brass); g.fillEllipse(rim);
    auto knob = rim.reduced(3);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff38332b), knob.getX(), knob.getY(),
                                          juce::Colour(0xff070605), knob.getRight(), knob.getBottom(), false));
    g.fillEllipse(knob);
    const float radius = knob.getWidth() * .5f;
    for (int i = 0; i < 64; ++i) {
      const float a = float(i) * juce::MathConstants<float>::twoPi / 64;
      g.setColour(juce::Colour(i % 2 ? 0xff3b3429 : 0xff12100c));
      g.drawLine(centre.x + std::sin(a) * radius * .18f, centre.y - std::cos(a) * radius * .18f,
                 centre.x + std::sin(a) * radius * .96f, centre.y - std::cos(a) * radius * .96f, .65f);
    }
    const float angle = start + pos * (end - start);
    g.setColour(cream());
    g.drawLine(centre.x + std::sin(angle) * d * .07f, centre.y - std::cos(angle) * d * .07f,
               centre.x + std::sin(angle) * d * .31f, centre.y - std::cos(angle) * d * .31f, 2.7f);
  }
  void drawLinearSlider(juce::Graphics &g, int x, int y, int w, int h, float position,
                        float, float, juce::Slider::SliderStyle, juce::Slider &slider) override {
    auto trough = juce::Rectangle<float>(float(x + 6), float(y), float(w - 12), float(h));
    g.setColour(juce::Colour(0xff070605)); g.fillRoundedRectangle(trough, 6); bezel(g, trough, 6);
    auto rubber = trough.reduced(5, 3);
    juce::ColourGradient gradient(juce::Colour(0xff090807), rubber.getX(), 0, juce::Colour(0xff090807), rubber.getRight(), 0, false);
    gradient.addColour(.45, juce::Colour(0xff655c4e)); g.setGradientFill(gradient); g.fillRoundedRectangle(rubber, 3);
    for (int i = 4; i < h - 3; i += 6) {
      g.setColour(juce::Colour(0xff070605)); g.drawHorizontalLine(y + i, rubber.getX(), rubber.getRight());
      g.setColour(juce::Colour(0xffbba37b).withAlpha(.3f)); g.drawHorizontalLine(y + i + 1, rubber.getX(), rubber.getRight());
    }
    const float marker = juce::jlimit(trough.getY() + 4, trough.getBottom() - 4, position);
    g.setColour(cream()); g.fillRoundedRectangle(rubber.getX(), marker - 2, rubber.getWidth(), 4, 1);
    if (slider.getComponentID() == "pitch-wheel") {
      g.setColour(juce::Colour(0xffff9b22));
      g.drawLine(trough.getX() - 5, trough.getCentreY(), trough.getX(), trough.getCentreY(), 3);
      g.drawLine(trough.getRight(), trough.getCentreY(), trough.getRight() + 5, trough.getCentreY(), 3);
    }
  }
};
class VintageKeyboard final : public juce::MidiKeyboardComponent {
public:
  explicit VintageKeyboard(juce::MidiKeyboardState &state) : MidiKeyboardComponent(state, horizontalKeyboard) {}
  void drawWhiteNote(int note, juce::Graphics &g, juce::Rectangle<float> r, bool down, bool over, juce::Colour, juce::Colour) override {
    juce::ColourGradient ivory(juce::Colour(0xfff9e5bf), r.getX(), r.getY(), juce::Colour(0xffd7bd90), r.getRight(), r.getBottom(), false);
    if (down) ivory = juce::ColourGradient(juce::Colour(0xffe3cf19), r.getX(), r.getY(), juce::Colour(0xffbbaa0b), r.getRight(), r.getBottom(), false);
    g.setGradientFill(ivory); g.fillRect(r.reduced(.5f, 1));
    g.setColour(juce::Colour(0xff493a25)); g.drawRect(r, .8f);
    if (over && !down) { g.setColour(juce::Colours::white.withAlpha(.16f)); g.fillRect(r); }
    if (note % 12 == 0) {
      g.setColour(juce::Colour(0xff211b12)); g.setFont(11);
      g.drawText(getWhiteNoteText(note), r.toNearestInt().removeFromBottom(17), juce::Justification::centred);
    }
  }
  void drawBlackNote(int, juce::Graphics &g, juce::Rectangle<float> r, bool down, bool over, juce::Colour) override {
    g.setColour(juce::Colours::black); g.fillRect(r);
    auto top = r.reduced(3, 1).withTrimmedBottom(9);
    g.setGradientFill(juce::ColourGradient(juce::Colour(down ? 0xff74643e : 0xff4b4438), top.getX(), top.getY(),
                                        juce::Colour(0xff100e0b), top.getRight(), top.getBottom(), false));
    g.fillRect(top); g.setColour(juce::Colour(0xff8f7a53).withAlpha(over ? .8f : .4f)); g.drawRect(top, .8f);
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
  VintageKeyboard keys;
  juce::Image woodPanel;
  juce::ComboBox presets;
  juce::TextButton previousSound, nextSound;
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
      : AudioProcessorEditor(p), processor(p), keys(p.keyboard) {
    setLookAndFeel(&look);
    addAndMakeVisible(keys);
    keys.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xffebe5d2));
    keys.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff121512));
    keys.setAvailableRange(48, 83);
    keys.setLowestVisibleKey(48);
    keys.setScrollButtonsVisible(false);
    keys.setKeyWidth(850.0f / 21);
    presets.setComponentID("sound-menu");
    presets.setName("Factory sound");
    addAndMakeVisible(presets);
    previousSound.setComponentID("preset-previous"); nextSound.setComponentID("preset-next");
    previousSound.setName("Previous sound"); nextSound.setName("Next sound");
    previousSound.setTooltip("Previous sound (wraps from the first to the last).");
    nextSound.setTooltip("Next sound (wraps from the last to the first).");
    previousSound.setRepeatSpeed(400, 120); nextSound.setRepeatSpeed(400, 120);
    auto stepSound = [this](int step) {
      const int count = processor.getNumPrograms();
      if (count <= 0) return;
      processor.setCurrentProgram((processor.getCurrentProgram() + step + count) % count);
      timerCallback();
    };
    previousSound.onClick = [stepSound] { stepSound(-1); };
    nextSound.onClick = [stepSound] { stepSound(1); };
    addAndMakeVisible(previousSound); addAndMakeVisible(nextSound);
    for (int i = 0; i < 100; ++i) presets.addItem(p.getProgramName(i), i + 1);
    presets.setSelectedId(p.getCurrentProgram() + 1, juce::dontSendNotification);
    presets.onChange = [this] {
      if (presets.getSelectedId() > 0) processor.setCurrentProgram(presets.getSelectedId() - 1);
      timerCallback();
    };
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
      addAndMakeVisible(*slider);
      // Create the editable value label after inheriting the panel look and feel.
      slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 66, 18);
      auto label = std::make_unique<juce::Label>();
      label->setText(name, juce::dontSendNotification);
      label->setJustificationType(juce::Justification::centred);
      label->setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
      label->setColour(juce::Label::textColourId, VintageLook::cream());
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
    createWoodPanel();
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
    previousSound.setBounds(389, 24, 26, 15); nextSound.setBounds(389, 40, 26, 15);
    presets.setBounds(420, 24, 280, 31);
    save.setBounds(711, 24, 108, 31); load.setBounds(828, 24, 108, 31); panic.setBounds(945, 24, 108, 31);
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
  void createWoodPanel() {
    // Cached once per editor, so grain never repaints randomly or touches audio.
    woodPanel = juce::Image(juce::Image::RGB, 1100, 800, false);
    juce::Image::BitmapData pixels(woodPanel, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < 800; ++y) for (int x = 0; x < 1100; ++x) {
      const bool cheek = x < 28 || x >= 1072;
      const float along = float(cheek ? y : x), across = float(cheek ? x : y);
      const float grain = std::sin(across * 2.6f + std::sin(along * .023f) * 1.8f)
                        + .55f * std::sin(across * 6.1f + along * .009f)
                        + .35f * std::sin(across * .26f + std::sin(along * .006f) * 3);
      const auto noise = uint32_t(x * 1973 + y * 9277 + 89173) * 1664525u + 1013904223u;
      const float fleck = float((noise >> 24) & 255) / 255 - .5f;
      const float edge = juce::jmin(1.0f, float(juce::jmin(juce::jmin(x, 1099 - x), juce::jmin(y, 799 - y))) / 14);
      const float shade = (.7f + .3f * edge) * (1 + grain * (cheek ? .17f : .09f) + fleck * .11f);
      auto channel = [shade](float value) { return juce::uint8(juce::jlimit(0, 255, int(value * shade))); };
      pixels.setPixelColour(x, y, juce::Colour(channel(cheek ? 134 : 34), channel(cheek ? 64 : 29), channel(cheek ? 27 : 23)));
    }
  }
  void paint(juce::Graphics &g) override {
    g.drawImageAt(woodPanel, 0, 0);
    const auto cream = VintageLook::cream();
    g.setColour(cream); g.setFont(VintageLook::lettering(33));
    g.drawText("BASSBOXX", 51, 8, 255, 39, juce::Justification::centredLeft);
    g.setFont(VintageLook::lettering(17));
    g.drawText("by", 52, 51, 25, 24, juce::Justification::centredLeft);
    juce::GlyphArrangement logo; logo.addLineOfText(juce::Font(juce::FontOptions(25.0f, juce::Font::bold)), "ProHoeZak", 0, 0);
    juce::Path mark; logo.createPath(mark);
    mark.applyTransform(juce::AffineTransform::translation(81, 73));
    g.setColour(juce::Colour(0xff090707)); g.strokePath(mark, juce::PathStrokeType(9));
    g.setColour(juce::Colour(0xffc8d915)); g.strokePath(mark, juce::PathStrokeType(7));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffffe532), 0, 48, juce::Colour(0xffde4db8), 0, 75, false));
    g.fillPath(mark); g.setColour(juce::Colour(0xff4c163c)); g.strokePath(mark, juce::PathStrokeType(.8f));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffff9b1e).withAlpha(.45f), 333, 38,
                                          juce::Colours::transparentBlack, 349, 38, true));
    g.fillEllipse(317, 22, 32, 32);
    g.setColour(VintageLook::gold()); g.fillEllipse(326, 31, 14, 14);
    g.setColour(juce::Colour(0xffffa214)); g.fillEllipse(328, 33, 10, 10);
    g.setColour(juce::Colour(0xfffff2a4)); g.fillEllipse(330, 34, 5, 7);
    g.setColour(cream); g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.drawText("POWER", 310, 51, 46, 16, juce::Justification::centred);
    auto section = [&](juce::Rectangle<int> r, const juce::String &name) {
      VintageLook::bezel(g, r.toFloat(), 5);
      auto plaque = juce::Rectangle<float>(float(r.getX() + 20), float(r.getY() - 10), float(r.getWidth() - 40), 21);
      g.setGradientFill(juce::ColourGradient(juce::Colour(0xff211b14), 0, plaque.getY(), juce::Colour(0xff0c0a08), 0, plaque.getBottom(), false));
      g.fillRoundedRectangle(plaque, 10); g.setColour(VintageLook::gold()); g.drawRoundedRectangle(plaque, 10, .8f);
      g.setColour(cream); g.setFont(VintageLook::lettering(14));
      g.drawText(name, r.getX() + 8, r.getY() - 10, r.getWidth() - 16, 20, juce::Justification::centred);
    };
    section({44, 107, 378, 478}, "OSCILLATOR BANK");
    section({432, 107, 118, 478}, "MIXER");
    section({560, 107, 326, 478}, "MODIFIERS");
    section({896, 107, 159, 478}, "OUTPUT");
    g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold))); g.setColour(cream);
    for (int i = 0; i < 3; ++i) {
      g.drawText("OSCILLATOR " + juce::String(i + 1), 207, 140 + 150 * i, 194, 22, juce::Justification::centred);
      g.setFont(10); g.drawText("WAVEFORM", 208, 168 + 150 * i, 194, 18, juce::Justification::centred); g.setFont(12);
      g.drawText("-24      0      +24", 78, 250 + 150 * i, 122, 17, juce::Justification::centred);
    }
    g.drawText("FILTER CONTOUR", 570, 260, 305, 16, juce::Justification::centred);
    g.drawText("LOUDNESS CONTOUR", 570, 410, 305, 16, juce::Justification::centred);
    g.drawText("VOICE MODE", 905, 443, 142, 22, juce::Justification::centred);
    g.setFont(10);
    g.drawText(processor.isMonophonic() ? "LAST NOTE / LEGATO" : "UP TO 16 VOICES", 903, 520, 146, 20, juce::Justification::centred);
    VintageLook::bezel(g, {42, 612, 145, 177}, 4);
    g.setColour(cream); g.setFont(11);
    g.drawText("PITCH", 45, 622, 62, 20, juce::Justification::centred);
    g.drawText("MOD", 115, 622, 62, 20, juce::Justification::centred);
    g.setFont(VintageLook::lettering(13));
    g.drawText("THREE OSCILLATOR VIRTUAL ANALOG SYNTHESIZER", 220, 611, 816, 23, juce::Justification::centred);
    for (int x : {38, 1062}) for (int y : {11, 788}) {
      g.setColour(juce::Colours::black); g.fillEllipse(float(x - 5), float(y - 4), 10, 10);
      g.setGradientFill(juce::ColourGradient(cream, float(x - 3), float(y - 3), VintageLook::gold().darker(.5f), float(x + 3), float(y + 3), false));
      g.fillEllipse(float(x - 4), float(y - 4), 8, 8);
      g.setColour(juce::Colours::black); g.drawLine(float(x - 2), float(y), float(x + 2), float(y), 1);
    }
  }
};
