# VHud Radio Wheel 2.16.0-beta

Rueda de radio estilo GTA V para GTA SA Android 2.10 (AML .so).
Repo aparte: Gomezsl/VHudRadio-aml-mod. ARCX no se toca.

- Lee los iconos de `Android/data/com.rockstargames.gtasa/files/VHudRadio/radio/radio_1.png` ...
  `radio_13.png` (+ `radio_current.png`), sin importarlos ni incrustarlos. Si no están
  ahí, los busca en `files/vhud/radio/` y luego en `/storage/emulated/0/vhud/radio/`.
- Rueda de 13 estaciones en el anillo (apagar la radio va arriba del anillo).
  Soltar sobre un segmento cambia la estación. Doble toque al claxon abre/cierra.
- Botón flotante con etiqueta (nombre + género de la estación):
  un toque = siguiente estación, dos toques = panel de personalización.
  Se puede ARRASTRAR con el dedo a cualquier lugar de la pantalla (X e Y);
  la etiqueta cambia de lado si queda a la izquierda. En el panel se cambia
  su tamaño (50-150%), su opacidad (30-100%), se bloquea su posición y se
  restablece a la esquina de abajo a la derecha.
- Sonido: clic al abrir la rueda y al seleccionar una estación (OpenSL ES).
- Panel de personalización propio (azul casi oscuro, sin barra de título
  de ImGui, deslizadores, recuadros y botones dibujados a mano): oscuridad
  del fondo, tamaño de la rueda y dos recuadros de color (anillo y texto del
  centro); al tocar un recuadro se abre el panel RGB táctil (cuadro de
  saturación/valor + barra de tono, con valores R G B y hex). También:
  idioma, tamaño y opacidad del botón, bloqueo de su posición, vista previa
  (solo se ve) y restablecer todo. Se guarda en
  `Android/data/com.rockstargames.gtasa/files/VHudRadio/VHudRadio.ini`.
  Si existe un .ini viejo en `data/configs/` o en `files/configs/`, se muda solo.
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
- Log: `VHudRadioLog.txt` junto al juego.
- Si ningún símbolo de radio resolvió (ver `probe/`), la rueda se dibuja pero no
  cambia la estación y lo deja escrito en el log. No se llama a nada que no resolvió.
