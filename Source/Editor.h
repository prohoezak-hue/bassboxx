#pragma once
#include "Plugin.h"
class VintageLook final : public juce::LookAndFeel_V4 {
public:
  void drawRotarySlider(juce::Graphics &g, int x, int y, int w, int h,
                        float pos, float start, float end,
                        juce::Slider &) override {
    auto r = juce::Rectangle<float>(float(x), float(y), float(w), float(h))
                 .reduced(5);
    float d = juce::jmin(r.getWidth(), r.getHeight());
    r = r.withSizeKeepingCentre(d, d);
    g.setColour(juce::Colours::black.withAlpha(.5f));
    g.fillEllipse(r.translated(2, 3));
    juce::ColourGradient grad(juce::Colour(0xff454545), r.getX(), r.getY(),
                              juce::Colour(0xff080808), r.getRight(),
                              r.getBottom(), false);
    g.setGradientFill(grad);
    g.fillEllipse(r);
    g.setColour(juce::Colour(0xff777777));
    g.drawEllipse(r, 1);
    auto c = r.getCentre();
    float a = start + pos * (end - start);
    g.setColour(juce::Colour(0xffffd7a0));
    g.drawLine(c.x + std::sin(a) * d * .2f, c.y - std::cos(a) * d * .2f,
               c.x + std::sin(a) * d * .42f, c.y - std::cos(a) * d * .42f,
               2.5f);
  }
};
class BassboxxEditor final : public juce::AudioProcessorEditor,
                             private juce::Timer {
  BassboxxProcessor &processor;
  VintageLook look;
  juce::MidiKeyboardComponent keys;
  juce::ComboBox presets;
  juce::TextButton save{"Save patch"}, load{"Load patch"}, panic{"Panic"};
  std::unique_ptr<juce::FileChooser> chooser;
  std::vector<std::unique_ptr<juce::Slider>> knobs;
  std::vector<std::unique_ptr<juce::Label>> labels;
  std::vector<
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>>
      attachments;
  std::array<juce::ComboBox, 3> waves;
  std::vector<
      std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>>
      waveAttachments;
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
    presets.setSelectedId(processor.getCurrentProgram() + 1,
                          juce::dontSendNotification);
  }

