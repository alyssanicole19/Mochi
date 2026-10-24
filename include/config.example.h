// ============================================================================
//  Configuración de Mochi
//  1) Copia este archivo y renómbralo a  config.h  (en la misma carpeta)
//  2) Llena tus datos. config.h NO se sube a GitHub (está en .gitignore),
//     así tu contraseña de Wi-Fi y tu token quedan privados.
// ============================================================================
#pragma once

// ---------- Wi-Fi (solo redes de 2.4 GHz) ----------
#define WIFI_SSID       "nombre-de-tu-wifi"
#define WIFI_PASSWORD   "tu-contrasena"

// ---------- Google Calendar (ver docs/google-calendar.md) ----------
// URL de tu Apps Script publicado (termina en /exec)
#define CALENDAR_URL    "https://script.google.com/macros/s/XXXXXXXX/exec"
// El mismo token secreto que pusiste en el script
#define CALENDAR_TOKEN  "cambia-esto-por-algo-largo-y-secreto"

// ---------- Zona horaria ----------
// Nueva York / Baltimore (con horario de verano automático)
#define TIMEZONE        "EST5EDT,M3.2.0,M11.1.0"

// ---------- Avisos ----------
#define ALERT_MINUTES_BEFORE   10   // aviso previo a cada evento
#define CALENDAR_REFRESH_MIN    5   // cada cuántos minutos revisa el calendario

// ---------- Pomodoro ----------
#define POMODORO_FOCUS_MIN     25
#define POMODORO_BREAK_MIN      5
#define POMODORO_LONG_BREAK_MIN 15
#define POMODORO_ROUNDS_BEFORE_LONG 4

// ---------- Sonido ----------
// Pin del buzzer pasivo (conector SH1.0 de la placa). Verifica en docs/cableado.md.
// Pon -1 si todavía no tienes buzzer.
#define BUZZER_PIN     16
#define SOUND_ENABLED  1
