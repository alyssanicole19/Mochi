// Pines fijos de la Waveshare ESP32-S3-Touch-LCD-1.28 (versión Touch)
#pragma once

#define PIN_I2C_SDA   6    // compartido: táctil CST816S + IMU QMI8658
#define PIN_I2C_SCL   7
#define PIN_TP_INT    5    // interrupción del táctil
#define PIN_TP_RST    13   // reset del táctil
#define PIN_IMU_INT1  4
#define PIN_LCD_BL    2    // luz de fondo (PWM)
#define PIN_BAT_ADC   1    // medición de batería

#define ADDR_CST816S  0x15
#define ADDR_QMI8658  0x6B  // algunas placas usan 0x6A (el código prueba ambas)
