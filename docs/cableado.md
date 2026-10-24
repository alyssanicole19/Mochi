# Cableado de Mochi v1

La placa **Waveshare ESP32-S3-Touch-LCD-1.28** ya trae conectados la pantalla
redonda, el táctil, el IMU y el cargador de batería. Solo faltan dos cosas.

## 1. Batería (conector MX1.25 de 2 pines)

| Batería | Placa |
|---|---|
| Rojo (+) | `+` del conector de batería |
| Negro (−) | `−` del conector de batería |

⚠️ **Revisa la polaridad antes de enchufar.** Muchas baterías genéricas traen el
conector al revés. Compara los colores con las marcas `+`/`−` serigrafiadas junto al
conector de la placa. Si no coinciden, no la conectes: se puede cambiar el orden de los
cables en el conector con una aguja, o pedir otra batería.

La batería se carga sola cuando Mochi está conectado por USB-C.

## 2. Buzzer pasivo (opcional)

El buzzer se conecta al conector pequeño **SH1.0 de 12 pines** de la placa, usando el
cable que viene en la caja.

| Buzzer | Placa |
|---|---|
| VCC | 3V3 |
| GND | GND |
| I/O (señal) | un GPIO libre del conector SH1.0 |

En `include/config.h` pon ese número en `BUZZER_PIN` (por defecto `16`).

> **Por confirmar:** el número exacto de cada pin del conector SH1.0 está en el
> diagrama de pines de la wiki de Waveshare. Cuando tengas la placa en mano,
> comparamos el cable con ese diagrama antes de soldar nada.

## Pines internos (ya conectados, solo como referencia)

| Función | GPIO |
|---|---|
| Pantalla GC9A01 (SPI): SCLK / MOSI / CS / DC / RST | 10 / 11 / 9 / 8 / 14 |
| Luz de fondo | 2 |
| I²C (táctil + IMU): SDA / SCL | 6 / 7 |
| Táctil CST816S: INT / RST | 5 / 13 |
| IMU QMI8658: INT1 | 4 |
| Medición de batería | 1 |
