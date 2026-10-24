// ============================================================================
//  Mochi v1 — robot organizador con personalidad
//  Placa: Waveshare ESP32-S3-Touch-LCD-1.28
//
//  Gestos:
//    toque          → mimo (se pone feliz; si lo molestas mucho, se enoja)
//    doble toque    → muestra la hora y tu próximo evento
//    toque largo    → inicia / detiene el Pomodoro
//    girarlo        → se marea
//    sacudirlo      → se asusta
// ============================================================================
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <time.h>
#include <math.h>

#include "config.h"
#include "hw_pins.h"
#include "face.h"
#include "touch.h"
#include "imu.h"
#include "sound.h"
#include "calendar.h"

TFT_eSPI tft;
TFT_eSprite spr(&tft);

// ---------------------------------------------------------------- colores
constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
const uint16_t C_TEXT  = rgb(230, 240, 245);
const uint16_t C_DIM   = rgb(90, 110, 120);
const uint16_t C_RING  = rgb(80, 230, 230);
const uint16_t C_BREAK = rgb(120, 240, 170);
const uint16_t C_GOLD  = rgb(255, 200, 60);
const uint16_t C_TRACK = rgb(25, 40, 45);

// ---------------------------------------------------------------- estado
enum class Pomo { Idle, Focus, Break };
Pomo pomo = Pomo::Idle;
uint32_t pomoEndMs = 0, pomoTotalMs = 0;
int pomoRound = 0;

struct {
  float affection = 50;     // cariño: sube con mimos
  float annoyance = 0;      // fastidio: sube si lo tocas muy seguido
  float dizziness = 0;      // vueltas acumuladas
  uint32_t lastInteraction = 0;
} mood;

// Emoción temporal (reacción) encima de la emoción base
Emotion reaction = Emotion::Neutral;
uint32_t reactionUntil = 0;

// Pantallas superpuestas
enum class Overlay { None, Info, EventSoon, EventNow };
Overlay overlay = Overlay::None;
uint32_t overlayUntil = 0;
CalEvent overlayEvent;

CalEvent events[Calendar::MAX_EVENTS];
int nEvents = 0;

// avisos ya mostrados (para no repetirlos)
uint32_t alertedSoon[24], alertedNow[24];
uint8_t soonIdx = 0, nowIdx = 0;

uint8_t brightness = 255, brightnessTarget = 255;

// ---------------------------------------------------------------- utilidades
bool timeValid() { return time(nullptr) > 1700000000; }

void react(Emotion e, uint32_t ms) {
  reaction = e;
  reactionUntil = millis() + ms;
}

bool wasAlerted(uint32_t* list, uint32_t id) {
  for (int i = 0; i < 24; i++) if (list[i] == id) return true;
  return false;
}

void setBacklight(uint8_t v) { ledcWrite(1, v); }

String hhmm(time_t t) {
  struct tm tmv; localtime_r(&t, &tmv);
  int h = tmv.tm_hour % 12; if (h == 0) h = 12;
  char buf[12];
  snprintf(buf, sizeof(buf), "%d:%02d %s", h, tmv.tm_min, tmv.tm_hour < 12 ? "AM" : "PM");
  return String(buf);
}

// Texto centrado que se corta si no cabe en el círculo
void drawFit(const char* txt, int y, int font, uint16_t col, int maxW) {
  spr.setTextColor(col, TFT_BLACK);
  spr.setTextDatum(MC_DATUM);
  String s(txt);
  if (spr.textWidth(s, font) > maxW) {
    while (s.length() > 1 && spr.textWidth(s + "...", font) > maxW) s.remove(s.length() - 1);
    s += "...";
  }
  spr.drawString(s, 120, y, font);
}

