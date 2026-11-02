#pragma once
#include "Engine.h"
#include <JuceHeader.h>
class BassboxxProcessor final : public juce::AudioProcessor {
public:
  BassboxxProcessor();
  juce::AudioProcessorValueTreeState state;
  juce::MidiKeyboardState keyboard;
  std::atomic<bool> panicRequested{false};
  void prepareToPlay(double, int) override;
  void releaseResources() override {}
  bool isBusesLayoutSupported(const BusesLayout &) const override;
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override { return true; }
  const juce::String getName() const override { return "BASSBOXX"; }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return false; }
  double getTailLengthSeconds() const override { return 5; }
  int getNumPrograms() override { return 100; }
  int getCurrentProgram() override { return program.load(); }
  void setCurrentProgram(int) override;
  const juce::String getProgramName(int) override;
  void changeProgramName(int, const juce::String &) override {}
  void getStateInformation(juce::MemoryBlock &) override;
  void setStateInformation(const void *, int) override;

private:
  bassboxx::Engine engine;
  std::atomic<int> program{0};
  std::array<std::atomic<float> *, 24> values{};
  static juce::AudioProcessorValueTreeState::ParameterLayout parameters();
  void readPatch();
  void midi(const juce::MidiMessage &);
};
