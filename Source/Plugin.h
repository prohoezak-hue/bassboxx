#pragma once
#include "UserPresets.h"
#include <JuceHeader.h>
class BassboxxProcessor final : public juce::AudioProcessor,
                               private juce::AudioProcessorValueTreeState::Listener {
public:
  BassboxxProcessor();
  ~BassboxxProcessor() override;
  juce::AudioProcessorValueTreeState state;
  juce::MidiKeyboardState keyboard;
  std::atomic<bool> panicRequested{false};
  bool isMonophonic() const { return values[19]->load() <= 1; }
  void setMonophonic(bool);
  void setPitchWheel(float);
  void setModWheel(float);
  float getPitchWheel() const { return pitchPosition.load(); }
  float getModWheel() const { return modPosition.load(); }
  void prepareToPlay(double, int) override;
  void releaseResources() override {}
  bool isBusesLayoutSupported(const BusesLayout &) const override;
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override { return true; }
  const juce::String getName() const override { return "TheFunkBoxx"; }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return false; }
  double getTailLengthSeconds() const override { return 5; }
  int getNumPrograms() override { return bassboxx::programCount; }
  int getCurrentProgram() override { return program.load(); }
  void setCurrentProgram(int) override;
  const juce::String getProgramName(int) override;
  void changeProgramName(int, const juce::String &) override {}
  void getStateInformation(juce::MemoryBlock &) override;
  void setStateInformation(const void *, int) override;

private:
  bassboxx::Engine engine;
  std::atomic<int> program{0};
  std::atomic<int> polyVoices{8}, pendingPitch{-1}, pendingMod{-1};
  std::atomic<float> pitchPosition{0}, modPosition{0};
  std::array<std::atomic<float> *, 24> values{};
  static juce::AudioProcessorValueTreeState::ParameterLayout parameters();
  void readPatch();
  void midi(const juce::MidiMessage &);
  void parameterChanged(const juce::String &, float value) override {
    if (value > 1) polyVoices.store(juce::jlimit(2, 16, juce::roundToInt(value)));
  }
};