// Anillo de progreso alrededor de la pantalla redonda (0..1)
void drawRing(float frac, uint16_t col) {
  spr.drawSmoothArc(120, 120, 119, 113, 0, 360, C_TRACK, TFT_BLACK, false);
  if (frac <= 0.005f) return;
  // en TFT_eSPI 0° está abajo; empezamos arriba (180°) y vamos en sentido horario
  int start = 180;
  int end = start + (int)(frac * 360);
  if (end <= 360) {
    spr.drawSmoothArc(120, 120, 119, 113, start, end, col, TFT_BLACK, true);
  } else {
    spr.drawSmoothArc(120, 120, 119, 113, start, 360, col, TFT_BLACK, false);
    spr.drawSmoothArc(120, 120, 119, 113, 0, end - 360, col, TFT_BLACK, true);
  }
}

// ---------------------------------------------------------------- Pomodoro
void startFocus() {
  pomo = Pomo::Focus;
  pomoTotalMs = (uint32_t)POMODORO_FOCUS_MIN * 60000UL;
  pomoEndMs = millis() + pomoTotalMs;
  Sound::play(Tune::FocusStart);
  react(Emotion::Happy, 1200);
}

void startBreak() {
  pomoRound++;
  bool longBreak = pomoRound % POMODORO_ROUNDS_BEFORE_LONG == 0;
  pomo = Pomo::Break;
  pomoTotalMs = (uint32_t)(longBreak ? POMODORO_LONG_BREAK_MIN : POMODORO_BREAK_MIN) * 60000UL;
  pomoEndMs = millis() + pomoTotalMs;
  Sound::play(Tune::FocusDone);
  react(Emotion::Love, 3000);
}

void stopPomodoro() {
  pomo = Pomo::Idle;
  Sound::play(Tune::BreakDone);
  react(Emotion::Relaxed, 1500);
}

void updatePomodoro() {
  if (pomo == Pomo::Idle) return;
  if ((int32_t)(millis() - pomoEndMs) < 0) return;
  if (pomo == Pomo::Focus) {
    startBreak();
  } else {
    pomo = Pomo::Idle;           // al terminar el descanso espera a que tú inicies otro
    Sound::play(Tune::BreakDone);
    react(Emotion::Happy, 2500);
  }
}

// ---------------------------------------------------------------- Calendario
void checkEvents() {
  static uint32_t lastCheck = 0;
  if (millis() - lastCheck < 1000) return;
  lastCheck = millis();

  nEvents = Calendar::snapshot(events);
  if (!timeValid()) return;
  time_t now = time(nullptr);

  for (int i = 0; i < nEvents; i++) {
    const CalEvent& e = events[i];
    if (e.allDay) continue;
    long secsTo = (long)(e.start - now);

    if (secsTo <= 0 && secsTo > -120 && !wasAlerted(alertedNow, e.id)) {
      alertedNow[nowIdx++ % 24] = e.id;
      alertedSoon[soonIdx++ % 24] = e.id;   // por si no hubo aviso previo
      overlay = Overlay::EventNow;
      overlayEvent = e;
      overlayUntil = millis() + 30000;
      Sound::play(Tune::EventNow);
      mood.lastInteraction = millis();
      return;
    }
    if (secsTo > 0 && secsTo <= ALERT_MINUTES_BEFORE * 60 && !wasAlerted(alertedSoon, e.id)) {
      alertedSoon[soonIdx++ % 24] = e.id;
      overlay = Overlay::EventSoon;
      overlayEvent = e;
      overlayUntil = millis() + 20000;
      Sound::play(Tune::EventSoon);
      mood.lastInteraction = millis();
      return;
    }
  }
}

const CalEvent* nextEvent() {
  time_t now = time(nullptr);
  for (int i = 0; i < nEvents; i++)
    if (!events[i].allDay && events[i].end > now) return &events[i];
  return nullptr;
}

