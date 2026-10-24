# Conectar Mochi con Google Calendar

Mochi no inicia sesión en tu Google. En su lugar, un pequeño script que corre en
**tu** cuenta le entrega solo los eventos de las próximas 24 horas, y únicamente
si Mochi conoce tu token secreto.

## Pasos

1. Entra a <https://script.google.com> con la cuenta cuyo calendario quieres usar
   y pulsa **Nuevo proyecto**. Ponle de nombre `Mochi`.
2. Borra lo que aparece y pega el contenido de [`apps_script/Code.gs`](../apps_script/Code.gs).
3. Cambia `TOKEN` por una frase larga que solo tú sepas
   (por ejemplo `mochi-ama-los-mochis-2026-xq7`).
4. Prueba: elige la función **testMochi** y pulsa **Ejecutar**. Google te pedirá
   permiso para leer tu calendario: acéptalo. En el registro deberías ver tus eventos.
5. Publica: **Implementar → Nueva implementación → Tipo: Aplicación web**
   - *Ejecutar como:* **Yo**
   - *Quién tiene acceso:* **Cualquier usuario**
   (sin el token correcto, el script solo responde `unauthorized`)
6. Copia la **URL de la aplicación web** (termina en `/exec`).
7. En `include/config.h` pega esa URL en `CALENDAR_URL` y el mismo token en `CALENDAR_TOKEN`.

> **Cuenta de la universidad:** algunas universidades bloquean publicar Apps Scripts
> como "Cualquier usuario". Si te pasa, usa tu cuenta personal de Gmail y, desde tu
> calendario de la universidad, comparte tu calendario con ella (o suscríbete a él)
> y agrega su ID en `CALENDAR_IDS`.

## Si cambias el script

Cada vez que edites el código: **Implementar → Gestionar implementaciones → editar →
Versión: Nueva versión**. Así la URL sigue siendo la misma.
