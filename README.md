# VHud Radio Wheel 2.32.0-beta

Rueda de radio estilo GTA V para GTA SA Android 2.10 (AML .so).
Repo aparte: Gomezsl/VHudRadio-aml-mod. ARCX no se toca.

- Los iconos van INCRUSTADOS en el .so (no hace falta ninguna carpeta):
  los 28 PNG (14 normales + 14 a color) están dentro del .so en base64 y se
  cargan de memoria al arrancar. La carpeta `VHudRadio/radio/` (y
  `VHudRadio/radio_color/` para los de color) ya es opcional: solo sirve
  para reemplazar un icono por uno propio. Si el PNG está en la carpeta,
  manda el de la carpeta; si no, se usa el incrustado. El log dice de dónde
  salió cada icono ("archivo" o "en el .so"). Antes de la v2.29 los iconos
  se leían solo de la carpeta (primero `files/VHudRadio/radio/`, luego
  `files/vhud/radio/` y luego `/storage/emulated/0/vhud/radio/`).
- Dos juegos de iconos de las emisoras (normales y a color), sin reemplazar
  archivos: los normales (blancos) viven en `VHudRadio/radio/` y los de color
  en `VHudRadio/radio_color/` (mismos nombres de archivo, mismos respaldos
  de carpeta). En el panel hay un INTERRUPTOR "Iconos de las emisoras a
  color": encendido se ven los de color (en la rueda y en el botón flotante),
  apagado se ven los normales. Se guarda como `iconosColor=0/1` en el .ini.
  Si a un icono a color le falta, se usa el normal de esa emisora.
- v2.30: menos trabajo en cada frame para que el mod de User Tracks no se
  trabe. La rueda hacia el frame de ImGui en CADA frame (para el botón
  flotante) y eso le quitaba CPU al decodificador de MP3 por software del
  mod SAUserTracks, así que la canción se escuchaba entrecortada. Ahora hay
  dos interruptores nuevos en el panel: "Cámara lenta al abrir la rueda"
  (`camaraLenta=0/1`, por defecto prendida) y "Mostrar botón flotante"
  (`botonVisible=0/1`, por defecto prendido). Con el botón oculto y la
  rueda/panel cerrados, la rueda se salta el frame de ImGui entero. El
  diagnóstico de audio del coche ya no corre en cada frame, solo al entrar
  al coche. La rueda se sigue abriendo con doble toque al claxon aunque el
  botón esté oculto. La v2.31 añade que tocar el centro de la rueda abre el
  panel, para que ocultar el botón nunca te deje sin ajustes.
- v2.32: al cambiar de estación se restaura la escala de tiempo a 1.00
  ANTES de llamar StartRadio. El mod de User Tracks (SAUserTracks) arranca
  su reproductor con StartRadio; si lo hacía con la cámara lenta puesta
  (0.30), su decodificador MP3 arrancaba a ritmo lento y la canción se
  escuchaba trabada aunque la rueda ya se hubiera cerrado. La cámara lenta
  sigue al abrir y al recorrer la rueda, solo se quita al elegir estación
  (que es cuando la rueda se cierra de todos modos).
- Rueda de 13 estaciones en el anillo (apagar la radio va arriba del anillo).
  Soltar sobre un segmento cambia la estación. Doble toque al claxon abre/cierra.
  Tocar el CENTRO de la rueda (el nombre de la estación) y soltar abre el
  panel de personalización: así se llega a los ajustes aunque el botón
  flotante esté oculto. Tocar fuera del anillo cierra la rueda sin cambiar.
- Botón flotante con etiqueta (nombre + género de la estación):
  un toque = siguiente estación, dos toques = panel de personalización.
  Se puede ARRASTRAR con el dedo a cualquier lugar de la pantalla (X e Y);
  la etiqueta cambia de lado si queda a la izquierda. En el panel se cambia
  su tamaño (50-150%), su opacidad (30-100%), se bloquea su posición y se
  restablece a la esquina de abajo a la derecha.
