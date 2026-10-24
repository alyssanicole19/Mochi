#include "face.h"
#include <math.h>

namespace {
  constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }
  const uint16_t BG      = TFT_BLACK;
  const uint16_t C_CYAN  = rgb(80, 230, 230);
  const uint16_t C_PINK  = rgb(255, 110, 170);
  const uint16_t C_RED   = rgb(255, 90, 60);
  const uint16_t C_GOLD  = rgb(255, 200, 60);
  const uint16_t C_MINT  = rgb(120, 240, 170);
  const uint16_t C_LILAC = rgb(190, 150, 255);

  const int CX = 120;
  int CY = 108;                     // centro de la cara (se mueve con setOffsetY)
  float offY = 0, offYTarget = 0;
  const int EYE_GAP = 46;           // distancia del centro a cada ojo

  // Parámetros animables de un ojo
  struct EyeShape { float w, h, r; };

  Emotion current = Emotion::Neutral;
  EyeShape cur  = {56, 72, 20};
  EyeShape tgt  = {56, 72, 20};
  float lookX = 0, lookY = 0, lookTX = 0, lookTY = 0;
  float biasX = 0, biasY = 0;
  uint32_t nextSaccade = 0, nextBlink = 0, blinkStart = 0;
  bool blinking = false;
  uint32_t emotionSince = 0;

  float lerp(float a, float b, float t) { return a + (b - a) * t; }

  uint16_t colorFor(Emotion e) {
    switch (e) {
      case Emotion::Love:    return C_PINK;
      case Emotion::Angry:   return C_RED;
      case Emotion::Alert:   return C_GOLD;
      case Emotion::Relaxed: return C_MINT;
      case Emotion::Dizzy:   return C_LILAC;
      default:               return C_CYAN;
    }
  }

  EyeShape shapeFor(Emotion e) {
    switch (e) {
      case Emotion::Happy:     return {58, 60, 22};
      case Emotion::Love:      return {58, 60, 22};
      case Emotion::Angry:     return {58, 58, 14};
      case Emotion::Surprised: return {70, 70, 35};
      case Emotion::Sleepy:    return {56, 8,  4};
      case Emotion::Bored:     return {58, 30, 12};
      case Emotion::Focused:   return {58, 40, 14};
      case Emotion::Alert:     return {64, 80, 24};
      case Emotion::Relaxed:   return {58, 50, 20};
      default:                 return {56, 72, 20};
    }
  }

  void drawHeart(TFT_eSprite& s, int x, int y, int size, uint16_t c) {
    int r = size / 2;
    s.fillCircle(x - r, y - r / 2, r, c);
    s.fillCircle(x + r, y - r / 2, r, c);
    s.fillTriangle(x - size, y - r / 4, x + size, y - r / 4, x, y + size, c);
  }

  void drawSpiral(TFT_eSprite& s, int x, int y, float rot, uint16_t c) {
    float px = x, py = y;
    for (float t = 0; t < 4 * PI; t += 0.25f) {
      float r = 2.2f * t;
      float nx = x + r * cosf(t + rot);
      float ny = y + r * sinf(t + rot);
      s.drawWideLine(px, py, nx, ny, 4, c, BG);
      px = nx; py = ny;
    }
  }

  // Dibuja un ojo. side = -1 izquierdo, +1 derecho
  void drawEye(TFT_eSprite& s, int side, uint16_t col, uint32_t now) {
    int ex = CX + side * EYE_GAP + (int)lookX;
    int ey = CY + (int)lookY;

    if (current == Emotion::Dizzy) {
      float rot = now / 120.0f * side;
      drawSpiral(s, ex, ey, rot, col);
      return;
    }
    if (current == Emotion::Love && !blinking) {
      float pulse = 1.0f + 0.08f * sinf(now / 150.0f);
      drawHeart(s, ex, ey, (int)(24 * pulse), col);
      return;
    }

    float h = cur.h;
    if (blinking) {
      float t = (now - blinkStart) / 140.0f;           // 0..1 cerrar y abrir
      float k = t < 0.5f ? 1 - t * 2 : (t - 0.5f) * 2;
      h = max(4.0f, h * k);
    }
    int w = (int)cur.w, hh = (int)h;
    int r = min((int)cur.r, min(w, hh) / 2);
    s.fillRoundRect(ex - w / 2, ey - hh / 2, w, hh, r, col);

    switch (current) {
      case Emotion::Happy:
      case Emotion::Relaxed: {
        // ojos en forma de "^": recortamos la parte de abajo con un círculo negro
        if (hh > 20) s.fillCircle(ex, ey + hh * 0.62f, w * 0.62f, BG);
        break;
      }
      case Emotion::Angry: {
        // párpado inclinado hacia el centro
        int top = ey - hh / 2 - 2;
        int innerX = ex - side * (w / 2 + 2);
        int outerX = ex + side * (w / 2 + 2);
        s.fillTriangle(innerX, top, outerX, top, innerX, top + hh * 0.6f, BG);
        break;
      }
      case Emotion::Bored: {
        // párpado superior plano, mirada caída
        s.fillRect(ex - w / 2 - 2, ey - hh / 2 - 2, w + 4, hh / 3, BG);
        break;
      }
      case Emotion::Surprised: {
        // brillo en el ojo
        s.fillCircle(ex + 12, ey - 14, 7, BG);
        break;
      }
      default: break;
    }
  }

  void drawMouth(TFT_eSprite& s, uint16_t col, uint32_t now) {
    int mx = CX + (int)(lookX * 0.6f), my = CY + 52 + (int)(lookY * 0.6f);
    switch (current) {
      case Emotion::Happy:
      case Emotion::Love:
        // sonrisita
        // (0° = abajo en TFT_eSPI; dibujamos en dos tramos para cruzar 0°)
        s.drawSmoothArc(mx, my - 8, 12, 9, 300, 360, col, BG, true);
        s.drawSmoothArc(mx, my - 8, 12, 9, 0, 60, col, BG, true);
        break;
      case Emotion::Surprised:
        s.fillCircle(mx, my, 7, col);
        s.fillCircle(mx, my, 4, BG);
        break;
      case Emotion::Angry:
        s.drawWideLine(mx - 10, my + 2, mx + 10, my + 2, 4, col, BG);
        break;
      case Emotion::Dizzy: {
        // boquita ondulada
        int px = mx - 14, py = my;
        for (int i = 1; i <= 14; i++) {
          int nx = mx - 14 + i * 2;
          int ny = my + (int)(3 * sinf(i * 0.9f + now / 100.0f));
          s.drawWideLine(px, py, nx, ny, 3, col, BG);
          px = nx; py = ny;
        }
        break;
      }
      case Emotion::Sleepy: {
        // "z" flotando
        int dy = (now / 60) % 30;
        s.setTextColor(col, BG);
        s.setTextDatum(MC_DATUM);
        s.drawString("z", CX + 70, CY - 30 - dy, 4);
        s.drawString("z", CX + 88, CY - 52 - dy / 2, 2);
        break;
      }
      default: break;
    }
  }
}

