// Lee los próximos eventos de Google Calendar a través de tu Apps Script.
// Corre en segundo plano (núcleo 0) para que la carita nunca se congele.
#pragma once
#include <Arduino.h>

struct CalEvent {
  char   title[48];
  time_t start;     // segundos (epoch)
  time_t end;
  bool   allDay;
  uint32_t id;      // hash para no repetir avisos
};

namespace Calendar {
  const int MAX_EVENTS = 8;
  void begin();                        // inicia la tarea en segundo plano
  // Copia los eventos actuales (ordenados por hora). Devuelve cuántos hay.
  int  snapshot(CalEvent* out);
  bool hasData();
}