// ---------------------------------------------------------------- entradas
void handleTouch() {
  TouchEvent t = Touch::read();
  if (t.gesture == Gesture::None) return;
  mood.lastInteraction = millis();

  // cualquier toque cierra un aviso
  if (overlay == Overlay::EventSoon || overlay == Overlay::EventNow) {
    overlay = Overlay::None;
    react(Emotion::Happy, 1500);
    Sound::play(Tune::Chirp);
    return;
  }

  switch (t.gesture) {
    case Gesture::Tap: {
      mood.annoyance += 18;
      mood.affection = min(100.0f, mood.affection + 6);
      if (mood.annoyance > 70) {
        react(Emotion::Angry, 2500);
        Sound::play(Tune::Grumble);
      } else if (mood.affection > 85 && random(3) == 0) {
        react(Emotion::Love, 2000);
        Sound::play(Tune::Chirp);
      } else {
        react(Emotion::Happy, 1800);
        Sound::play(Tune::Chirp);
      }
      break;
    }
    case Gesture::DoubleTap:
    case Gesture::SwipeUp:
      overlay = (overlay == Overlay::Info) ? Overlay::None : Overlay::Info;
      overlayUntil = millis() + 7000;
      break;
    case Gesture::LongPress:
      if (pomo == Pomo::Idle) startFocus();
      else                    stopPomodoro();
      break;
    default: break;
  }
}

void handleMotion(float dt) {
  ImuData d;
  if (!Imu::read(d)) return;

  // girar → acumula "vueltas"
  float spin = sqrtf(d.gx * d.gx + d.gy * d.gy + d.gz * d.gz);   // °/s
  if (spin > 120) {
    mood.dizziness += spin * dt / 360.0f;
    mood.lastInteraction = millis();
  }
  mood.dizziness = max(0.0f, mood.dizziness - 0.35f * dt);
  mood.dizziness = min(mood.dizziness, 6.0f);

  static bool wasDizzy = false;
  bool dizzy = mood.dizziness > 1.2f || (wasDizzy && mood.dizziness > 0.3f);
  if (dizzy && !wasDizzy) Sound::play(Tune::Dizzy);
  wasDizzy = dizzy;

  // sacudida → susto
  float g = sqrtf(d.ax * d.ax + d.ay * d.ay + d.az * d.az);
  static uint32_t lastShake = 0;
  if (fabsf(g - 1.0f) > 1.3f && millis() - lastShake > 1500 && !dizzy) {
    lastShake = millis();
    react(Emotion::Surprised, 1500);
    mood.lastInteraction = millis();
  }

  // inclinación → la mirada "se resbala" hacia abajo
  Face::setLookBias(-d.ax * 1.5f, d.ay * 1.5f);
}

// ---------------------------------------------------------------- emoción base
Emotion baseEmotion() {
  if (mood.dizziness > 0.3f && (mood.dizziness > 1.2f || Face::emotion() == Emotion::Dizzy))
    return Emotion::Dizzy;
  if (overlay == Overlay::EventSoon || overlay == Overlay::EventNow) return Emotion::Alert;
  if (millis() < reactionUntil) return reaction;
  if (pomo == Pomo::Focus) return Emotion::Focused;
  if (pomo == Pomo::Break) return Emotion::Relaxed;

  uint32_t idle = millis() - mood.lastInteraction;
  if (idle > 10UL * 60000UL) return Emotion::Sleepy;
  if (idle > 3UL * 60000UL)  return Emotion::Bored;
  return Emotion::Neutral;
}

// ---------------------------------------------------------------- dibujo
void drawOverlay() {
  if (overlay != Overlay::None && millis() > overlayUntil) overlay = Overlay::None;

  switch (overlay) {
    case Overlay::Info: {
      // reloj y próximo evento debajo de los ojos
      if (timeValid()) drawFit(hhmm(time(nullptr)).c_str(), 172, 4, C_TEXT, 170);
      else             drawFit("sin hora aun", 172, 2, C_DIM, 170);
      const CalEvent* e = nextEvent();
      if (e) {
        String line = hhmm(e->start) + "  " + e->title;
        drawFit(line.c_str(), 200, 2, C_RING, 150);
      } else {
        drawFit(Calendar::hasData() ? "nada pendiente :)" : "conectando...", 200, 2, C_DIM, 150);
      }
      break;
    }
    case Overlay::EventSoon: {
      long mins = (long)(overlayEvent.start - time(nullptr) + 59) / 60;
      char buf[24]; snprintf(buf, sizeof(buf), "en %ld min", max(1L, mins));
      drawFit(buf, 172, 4, C_GOLD, 170);
      drawFit(overlayEvent.title, 200, 2, C_TEXT, 150);
      break;
    }
    case Overlay::EventNow: {
      drawFit("ahora!", 172, 4, C_GOLD, 170);
      drawFit(overlayEvent.title, 200, 2, C_TEXT, 150);
      // anillo dorado que parpadea
      if ((millis() / 400) % 2) drawRing(1.0f, C_GOLD);
      break;
    }
    default: break;
  }
}

