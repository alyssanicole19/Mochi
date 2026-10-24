# Mochi 🍡

**Mochi es un pequeño robot de escritorio que te ayuda a organizarte… con personalidad.**
Te avisa de tus eventos de Google Calendar, te acompaña en tus Pomodoros, se pone feliz
cuando lo tocas, se enoja si lo molestas demasiado y se marea si lo haces girar.

*Mochi is a tiny desk robot that keeps you organized — with personality. English below.*

---

## ¿Qué hace?

| Lo que haces | Lo que hace Mochi |
|---|---|
| Nada (tienes un evento pronto) | 10 min antes: ojos dorados, suena un aviso y muestra el evento. Al empezar: "¡ahora!" |
| Toque | Se pone feliz 😊 (o se enamora 💗 si lo mimas seguido) |
| Muchos toques seguidos | Se enoja 😠 |
| Doble toque | Muestra la hora y tu próximo evento |
| Toque largo | Inicia / detiene un Pomodoro (25 min + 5 de descanso, cada 4 uno largo) |
| Girarlo | Se marea 😵‍💫 (ojos en espiral) |
| Sacudirlo | Se asusta 😮 |
| Ignorarlo 3 min / 10 min | Se aburre / se duerme 😴 (y baja el brillo) |

## Materiales

- [Waveshare ESP32-S3-Touch-LCD-1.28](https://www.waveshare.com/product/mcu-tools/esp32-s3-touch-lcd-1.28.htm) (versión **Touch**)
- Batería LiPo 3.7 V, 400–1000 mAh, conector MX1.25 (JST 1.25 mm, 2 pines)
- Cable USB-C de datos
- Opcional: buzzer pasivo 3.3–5 V

Cableado: [docs/cableado.md](docs/cableado.md)

## Instalación

1. Instala [VS Code](https://code.visualstudio.com/) y la extensión **PlatformIO IDE**.
2. Descarga este repositorio y ábrelo en VS Code.
3. Copia `include/config.example.h` como `include/config.h` y llena tu Wi-Fi.
4. Conecta Mochi con Google Calendar: [docs/google-calendar.md](docs/google-calendar.md).
5. Conecta la placa por USB-C y pulsa **Upload** (→) en la barra de PlatformIO.

Si la placa no aparece: mantén **BOOT**, pulsa **RESET**, suelta **BOOT** e intenta de nuevo.

## Estructura

```
src/
  main.cpp      estados, Pomodoro, avisos y personalidad
  face.cpp      la carita y sus emociones
  touch.cpp     táctil CST816S
  imu.cpp       acelerómetro / giroscopio QMI8658
  sound.cpp     melodías del buzzer
  calendar.cpp  lee tus eventos (en segundo plano)
apps_script/Code.gs   puente seguro con Google Calendar
docs/                 cableado y guías
```

## Seguridad

- `include/config.h` (Wi-Fi y token) **no se sube** a GitHub.
- El script de Calendar solo responde con el token correcto y solo envía título y horario de las próximas 24 h.
- En la v1 Mochi no verifica el certificado HTTPS de Google (para simplificar). Mejora pendiente para la v2.

## Hoja de ruta

- [x] v1: caritas, Pomodoro, avisos de Calendar, mareo, personalidad, buzzer
- [ ] v2: voz con amplificador MAX98357A y parlante
- [ ] Indicador de batería
- [ ] Case 3D 

---

## English

Mochi is a small ESP32-S3 desk companion with a round 1.28" display. It shows
expressive animated eyes, reminds you of upcoming Google Calendar events, runs
Pomodoro sessions (long-press), reacts to taps, gets dizzy when spun (IMU) and
falls asleep when ignored.

**Setup:** install PlatformIO, copy `include/config.example.h` to `include/config.h`,
fill in Wi-Fi, deploy the Apps Script in `apps_script/Code.gs` (see
`docs/google-calendar.md`), then build & upload.

## Licencia / License

MIT — haz tu propio Mochi. Make your own Mochi.
