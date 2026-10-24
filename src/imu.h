// IMU QMI8658 (acelerómetro + giroscopio) — driver mínimo
#pragma once
#include <Arduino.h>

struct ImuData {
  float ax, ay, az;   // en g
  float gx, gy, gz;   // en grados/segundo
};

namespace Imu {
  bool begin();
  bool read(ImuData& out);
}