namespace Face {

void begin() {
  nextBlink = millis() + 2500;
  nextSaccade = millis() + 1500;
}

void setEmotion(Emotion e) {
  if (e == current) return;
  current = e;
  tgt = shapeFor(e);
  emotionSince = millis();
}

Emotion emotion() { return current; }

void setLookBias(float x, float y) {
  biasX = constrain(x, -1.0f, 1.0f);
  biasY = constrain(y, -1.0f, 1.0f);
}

void setOffsetY(float y) { offYTarget = y; }

void forceBlink() { blinking = true; blinkStart = millis(); }

void render(TFT_eSprite& s, uint32_t now) {
  // animación suave hacia la forma objetivo
  cur.w = lerp(cur.w, tgt.w, 0.25f);
  cur.h = lerp(cur.h, tgt.h, 0.25f);
  cur.r = lerp(cur.r, tgt.r, 0.25f);

  // parpadeo (no cuando duerme o está mareado)
  bool canBlink = current != Emotion::Sleepy && current != Emotion::Dizzy;
  if (canBlink && !blinking && now > nextBlink) { blinking = true; blinkStart = now; }
  if (blinking && now - blinkStart > 140) {
    blinking = false;
    nextBlink = now + random(1800, 5500);
    if (random(6) == 0) nextBlink = now + 220;          // a veces doble parpadeo
  }

  // mirada: pequeños movimientos aleatorios + inclinación del robot
  if (now > nextSaccade) {
    bool calm = current == Emotion::Focused || current == Emotion::Sleepy;
    float range = calm ? 6 : 16;
    lookTX = random(-100, 101) / 100.0f * range;
    lookTY = random(-100, 101) / 100.0f * range * 0.6f;
    nextSaccade = now + random(1200, 4000);
  }
  lookX = lerp(lookX, lookTX + biasX * 18, 0.18f);
  lookY = lerp(lookY, lookTY + biasY * 12, 0.18f);

  offY = lerp(offY, offYTarget, 0.2f);
  CY = 108 + (int)offY;

  uint16_t col = colorFor(current);
  drawEye(s, -1, col, now);
  drawEye(s, +1, col, now);
  drawMouth(s, col, now);
}

}  // namespace Face
