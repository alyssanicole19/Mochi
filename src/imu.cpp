#include "imu.h"
#include "hw_pins.h"
#include <Wire.h>

namespace {
  uint8_t addr = ADDR_QMI8658;

  // Registros del QMI8658
  const uint8_t REG_WHO_AM_I = 0x00;   // debe responder 0x05
  const uint8_t REG_CTRL1    = 0x02;
  const uint8_t REG_CTRL2    = 0x03;   // acelerómetro
  const uint8_t REG_CTRL3    = 0x04;   // giroscopio
  const uint8_t REG_CTRL7    = 0x08;   // habilitar sensores
  const uint8_t REG_AX_L     = 0x35;   // 12 bytes: ax ay az gx gy gz
  const uint8_t REG_RESET    = 0x60;

  const float ACC_LSB_PER_G   = 8192.0f;  // rango ±4 g
  const float GYR_LSB_PER_DPS = 64.0f;    // rango ±512 °/s

  bool writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg); Wire.write(val);
    return Wire.endTransmission() == 0;
  }
  bool readRegs(uint8_t reg, uint8_t* buf, uint8_t len) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((int)addr, (int)len) != len) return false;
    for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
  }
  bool probe(uint8_t a) {
    addr = a;
    uint8_t id = 0;
    return readRegs(REG_WHO_AM_I, &id, 1) && id == 0x05;
  }
}

namespace Imu {

bool begin() {
  if (!probe(0x6B) && !probe(0x6A)) return false;
  writeReg(REG_RESET, 0xB0);  delay(20);
  writeReg(REG_CTRL1, 0x40);  // auto-incremento de dirección, little endian
  writeReg(REG_CTRL2, 0x15);  // acel: ±4 g, ~250 Hz
  writeReg(REG_CTRL3, 0x55);  // giro: ±512 °/s, ~250 Hz
  writeReg(REG_CTRL7, 0x03);  // enciende acelerómetro y giroscopio
  delay(30);
  return true;
}

bool read(ImuData& o) {
  uint8_t b[12];
  if (!readRegs(REG_AX_L, b, 12)) return false;
  auto s16 = [&](int i) { return (int16_t)((b[i + 1] << 8) | b[i]); };
  o.ax = s16(0) / ACC_LSB_PER_G;
  o.ay = s16(2) / ACC_LSB_PER_G;
  o.az = s16(4) / ACC_LSB_PER_G;
  o.gx = s16(6) / GYR_LSB_PER_DPS;
  o.gy = s16(8) / GYR_LSB_PER_DPS;
  o.gz = s16(10) / GYR_LSB_PER_DPS;
  return true;
}

}  // namespace Imu
