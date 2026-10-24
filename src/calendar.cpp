#include "calendar.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace {
  CalEvent shared[Calendar::MAX_EVENTS];
  int sharedN = 0;
  volatile bool gotData = false;
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

  uint32_t fnv1a(const char* s) {
    uint32_t h = 2166136261u;
    while (*s) { h ^= (uint8_t)*s++; h *= 16777619u; }
    return h;
  }

  bool fetch() {
    if (WiFi.status() != WL_CONNECTED) return false;

    WiFiClientSecure client;
    // Versión 1: no verificamos el certificado de Google para simplificar.
    // El token protege tu calendario; ver README → "Seguridad".
    client.setInsecure();

    HTTPClient http;
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);  // Apps Script redirige
    http.setTimeout(15000);
    String url = String(CALENDAR_URL) + "?token=" + CALENDAR_TOKEN;
    if (!http.begin(client, url)) return false;

    int code = http.GET();
    if (code != 200) {
      Serial.printf("[calendar] HTTP %d\n", code);
      http.end();
      return false;
    }
    String body = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) { Serial.printf("[calendar] JSON: %s\n", err.c_str()); return false; }
    if (doc["error"].is<const char*>()) {
      Serial.printf("[calendar] script: %s\n", doc["error"].as<const char*>());
      return false;
    }

    CalEvent tmp[Calendar::MAX_EVENTS];
    int n = 0;
    for (JsonObject e : doc["events"].as<JsonArray>()) {
      if (n >= Calendar::MAX_EVENTS) break;
      CalEvent& ce = tmp[n];
      strlcpy(ce.title, e["title"] | "(sin titulo)", sizeof(ce.title));
      ce.start  = (time_t)e["start"].as<long>();
      ce.end    = (time_t)e["end"].as<long>();
      ce.allDay = e["allDay"] | false;
      char key[64];
      snprintf(key, sizeof(key), "%s|%ld", ce.title, (long)ce.start);
      ce.id = fnv1a(key);
      n++;
    }
    for (int i = 1; i < n; i++)                         // ordenar por inicio
      for (int j = i; j > 0 && tmp[j].start < tmp[j - 1].start; j--)
        std::swap(tmp[j], tmp[j - 1]);

    portENTER_CRITICAL(&mux);
    memcpy(shared, tmp, sizeof(CalEvent) * n);
    sharedN = n;
    portEXIT_CRITICAL(&mux);
    gotData = true;
    Serial.printf("[calendar] %d eventos\n", n);
    return true;
  }

  void task(void*) {
    for (;;) {
      bool ok = fetch();
      // si falla, reintenta en 30 s; si no, espera el intervalo normal
      uint32_t waitMs = ok ? (uint32_t)CALENDAR_REFRESH_MIN * 60000UL : 30000UL;
      vTaskDelay(pdMS_TO_TICKS(waitMs));
    }
  }
}

namespace Calendar {

void begin() {
  xTaskCreatePinnedToCore(task, "calendar", 16384, nullptr, 1, nullptr, 0);
}

int snapshot(CalEvent* out) {
  portENTER_CRITICAL(&mux);
  int n = sharedN;
  memcpy(out, shared, sizeof(CalEvent) * n);
  portEXIT_CRITICAL(&mux);
  return n;
}

bool hasData() { return gotData; }

}  // namespace Calendar
