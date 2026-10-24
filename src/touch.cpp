#include "touch.h"
#include "hw_pins.h"
#include <Wire.h>

namespace {
  bool wasTouching = false;
  uint32_t touchStart = 0;
  uint32_t lastTapEnd = 0;
  bool pendingTap = false;
  bool longFired = false;

  bool readRegs(uint8_t reg, uint8_t* buf, uint8_t len) {
    Wire.beginTransmission(ADDR_CST816S);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((int)ADDR_CST816S, (int)len) != len) return false;
    for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
  }
  void writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(ADDR_CST816S);
    Wire.write(reg); Wire.write(val);
    Wire.endTransmission();
  }
}

namespace Touch {

bool begin() {
  pinMode(PIN_TP_RST, OUTPUT);
  digitalWrite(PIN_TP_RST, LOW);  delay(10);
  digitalWrite(PIN_TP_RST, HIGH); delay(60);
  writeReg(0xFE, 0x01);           // desactiva el auto-sleep del táctil
  uint8_t id;
  return readRegs(0xA7, &id, 1);  // chip ID: si responde, está vivo
}

// Detectamos toque / doble toque / toque largo nosotros mismos,
// porque los gestos del chip no siempre son confiables.
TouchEvent read() {
  TouchEvent ev;
  uint8_t d[6];
  bool touching = false;
  if (readRegs(0x01, d, 6)) {
    touching = (d[1] & 0x0F) > 0;
    ev.x = ((d[2] & 0x0F) << 8) | d[3];
    ev.y = ((d[4] & 0x0F) << 8) | d[5];
    uint8_t g = d[0];
    if (g >= 0x01 && g <= 0x04) ev.gesture = (Gesture)g;   // deslizar
  }
  ev.touching = touching;
  uint32_t now = millis();

  if (touching && !wasTouching) { touchStart = now; longFired = false; }

  if (touching && !longFired && now - touchStart > 900) {
    ev.gesture = Gesture::LongPress;
    longFired = true;
    pendingTap = false;
  }

  if (!touching && wasTouching && !longFired && ev.gesture == Gesture::None) {
    if (pendingTap && now - lastTapEnd < 350) {
      ev.gesture = Gesture::DoubleTap;
      pendingTap = false;
    } else {
      pendingTap = true;
      lastTapEnd = now;
    }
  }

  // un toque simple se confirma cuando ya no puede ser doble
  if (pendingTap && !touching && now - lastTapEnd >= 350) {
    ev.gesture = Gesture::Tap;
    pendingTap = false;
  }

  wasTouching = touching;
  return ev;
}

}  // namespace Touch
