// La carita de Mochi: ojos animados con emociones
#pragma once
#include <TFT_eSPI.h>

enum class Emotion : uint8_t {
  Neutral, Happy, Love, Angry, Surprised, Sleepy, Bored, Focused, Alert, Dizzy, Relaxed
};

namespace Face {
  void begin();
  void setEmotion(Emotion e);
  Emotion emotion();
  void setLookBias(float x, float y);   // -1..1, p.ej. inclinación del robot
  void render(TFT_eSprite& spr, uint32_t nowMs);
  void forceBlink();
  void setOffsetY(float y);             // sube la cara cuando hay texto abajo
}
