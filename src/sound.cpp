#include "sound.h"
#include "config.h"

namespace {
  struct Note { uint16_t freq; uint16_t ms; };   // freq 0 = silencio

  const Note CHIRP[]      = {{1800, 50}, {2400, 70}};
  const Note GRUMBLE[]    = {{300, 120}, {0, 40}, {250, 180}};
  const Note DIZZY[]      = {{900, 60}, {1200, 60}, {800, 60}, {1300, 60}, {700, 60}, {1100, 120}};
  const Note EVENT_SOON[] = {{1319, 120}, {0, 60}, {1319, 120}, {0, 60}, {1760, 220}};
  const Note EVENT_NOW[]  = {{1047, 120}, {1319, 120}, {1568, 120}, {2093, 300}, {0, 150},
                             {1568, 120}, {2093, 350}};
  const Note FOCUS_START[]= {{880, 100}, {1175, 160}};
  const Note FOCUS_DONE[] = {{1568, 120}, {1319, 120}, {1568, 120}, {2093, 400}};
  const Note BREAK_DONE[] = {{1047, 150}, {1319, 150}, {1047, 250}};
  const Note HELLO[]      = {{1319, 90}, {1568, 90}, {2093, 160}};

  const Note* cur = nullptr;
  uint8_t curLen = 0, idx = 0;
  uint32_t noteEnd = 0;

  void tone_(uint16_t f) {
#if BUZZER_PIN >= 0 && SOUND_ENABLED
    if (f == 0) ledcWriteTone(0, 0);
    else        ledcWriteTone(0, f);
#endif
  }

  template <size_t N> void set(const Note (&arr)[N]) { cur = arr; curLen = N; }
}

namespace Sound {

void begin() {
#if BUZZER_PIN >= 0 && SOUND_ENABLED
  ledcSetup(0, 2000, 8);
  ledcAttachPin(BUZZER_PIN, 0);
  ledcWriteTone(0, 0);
#endif
}

void play(Tune t) {
  switch (t) {
    case Tune::Chirp:      set(CHIRP); break;
    case Tune::Grumble:    set(GRUMBLE); break;
    case Tune::Dizzy:      set(DIZZY); break;
    case Tune::EventSoon:  set(EVENT_SOON); break;
    case Tune::EventNow:   set(EVENT_NOW); break;
    case Tune::FocusStart: set(FOCUS_START); break;
    case Tune::FocusDone:  set(FOCUS_DONE); break;
    case Tune::BreakDone:  set(BREAK_DONE); break;
    case Tune::Hello:      set(HELLO); break;
  }
  idx = 0;
  tone_(cur[0].freq);
  noteEnd = millis() + cur[0].ms;
}

void update() {
  if (!cur) return;
  if ((int32_t)(millis() - noteEnd) < 0) return;
  idx++;
  if (idx >= curLen) { tone_(0); cur = nullptr; return; }
  tone_(cur[idx].freq);
  noteEnd = millis() + cur[idx].ms;
}

bool busy() { return cur != nullptr; }

}  // namespace Sound