- Sonido: clic al abrir la rueda y al seleccionar una estación (OpenSL ES).
- Panel de personalización propio (azul casi oscuro, sin barra de título
  de ImGui, deslizadores, recuadros y botones dibujados a mano): oscuridad
  del fondo, tamaño de la rueda y tres recuadros de color (anillo, texto del
  centro y fondo de la rueda); al tocar un recuadro se abre el panel RGB
  táctil (cuadro de saturación/valor + barra de tono, con valores R G B y
  hex). También: idioma, tamaño y opacidad del botón, bloqueo de su
  posición, vista previa (solo se ve) y restablecer todo. Se guarda en
  `Android/data/com.rockstargames.gtasa/files/VHudRadio/VHudRadio.ini`.
  Si existe un .ini viejo en `data/configs/` o en `files/configs/`, se muda solo.
- La cabecera del panel ("Personalizar rueda de radio" + subtítulo) es FIJA
  (como la del menú de ARCX): no se desplaza con el contenido, que se desliza
  por debajo, y lleva fondo opaco para que el contenido no se vea por detrás
  del título al bajar. Para cerrar ya no hay que bajar hasta "Cerrar": hay
  una X arriba a la derecha (círculo oscuro con anillo azul y X blanca, como
  en ARCX), centrada en la línea del título. El botón "Cerrar" de abajo ya no
  existe.
- Los botones ya no son simples rectángulos de ImGui: tienen esquinas
  redondeadas, brillo superior y borde claro, como las filas de ARCX. Cada
  ajuste lleva su icono a la izquierda (sol, expandir, globo, opacidad) y el
  valor en una pastilla a la derecha. "Bloquear posición del botón" es un
  INTERRUPTOR (pastilla con círculo, como los de ARCX), no un botón con
  ON/OFF. Los botones van en rejilla de 2 columnas con icono (restablecer
  posición y vista previa arriba, restablecer todo abajo).
- El panel se puede desplazar (deslizar) aunque el selector de color esté
  abierto, salvo si el arrastre empieza sobre el cuadro de color o la barra
  de tono (esos arrastres son del color). Antes, con el selector abierto no
  se podía bajar a los ajustes de abajo.
- La barra de tono ya no se regresa sola: con un color gris/blanco (como el
  texto blanco por defecto) el tono no se ve, y al soltar volvía a rojo
  porque un gris no tiene tono. Ahora el tono elegido se conserva, y al
  mover la barra con un color gris el color se pone saturado para que el
  cambio se vea.
- Crédito al final del panel: "rueda de radio GTAV de Hunter09" (abajo del
  todo, centrado, como el "By Hunter09" de ARCX).
- Las barras (deslizadores) ya no se regresan a su mismo lugar al tocar otra
  parte del panel: el valor guardado para restaurar si el arrastre era un
  desplazamiento se olvidaba nunca, así que un desplazamiento posterior
  restauraba la última barra ajustada. Ahora se olvida al soltar el dedo.
- El log (VHudRadioLog.txt) ahora se crea en files/VHudRadio/, junto al
  .ini, en vez de en files/ (junto al juego).
- El crédito "rueda de radio GTAV de Hunter09" cambia con el idioma elegido
  (español, inglés, indonesio, portugués, francés, alemán), como el resto
  del panel.
- Ya no se crean las carpetas viejas vhud/ al arrancar el juego (solo se
  crea VHudRadio/). Las carpetas viejas se siguen leyendo como respaldo si
  existen (por si alguien aún tiene ahí los iconos), pero ya no se crean
  vacías.
