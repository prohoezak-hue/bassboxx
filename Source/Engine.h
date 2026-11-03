#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace bassboxx {
constexpr double pi = 3.14159265358979323846;
struct Patch {
  std::array<float, 3> level{0.65f, 0.35f, 0.2f}, tune{0, 0.07f, -12};
  std::array<int, 3> wave{0, 0, 1};
  float cutoff = 1800, resonance = 0.2f, attack = 0.008f, decay = 0.25f,
        sustain = 0.65f, release = 0.3f, env = 0.5f, glide = 0, boost = 0,
        gain = 0.65f;
  float filterAttack = .006f, filterDecay = .3f, filterSustain = .2f,
        filterRelease = .25f;
  int voices = 8;
};
inline Patch factory(int i) {
  Patch p;
  i = std::clamp(i, 0, 99);
  int bank = i / 10, variation = i % 10;
  p.wave = {bank % 4, (bank + 1) % 4, 1};
  p.tune = {0, 0.02f + variation * 0.012f, bank == 5 ? 7.0f : -12.0f};
  p.cutoff = 150.0f * std::pow(1.55f, float(variation)) + bank * 110;
  p.resonance = 0.1f + bank * 0.065f;
  p.attack = bank == 6 ? 0.35f : 0.004f;
  p.decay = 0.12f + variation * 0.055f;
  p.sustain = bank == 3 ? 0.0f : 0.5f;
  p.release = bank == 6 ? 1.2f : 0.15f + variation * 0.035f;
  p.env = 0.2f + bank * 0.075f;
  p.filterAttack = p.attack;
  p.filterDecay = p.decay * 1.4f;
  p.filterSustain = bank == 6 ? .7f : .1f;
  p.filterRelease = p.release;
  p.boost = 0.25f + variation * 0.055f;
  p.glide = bank == 1 ? 0.12f : 0;
  p.voices = bank == 1 ? 1 : 8;
  return p;
}
struct Envelope {
  float value = 0;
  int stage = 0;
  void on() { stage = 1; }
  void off() {
    if (stage)
      stage = 4;
  }
  float tick(const Patch &p, double sr) {
    return tick(p.attack, p.decay, p.sustain, p.release, sr);
  }
  float tick(float attack, float decay, float sustain, float release,
             double sr) {
    if (stage == 1) {
      value += 1.0f / float(std::max(0.001f, attack) * sr);
      if (value >= 1) {
        value = 1;
        stage = 2;
      }
    } else if (stage == 2) {
      value -= (1 - sustain) / float(std::max(0.001f, decay) * sr);
      if (value <= sustain) {
        value = sustain;
        stage = 3;
      }
    } else if (stage == 3)
      value = sustain;
    else if (stage == 4) {
      value -= 1.0f / float(std::max(0.001f, release) * sr);
      if (value <= 0) {
        value = 0;
        stage = 0;
      }
    }
    return value;
  }
};
class Engine {
  struct Voice {
    int note = -1, channel = 1;
    bool held = false, deferred = false;
    uint64_t age = 0;
    double hz = 0;
    float velocity = 0, ic1 = 0, ic2 = 0;
    std::array<double, 3> phase{};
    Envelope amp, filterEnvelope;
  };
  std::array<Voice, 16> v{};
  std::array<bool, 16> pedal{};
  std::array<float, 16> bends{};
  std::array<float, 16> modulation{};
  std::array<std::array<uint64_t, 128>, 16> heldOrder{};
  std::array<std::array<float, 128>, 16> heldVelocity{};
  int heldCount = 0;
  bool monoMode = false, modulationActive = false;
  double modulationPhase = 0;
  double sr = 44100;
  uint64_t age = 0;
  float bass = 0;
  Patch smooth;
  bool initial = true;
  void syncMode() {
    const bool next = patch.voices == 1;
    if (next == monoMode) return;
    if (next) {
      Voice newest;
      for (const auto &voice : v)
        if (voice.amp.stage && voice.held && voice.age >= newest.age) newest = voice;
      if (!newest.amp.stage)
        for (const auto &voice : v)
          if (voice.amp.stage && voice.age >= newest.age) newest = voice;
      v = {};
      v[0] = newest;
      if (heldCount > 0) restoreMonoHeldNote();
    }
    monoMode = next;
  }
  void assignMono(int note, float velocity, int channel, bool legato) {
    auto &voice = v[0];
    if (!legato || !voice.amp.stage || voice.amp.stage == 4) {
      const double oldHz = voice.hz;
      voice = Voice{};
      voice.hz = patch.glide > 0 && oldHz > 0 ? oldHz : 440 * std::pow(2.0, (note - 69) / 12.0);
      voice.amp.on();
      voice.filterEnvelope.on();
    }
    voice.note = note;
    voice.channel = channel;
    voice.velocity = velocity;
    voice.held = true;
    voice.deferred = false;
    voice.age = heldOrder[size_t(channel - 1)][size_t(note)];
  }
  bool restoreMonoHeldNote() {
    uint64_t newest = 0;
    int note = 0, channel = 0;
    for (int c = 0; c < 16; ++c)
      for (int n = 0; n < 128; ++n)
        if (heldOrder[size_t(c)][size_t(n)] > newest) {
          newest = heldOrder[size_t(c)][size_t(n)]; note = n; channel = c;
        }
    if (!newest) return false;
    assignMono(note, heldVelocity[size_t(channel)][size_t(note)], channel + 1, true);
    return true;
  }
  static double blep(double t, double dt) {
    if (t < dt) {
      t /= dt;
      return t + t - t * t - 1;
    }
    if (t > 1 - dt) {
      t = (t - 1) / dt;
      return t * t + t + t + 1;
    }
    return 0;
  }
  static double osc(double t, double dt, int wave) {
    if (wave == 0)
      return 2 * t - 1 - blep(t, dt);
    if (wave == 1)
      return (t < 0.5 ? 1 : -1) + blep(t, dt) -
             blep(std::fmod(t + 0.5, 1.0), dt);
    if (wave == 2)
      return 2 / pi * std::asin(std::sin(2 * pi * t));
    return std::sin(2 * pi * t);
  }

public:
  Patch patch;
  void prepare(double rate) {
    sr = rate;
    reset();
  }
  void reset() {
    v = {};
    pedal = {};
    bends = {};
    modulation = {};
    heldOrder = {};
    heldVelocity = {};
    heldCount = 0;
    monoMode = false;
    modulationActive = false;
    modulationPhase = 0;
    bass = 0;
    initial = true;
  }
  void noteOn(int note, float velocity, int ch = 1) {
    if (ch < 1 || ch > 16 || note < 0 || note > 127)
      return;
    syncMode();
    const bool legato = heldCount > 0;
    auto &order = heldOrder[size_t(ch - 1)][size_t(note)];
    if (!order) ++heldCount;
    order = ++age;
    heldVelocity[size_t(ch - 1)][size_t(note)] = velocity;
    if (monoMode) { assignMono(note, velocity, ch, legato); return; }
    int n = std::clamp(patch.voices, 1, 16);
    Voice *voice = &v[0];
    for (int i = 0; i < n; ++i) {
      if (v[i].amp.stage == 0) {
        voice = &v[i];
        break;
      }
      if (v[i].age < voice->age)
        voice = &v[i];
    }
    double old = voice->hz;
    *voice = Voice{};
    voice->hz = patch.glide > 0 && old > 0
                    ? old
                    : 440 * std::pow(2.0, (note - 69) / 12.0);
    voice->note = note;
    voice->channel = ch;
    voice->held = true;
    voice->age = age;
    voice->velocity = velocity;
    voice->amp.on();
    voice->filterEnvelope.on();
  }
  void noteOff(int note, int ch = 1) {
    if (ch < 1 || ch > 16 || note < 0 || note > 127) return;
    syncMode();
    auto &order = heldOrder[size_t(ch - 1)][size_t(note)];
    if (order) { order = 0; --heldCount; }
    if (monoMode && v[0].note == note && v[0].channel == ch && restoreMonoHeldNote()) return;
    for (auto &x : v)
      if (x.note == note && x.channel == ch && x.held) {
        x.held = false;
        if (pedal[ch - 1])
          x.deferred = true;
        else {
          x.amp.off();
          x.filterEnvelope.off();
        }
      }
  }
  void sustain(bool down, int ch = 1) {
    if (ch < 1 || ch > 16) return;
    pedal[ch - 1] = down;
    if (!down)
      for (auto &x : v)
        if (x.channel == ch && x.deferred) {
          x.deferred = false;
          x.amp.off();
          x.filterEnvelope.off();
        }
  }
  void bend(float semitones, int ch = 1) {
    if (ch >= 1 && ch <= 16 && std::isfinite(semitones)) bends[size_t(ch - 1)] = semitones;
  }
  void modulate(float amount, int ch = 1) {
    if (ch < 1 || ch > 16) return;
    modulation[size_t(ch - 1)] = std::isfinite(amount) ? std::clamp(amount, 0.0f, 1.0f) : 0;
    modulationActive = std::any_of(modulation.begin(), modulation.end(), [](float x) { return x > 0; });
  }
  void allOff(int ch = 0) {
    if (ch < 0 || ch > 16) return;
    for (int c = 0; c < 16; ++c)
      if (ch == 0 || c == ch - 1)
        for (auto &order : heldOrder[size_t(c)])
          if (order) { order = 0; --heldCount; }
    for (auto &x : v)
      if (ch == 0 || x.channel == ch)
        x = Voice{};
    if (ch == 0)
      pedal = {};
    else
      pedal[ch - 1] = false;
    if (monoMode && !v[0].amp.stage && heldCount > 0) restoreMonoHeldNote();
  }
  float sample() {
    syncMode();
    if (initial) {
      smooth = patch;
      initial = false;
    }
    float a = float(1 - std::exp(-1 / (0.01 * sr)));
    smooth.cutoff += a * (patch.cutoff - smooth.cutoff);
    smooth.boost += a * (patch.boost - smooth.boost);
    smooth.gain += a * (patch.gain - smooth.gain);
    float out = 0;
    const double vibrato = modulationActive ? std::sin(2 * pi * modulationPhase) : 0;
    modulationPhase += 5.0 / sr;
    if (modulationPhase >= 1) modulationPhase -= 1;
    int n = std::clamp(patch.voices, 1, 16);
    for (int i = 0; i < 16; ++i) {
      auto &x = v[i];
      if (i >= n) {
        x = Voice{};
        continue;
      }
      if (!x.amp.stage)
        continue;
      double target =
          440 * std::pow(2.0, (x.note - 69 + bends[x.channel - 1]) / 12.0);
      x.hz += (target - x.hz) *
              (patch.glide <= 0 ? 1 : 1 - std::exp(-1 / (patch.glide * sr)));
      float e = x.amp.tick(patch, sr), s = 0;
      float fe =
          x.filterEnvelope.tick(patch.filterAttack, patch.filterDecay,
                                patch.filterSustain, patch.filterRelease, sr);
      double frequency = x.hz;
      const float depth = modulation[size_t(x.channel - 1)];
      if (depth > 0) frequency *= std::pow(2.0, depth * .5 * vibrato / 12.0);
      for (int o = 0; o < 3; ++o) {
        double dt = std::clamp(frequency * std::pow(2.0, patch.tune[o] / 12.0) / sr,
                               0.000001, 0.45);
        s += float(osc(x.phase[o], dt, patch.wave[o])) * patch.level[o];
        x.phase[o] += dt;
        x.phase[o] -= std::floor(x.phase[o]);
      }
      // TPT state-variable low-pass with nonlinear input drive.
      float cutoff =
          std::clamp(smooth.cutoff * std::pow(2.0f, fe * patch.env * 5), 20.0f,
                     float(sr * 0.42));
      float g = float(std::tan(pi * cutoff / sr)),
            k = 2 - 1.85f * patch.resonance;
      float v1 = (x.ic1 + g * (std::tanh(s) - x.ic2)) / (1 + g * (g + k));
      float v2 = x.ic2 + g * v1;
      x.ic1 = 2 * v1 - x.ic1;
      x.ic2 = 2 * v2 - x.ic2;
      out += v2 * e * x.velocity * 0.18f;
    }
    bass += float(1 - std::exp(-2 * pi * 160 / sr)) * (out - bass);
    return std::tanh((out + bass * smooth.boost * 5) * smooth.gain);
  }
};
} // namespace bassboxx
