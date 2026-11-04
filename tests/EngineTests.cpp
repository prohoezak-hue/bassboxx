#include "UserPresets.h"
#include <cstdlib>
#include <iostream>
static void check(bool ok, const char *what) {
  if (!ok) {
    std::cerr << "FAIL: " << what << '\n';
    std::exit(1);
  }
}
int main() {
  for (double rate : {44100., 48000., 96000.}) {
    bassboxx::Engine e;
    e.prepare(rate);
    check(e.sample() == 0, "silence before MIDI");
    e.noteOn(45, 1);
    double energy = 0;
    for (int i = 0; i < int(rate); ++i) {
      float s = e.sample();
      check(std::isfinite(s) && std::abs(s) <= 1, "finite bounded signal");
      energy += s * s;
    }
    check(energy > 1, "MIDI generates audio");
    e.noteOff(45);
    for (int i = 0; i < int(rate * 6); ++i)
      e.sample();
    check(std::abs(e.sample()) < 1e-6, "release returns to silence");
    e.sustain(true);
    e.noteOn(48, 1);
    e.noteOff(48);
    energy = 0;
    for (int i = 0; i < int(rate); ++i)
      energy += std::abs(e.sample());
    check(energy > 1, "sustain pedal holds note");
    e.sustain(false);
    for (int i = 0; i < int(rate * 6); ++i)
      e.sample();
    check(std::abs(e.sample()) < 1e-6, "pedal release ends note");
    for (int preset = 0; preset < bassboxx::programCount; ++preset) {
      e.reset();
      e.patch = bassboxx::programPatch(preset);
      for (int n = 24; n < 48; ++n)
        e.noteOn(n, .8f);
      energy = 0;
      for (int i = 0; i < 12000; ++i) {
        float s = e.sample();
        check(std::isfinite(s) && std::abs(s) <= 1,
              "all factory patches stable under voice stealing");
        energy += s * s;
      }
      check(energy > .01, "every factory patch audible");
      e.allOff();
      for (int i = 0; i < 10000; ++i)
        e.sample();
      check(std::abs(e.sample()) < 1e-6, "panic silence");
    }
  }
  bassboxx::Engine a, b;
  a.prepare(48000);
  b.prepare(48000);
  a.patch.cutoff = b.patch.cutoff = 500;
  a.patch.env = b.patch.env = 0;
  b.patch.boost = 1;
  a.noteOn(36, 1);
  b.noteOn(36, 1);
  double low = 0, high = 0;
  for (int i = 0; i < 48000; ++i) {
    auto x = a.sample(), y = b.sample();
    low += x * x;
    high += y * y;
  }
  check(high > low * 2, "Bass Boost increases bass energy");
  bassboxx::Engine pitch;
  pitch.prepare(48000);
  pitch.patch.level = {1, 0, 0};
  pitch.patch.wave = {3, 3, 3};
  pitch.patch.cutoff = 18000;
  pitch.patch.env = 0;
  pitch.patch.sustain = 1;
  pitch.noteOn(57, 1);
  for (int i = 0; i < 4800; ++i)
    pitch.sample();
  auto crossings = [&]() {
    int count = 0;
    float prev = pitch.sample();
    for (int i = 0; i < 48000; ++i) {
      float next = pitch.sample();
      if (prev <= 0 && next > 0)
        ++count;
      prev = next;
    }
    return count;
  };
  check(std::abs(crossings() - 220) <= 2, "oscillator pitch A3 220Hz");
  pitch.bend(12);
  for (int i = 0; i < 4800; ++i)
    pitch.sample();
  check(std::abs(crossings() - 440) <= 2, "pitch bend changes frequency");
  auto sine = [](bassboxx::Engine &e, int voices) {
    e.prepare(48000);
    e.patch.level = {1, 0, 0}; e.patch.wave = {3, 3, 3};
    e.patch.cutoff = 18000; e.patch.env = 0; e.patch.sustain = 1;
    e.patch.attack = .001f; e.patch.decay = .001f; e.patch.release = .02f;
    e.patch.voices = voices;
  };
  auto frequency = [](bassboxx::Engine &e) {
    for (int i = 0; i < 4800; ++i) e.sample();
    float previous = e.sample(); int count = 0;
    for (int i = 0; i < 48000; ++i) {
      const float next = e.sample();
      if (previous <= 0 && next > 0) ++count;
      previous = next;
    }
    return count;
  };
  bassboxx::Engine mono;
  sine(mono, 1);
  mono.noteOn(57, 1); check(std::abs(frequency(mono) - 220) <= 2, "mono first note");
  mono.noteOn(69, 1); check(std::abs(frequency(mono) - 440) <= 2, "mono last-note priority, no chord");
  mono.noteOff(69); check(std::abs(frequency(mono) - 220) <= 2, "mono returns to still-held key");
  mono.noteOn(64, 1); mono.noteOff(57);
  check(std::abs(frequency(mono) - 330) <= 2, "releasing older mono key does not release current note");
  mono.sustain(true); mono.noteOff(64);
  check(std::abs(frequency(mono) - 330) <= 2, "mono sustain pedal holds current note");
  mono.sustain(false);
  for (int i = 0; i < 4800; ++i) mono.sample();
  check(std::abs(mono.sample()) < 1e-6, "mono pedal release ends note");
  mono.noteOn(57, 1); mono.noteOn(69, 1); mono.allOff(); mono.noteOff(69); mono.noteOff(57);
  mono.noteOn(60, 1); mono.noteOff(60);
  for (int i = 0; i < 4800; ++i) mono.sample();
  check(std::abs(mono.sample()) < 1e-6, "panic clears mono held-note stack");
  bassboxx::Engine changing, single;
  sine(changing, 8); sine(single, 1);
  changing.noteOn(57, 1); changing.noteOn(69, 1); changing.patch.voices = 1;
  single.noteOn(69, 1);
  for (int i = 0; i < 4800; ++i)
    check(changing.sample() == single.sample(), "switching to mono preserves newest note and removes other voices");
  bassboxx::Engine expanding, poly;
  sine(expanding, 1); sine(poly, 8);
  expanding.noteOn(69, 1); expanding.patch.voices = 8; expanding.noteOn(57, 1);
  poly.noteOn(69, 1); poly.noteOn(57, 1);
  for (int i = 0; i < 4800; ++i)
    check(expanding.sample() == poly.sample(), "switching back to poly allows multiple notes");
  bassboxx::Engine modulated, unchanged;
  sine(modulated, 8); sine(unchanged, 8);
  modulated.noteOn(57, 1, 2); unchanged.noteOn(57, 1, 2);
  modulated.modulate(1, 1);
  for (int i = 0; i < 4800; ++i)
    check(modulated.sample() == unchanged.sample(), "MIDI CC1 modulation is per-channel");
  modulated.modulate(1, 2);
  double difference = 0;
  for (int i = 0; i < 48000; ++i) {
    const float value = modulated.sample();
    check(std::isfinite(value) && std::abs(value) <= 1, "mod wheel keeps audio finite and bounded");
    difference += std::abs(value - unchanged.sample());
  }
  check(difference > 10, "mod wheel produces audible vibrato");
  sine(modulated, 8); modulated.noteOn(57, 1); modulated.modulate(1);
  for (int i = 0; i < 4800; ++i) modulated.sample();
  double last = -1, minHz = 1000, maxHz = 0;
  float previous = modulated.sample();
  for (int i = 0; i < 48000; ++i) {
    const float next = modulated.sample();
    if (previous <= 0 && next > 0) {
      const double at = i - previous / double(next - previous);
      if (last >= 0) {
        const double hz = 48000 / (at - last);
        minHz = std::min(minHz, hz); maxHz = std::max(maxHz, hz);
      }
      last = at;
    }
    previous = next;
  }
  check(minHz > 210 && maxHz < 230 && maxHz - minHz > 8, "mod wheel oscillates around original pitch with bounded depth");
  std::cout << "PASS: 3 sample rates, 125 presets each, audio, release, pedal, "
               "voice stealing, panic, Bass Boost, mono/poly, last-note return and CC1 vibrato\n";
}