- El panel se puede MOVER (arrastrando desde la cabecera, como el menú de ARCX)
  y cambiar de TAMAÑO en ancho y en alto (desde la esquina inferior derecha,
  con tres rayas en diagonal, como en ARCX; la esquina no se pierde aunque el
  contenido sea más alto que el panel). La posición y el tamaño se guardan en
  el .ini (`panelX`/`panelY`/`panelAncho`/`panelAlto`). Por defecto es MEDIANO
  (0.44 de ancho x alto automático) para que se vean casi todos los ajustes;
  si se hace más chico, se DESLIZA con el dedo por DIRECCIÓN, como el menú de
  ARCX: un arrastre más vertical que horizontal desplaza el contenido (así se
  llega a los ajustes de abajo aunque el arrastre empiece sobre un deslizador,
  que no se mueve), y un arrastre más horizontal ajusta el deslizador. Solo
  responde a toques DENTRO del panel (un arrastre en el juego no lo desplaza),
  sigue al dedo desde donde se pulsó y al soltar sigue deslizándose con
  inercia, como ARCX. Cuando el contenido no cabe y queda más abajo, abajo
  del panel dice "Desliza para ver más" (en el idioma elegido). Con el
  selector de color abierto no hay desplazamiento por dirección (sus
  arrastres son del color).
- Idiomas: español, inglés, indonesio, portugués (Brasil), francés y alemán.
  Todo el panel y los géneros se traducen (los nombres de las estaciones no,
  son nombres propios). Se elige en el panel y se guarda como `idioma=es`
  (o en/id/pt/fr/de) en el .ini.
- Al abrir la rueda el juego va lento (ralentización estilo GTA V) y vuelve
  a velocidad normal al cerrarla: los valores de slow-motion se escriben en
  cada frame mientras está abierta (la v2.6 solo los escribía al abrir y el
  juego los borraba), con un registro en el log cada segundo para confirmar
  que la escala se mantiene en 0.30.
- Vehículos sin radio (bicis, tractores, patrullas): BLOQUEO por lista de
  modelos. La v2.14 se cerraba al llamar a la función GetVehicleAudioSettings
  del juego (el símbolo resuelve pero no es código llamable en este teléfono),
  así que la v2.15 ya no llama a ninguna función del juego: al entrar a un
  vehículo se lee su modelo (en 0x32, verificado por el log de la v2.13) y se
  compara con la lista de modelos sin radio civil (bicis 481/509/510,
  emergencia, patrullas 596-599, SWAT 601, especiales/RC/trabajo). Sin radio
  civil la rueda NO se abre (ni por claxon ni por zona), el botón flotante
  NO se ve, NO cambia de estación y el panel NO se puede abrir. Ante
  cualquier duda (modelo raro) la rueda ABRE para no bloquear un carro por error.
- La rueda funciona también en misiones: solo depende de estar en un vehículo
  con radio y no estar en pausa. En cinemáticas el juego pausa la entrada y
  la UI se oculta, como en el menú de pausa.
- La pausa se detecta por varias señales (CMenuManager, CTimer::m_UserPause
  y CMenuManager::DrawFrontEnd) para que la UI no se vea en el menú de pausa.
  (Si el teléfono no resuelve el menú, el panel sigue siendo usable y se
  puede cerrar con su botón Cerrar.)
- Log: `VHudRadioLog.txt` en files/VHudRadio/, junto al .ini.
- Si ningún símbolo de radio resolvió (ver `probe/`), la rueda se dibuja pero no
  cambia la estación y lo deja escrito en el log. No se llama a nada que no resolvió.

## Créditos

- El mod (la rueda de radio) es de Hunter09.
- Las imágenes de las emisoras (`VHudRadio/radio/`) son del proyecto
  [V-HUD](https://github.com/gennariarmando/v-hud), de Armando Gennari (_AG),
  bajo licencia MIT. El archivo LICENSE original de V-HUD va incluido en el
  paquete que se comparte, con su línea de Copyright intacta.
- El juego de iconos a color (`VHudRadio/radio_color/`) es opcional: desde
  la v2.29 también va incrustado en el .so, así que no hace falta copiarlo
  aparte. Es el juego personal de Hunter09 (logos redondos a color); si se
  comparte el mod, tener en cuenta que el .so ya los lleva dentro.
- Los logos de las emisoras son de Rockstar Games (GTA San Andreas).
