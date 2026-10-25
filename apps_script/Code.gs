/**
 * Mochi — puente con Google Calendar
 *
 * Este script corre en TU cuenta de Google y le entrega a Mochi solo
 * los eventos de las próximas 24 horas (título, inicio, fin).
 * Instrucciones paso a paso: docs/google-calendar.md
 */

// 1) Cambia esto por una frase larga y secreta (la misma que en config.h)
const TOKEN = 'cambia-esto-por-algo-largo-y-secreto';

// 2) Calendarios a revisar.
//    'all'  = todos los que tienes marcados como visibles en Google Calendar
//             (menos los de feriados).
//    También puedes poner una lista de IDs, p. ej. ['primary', 'id-de-otro@group.calendar.google.com']
const CALENDAR_IDS = 'all';

const HOURS_AHEAD = 24;
const MAX_EVENTS = 8;

function doGet(e) {
  if (!e || !e.parameter || e.parameter.token !== TOKEN) {
    return json_({ error: 'unauthorized' });
  }

  const now = new Date();
  const until = new Date(now.getTime() + HOURS_AHEAD * 3600 * 1000);
  let events = [];
  const seen = {};

  calendars_().forEach(function (cal) {
    cal.getEvents(now, until).forEach(function (ev) {
      // omite eventos que rechazaste
      try {
        if (ev.getMyStatus() === CalendarApp.GuestStatus.NO) return;
      } catch (err) { /* eventos sin invitados */ }
      // el mismo evento puede estar en dos calendarios: lo mostramos una vez
      const key = ev.getTitle() + '|' + ev.getStartTime().getTime();
      if (seen[key]) return;
      seen[key] = true;
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

function calendars_() {
  if (CALENDAR_IDS === 'all') {
    return CalendarApp.getAllCalendars().filter(function (cal) {
      const id = cal.getId();
      const isHoliday = id.indexOf('#holiday') !== -1 || id.indexOf('holiday@') !== -1;
      return !isHoliday && !cal.isHidden() && cal.isSelected();
    });
  }
  return CALENDAR_IDS.map(function (id) {
    return id === 'primary' ? CalendarApp.getDefaultCalendar() : CalendarApp.getCalendarById(id);
  }).filter(function (cal) { return cal; });
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
  Logger.log('Calendarios: ' + calendars_().map(function (c) { return c.getName(); }).join(', '));
  const out = doGet({ parameter: { token: TOKEN } });
  Logger.log(out.getContent());
}

// Igual que testMochi, pero mira los próximos 7 días (solo para comprobar).
function testMochiSemana() {
  const now = new Date();
  const until = new Date(now.getTime() + 7 * 24 * 3600 * 1000);
  calendars_().forEach(function (cal) {
    cal.getEvents(now, until).forEach(function (ev) {
      Logger.log(cal.getName() + ' → ' + clean_(ev.getTitle()) + ' · ' + ev.getStartTime());
    });
  });
}