void drawPomodoroHud() {
  if (pomo == Pomo::Idle) return;
  int32_t left = (int32_t)(pomoEndMs - millis());
  if (left < 0) left = 0;
  float frac = (float)left / (float)pomoTotalMs;
  uint16_t col = pomo == Pomo::Focus ? C_RING : C_BREAK;
  if (overlay != Overlay::EventNow) drawRing(frac, col);

  if (overlay == Overlay::None) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d:%02d", left / 60000, (left / 1000) % 60);
    drawFit(buf, 186, 4, col, 160);
    char r[20];
    snprintf(r, sizeof(r), pomo == Pomo::Focus ? "pomodoro %d" : "descanso",
             pomoRound % POMODORO_ROUNDS_BEFORE_LONG + 1);
    drawFit(r, 210, 2, C_DIM, 120);
  }
}

// ---------------------------------------------------------------- setup / loop
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nMochi v1 despertando...");

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  // luz de fondo con PWM (canal 1; el canal 0 es del buzzer).
  // Va después de tft.init() porque init() reconfigura ese pin.
  ledcSetup(1, 5000, 8);
  ledcAttachPin(PIN_LCD_BL, 1);
  setBacklight(255);

  spr.setColorDepth(16);
  if (!spr.createSprite(240, 240)) {
    Serial.println("ERROR: no hay memoria para el sprite");
  }

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  if (!Touch::begin()) Serial.println("Aviso: no encuentro el tactil (CST816S)");
  if (!Imu::begin())   Serial.println("Aviso: no encuentro el IMU (QMI8658)");

  Sound::begin();
  Face::begin();
  randomSeed(esp_random());

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  configTzTime(TIMEZONE, "pool.ntp.org", "time.google.com");
  Calendar::begin();

  mood.lastInteraction = millis();
  react(Emotion::Happy, 2000);
  Sound::play(Tune::Hello);
}

void loop() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();
  float dt = (now - lastFrame) / 1000.0f;
  if (dt < 0.033f) { Sound::update(); return; }      // ~30 cuadros por segundo
  lastFrame = now;
  if (dt > 0.2f) dt = 0.2f;

  Sound::update();
  handleTouch();
  handleMotion(dt);
  updatePomodoro();
  checkEvents();

  // el fastidio se va pasando
  mood.annoyance = max(0.0f, mood.annoyance - 12.0f * dt);
  mood.affection = max(20.0f, mood.affection - 0.05f * dt);

  Emotion e = baseEmotion();
  Face::setEmotion(e);
  bool textBelow = overlay != Overlay::None || pomo != Pomo::Idle;
  Face::setOffsetY(textBelow ? -18 : 0);

  // brillo: se apaga un poco cuando duerme
  brightnessTarget = (e == Emotion::Sleepy) ? 40 : 255;
  if (brightness != brightnessTarget) {
    brightness += (brightnessTarget > brightness) ? min(15, brightnessTarget - brightness)
                                                  : -min(5, brightness - brightnessTarget);
    setBacklight(brightness);
  }

  spr.fillSprite(TFT_BLACK);
  Face::render(spr, now);
  drawPomodoroHud();
  drawOverlay();
  spr.pushSprite(0, 0);
}
