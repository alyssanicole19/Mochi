/**
 * Mochi — puente con Google Calendar
 *
 * Este script corre en TU cuenta de Google y le entrega a Mochi solo
 * los eventos de las próximas 24 horas (título, inicio, fin).
 * Instrucciones paso a paso: docs/google-calendar.md
 */

// 1) Cambia esto por una frase larga y secreta (la misma que en config.h)
const TOKEN = 'cambia-esto-por-algo-largo-y-secreto';

// 2) Calendarios a revisar. 'primary' = tu calendario principal.
//    Puedes agregar otros por su ID (Configuración del calendario → "ID del calendario").
const CALENDAR_IDS = ['primary'];

const HOURS_AHEAD = 24;
const MAX_EVENTS = 8;

function doGet(e) {
  if (!e || !e.parameter || e.parameter.token !== TOKEN) {
    return json_({ error: 'unauthorized' });
  }

  const now = new Date();
  const until = new Date(now.getTime() + HOURS_AHEAD * 3600 * 1000);
  let events = [];

  CALENDAR_IDS.forEach(function (id) {
    const cal = id === 'primary' ? CalendarApp.getDefaultCalendar() : CalendarApp.getCalendarById(id);
    if (!cal) return;
    cal.getEvents(now, until).forEach(function (ev) {
      // omite eventos que rechazaste
      try {
        if (ev.getMyStatus() === CalendarApp.GuestStatus.NO) return;
      } catch (err) { /* eventos sin invitados */ }
      events.push({
        title: clean_(ev.getTitle()),
        start: Math.floor(ev.getStartTime().getTime() / 1000),
        end: Math.floor(ev.getEndTime().getTime() / 1000),
        allDay: ev.isAllDayEvent()
      });
    });
  });

  events.sort(function (a, b) { return a.start - b.start; });
  return json_({ events: events.slice(0, MAX_EVENTS) });
}

// La pantalla de Mochi no tiene tildes ni ñ: "Reunión año" → "Reunion ano"
function clean_(s) {
  return (s || '(sin titulo)')
    .normalize('NFD').replace(/[̀-ͯ]/g, '')
    .replace(/[^\x20-\x7E]/g, '')
    .trim()
    .substring(0, 46);
}

function json_(obj) {
  return ContentService.createTextOutput(JSON.stringify(obj))
    .setMimeType(ContentService.MimeType.JSON);
}

// Para probar desde el editor: Ejecutar → testMochi, y mira el registro.
function testMochi() {
  const out = doGet({ parameter: { token: TOKEN } });
  Logger.log(out.getContent());
}
