#include "Engine.h"
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
    for (int preset = 0; preset < 100; ++preset) {
      e.reset();
      e.patch = bassboxx::factory(preset);
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
  std::cout << "PASS: 3 sample rates, 100 presets each, audio, release, pedal, "
               "voice stealing, panic, Bass Boost\n";
}