public:
  explicit BassboxxEditor(BassboxxProcessor &p)
      : AudioProcessorEditor(p), processor(p),
        keys(p.keyboard, juce::MidiKeyboardComponent::horizontalKeyboard) {
    setLookAndFeel(&look);
    startTimerHz(5);
    addAndMakeVisible(keys);
    keys.setAvailableRange(48, 83);
    keys.setLowestVisibleKey(48);
    keys.setScrollButtonsVisible(false);
    keys.setKeyWidth(juce::jmax(20.0f, 1004.0f / 21));
    addAndMakeVisible(presets);
    for (int i = 0; i < 100; ++i)
      presets.addItem(p.getProgramName(i), i + 1);
    presets.setSelectedId(p.getCurrentProgram() + 1,
                          juce::dontSendNotification);
    presets.onChange = [this] {
      processor.setCurrentProgram(presets.getSelectedId() - 1);
    };
    for (auto *b : {&save, &load, &panic})
      addAndMakeVisible(b);
    save.onClick = [this] { file(true); };
    load.onClick = [this] { file(false); };
    panic.onClick = [this] {
      processor.panicRequested.store(true);
      for (int c = 1; c <= 16; ++c)
        processor.keyboard.allNotesOff(c);
    };
    auto knob = [&](juce::String id, juce::String name) {
      auto s = std::make_unique<juce::Slider>();
      s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
      s->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 78, 18);
      s->setColour(juce::Slider::textBoxTextColourId, juce::Colours::ivory);
      s->setColour(juce::Slider::textBoxOutlineColourId,
                   juce::Colours::transparentBlack);
      addAndMakeVisible(*s);
      auto l = std::make_unique<juce::Label>();
      l->setText(name, juce::dontSendNotification);
      l->setJustificationType(juce::Justification::centred);
      l->setColour(juce::Label::textColourId, juce::Colours::ivory);
      addAndMakeVisible(*l);
      attachments.push_back(
          std::make_unique<
              juce::AudioProcessorValueTreeState::SliderAttachment>(p.state, id,
                                                                    *s));
      knobs.push_back(std::move(s));
      labels.push_back(std::move(l));
    };
    for (int i = 0; i < 3; ++i) {
      auto n = juce::String(i + 1);
      knob("level" + n, "OSC " + n + " LEVEL");
      knob("tune" + n, "TUNE (st)");
      addAndMakeVisible(waves[i]);
      waves[i].addItemList({"Saw", "Pulse", "Triangle", "Sine"}, 1);
      waveAttachments.push_back(
          std::make_unique<
              juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
              p.state, "wave" + n, waves[i]));
    }
    for (auto pair :
         std::initializer_list<std::pair<const char *, const char *>>{
             {"cutoff", "CUTOFF"},
             {"resonance", "RESONANCE"},
             {"env", "FILTER ENV"},
             {"attack", "ATTACK"},
             {"decay", "DECAY"},
             {"sustain", "SUSTAIN"},
             {"release", "RELEASE"},
             {"glide", "GLIDE"},
             {"voices", "VOICES"},
             {"boost", "BASS BOOST"},
             {"gain", "OUTPUT"},
             {"filterAttack", "FILTER ATT"},
             {"filterDecay", "FILTER DEC"},
             {"filterSustain", "FILTER SUS"},
             {"filterRelease", "FILTER REL"}})
      knob(pair.first, pair.second);
    setSize(1100, 800);
  }
  ~BassboxxEditor() override {
    stopTimer();
    setLookAndFeel(nullptr);
  }
  void resized() override {
    presets.setBounds(400, 24, 300, 28);
    save.setBounds(710, 24, 110, 28);
    load.setBounds(825, 24, 110, 28);
    panic.setBounds(940, 24, 100, 28);
    for (int i = 0; i < int(knobs.size()); ++i) {
      int col = i < 6 ? i : i < 17 ? i - 6 : i - 17;
      int row = i < 6 ? 0 : i < 17 ? 1 : 2;
      int x = 48 + col * (row ? 91 : 155);
      int y = row == 0 ? 135 : row == 1 ? 335 : 480;
      knobs[i]->setBounds(x, y, 86, 105);
      labels[i]->setBounds(x - 5, y - 24, 96, 23);
    }
    for (int i = 0; i < 3; ++i)
      waves[i].setBounds(55 + i * 310, 255, 220, 26);
    keys.setBounds(48, 645, 1004, 135);
  }
  void paint(juce::Graphics &g) override {
    g.fillAll(juce::Colour(0xff331b10));
    for (int x = 0; x < 1100; ++x) {
      if (x > 30 && x < 1070)
        continue;
      g.setColour(
          juce::Colour(0xffa26332).withAlpha(.25f + .15f * std::sin(x * 1.7f)));
      g.drawVerticalLine(x, 0, 800);
    }
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffb63438), 30, 0,
                                           juce::Colour(0xff46080c), 1070, 800,
                                           false));
    g.fillRect(30, 0, 1040, 800);
    for (int y = 0; y < 800; y += 3) {
      g.setColour(juce::Colours::white.withAlpha(.025f));
      g.drawHorizontalLine(y, 30, 1070);
    }
    g.setColour(juce::Colours::ivory);
    g.setFont(32);
    g.drawText("BASSBOXX", 52, 16, 300, 40, juce::Justification::centredLeft);
    g.setFont(15);
    g.drawText("by ProHoeZak", 54, 56, 280, 25,
               juce::Justification::centredLeft);
    g.setFont(14);
    g.drawText("THREE OSCILLATOR VIRTUAL ANALOG SYNTHESIZER", 52, 606, 900, 25,
               juce::Justification::centredLeft);
    for (int x : {39, 1061})
      for (int y : {10, 790}) {
        g.setColour(juce::Colours::silver);
        g.fillEllipse(float(x - 3), float(y - 3), 6, 6);
        g.setColour(juce::Colours::black);
        g.drawLine(float(x - 2), float(y), float(x + 2), float(y));
      }
  }
};
