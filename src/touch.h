// Táctil CST816S — driver mínimo
#pragma once
#include <Arduino.h>

enum class Gesture : uint8_t {
  None = 0x00, SwipeUp = 0x01, SwipeDown = 0x02, SwipeLeft = 0x03,
  SwipeRight = 0x04, Tap = 0x05, DoubleTap = 0x0B, LongPress = 0x0C
};

struct TouchEvent {
  bool touching = false;
  Gesture gesture = Gesture::None;
  int16_t x = 0, y = 0;
};

namespace Touch {
  bool begin();
  TouchEvent read();          // llamar en cada ciclo
}
