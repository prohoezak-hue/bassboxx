#include "Engine.h"
#include <fstream>
#include <iostream>
static void word(std::ostream &out, unsigned value, int bytes) {
  for (int i = 0; i < bytes; ++i)
    out.put(char((value >> (i * 8)) & 255));
}
int main(int argc, char **argv) {
  const int sr = 48000, seconds = 8, count = sr * seconds;
  std::ofstream out(argc > 1 ? argv[1] : "TheFunkBoxx-demo.wav", std::ios::binary);
  if (!out)
    return 1;
  out.write("RIFF", 4);
  word(out, 36 + count * 2, 4);
  out.write("WAVEfmt ", 8);
  word(out, 16, 4);
  word(out, 1, 2);
  word(out, 1, 2);
  word(out, sr, 4);
  word(out, sr * 2, 4);
  word(out, 2, 2);
  word(out, 16, 2);
  out.write("data", 4);
  word(out, count * 2, 4);
  bassboxx::Engine synth;
  synth.prepare(sr);
  synth.patch = bassboxx::factory(7);
  int notes[] = {36, 36, 43, 39, 36, 48, 46, 43};
  for (int i = 0; i < count; ++i) {
    int step = i / (sr / 4);
    if (i % (sr / 4) == 0 && step < 28)
      synth.noteOn(notes[step % 8], .8f);
    if (i % (sr / 4) == sr / 6)
      synth.noteOff(notes[step % 8]);
    float s = synth.sample();
    word(out, unsigned(int(s * 32767) & 65535), 2);
  }
  std::cout << "Rendered 8 seconds of MIDI-equivalent bass sequence\n";
  return out ? 0 : 1;
}
