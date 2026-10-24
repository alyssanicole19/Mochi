// Sonidos de Mochi con buzzer pasivo (no bloquean el programa)
#pragma once
#include <Arduino.h>

enum class Tune : uint8_t {
  Chirp,        // toque feliz
  Grumble,      // enojado
  Dizzy,        // mareado
  EventSoon,    // evento en 10 minutos
  EventNow,     // el evento empieza ya
  FocusStart,   // empieza Pomodoro
  FocusDone,    // terminó el bloque de concentración
  BreakDone,    // terminó el descanso
  Hello         // al encender
};

namespace Sound {
  void begin();
  void play(Tune t);
  void update();     // llamar en cada ciclo
  bool busy();
}
