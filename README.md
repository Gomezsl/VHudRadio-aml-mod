# VHud Radio Wheel 2.11.0-beta

Rueda de radio estilo GTA V para GTA SA Android 2.10 (AML .so).
Repo aparte: Gomezsl/VHudRadio-aml-mod. ARCX no se toca.

- Lee los iconos de `Android/data/com.rockstargames.gtasa/files/vhud/radio/radio_1.png` ...
  `radio_13.png` (+ `radio_current.png`), sin importarlos ni incrustarlos. Si no están
  ahí, los busca en `/storage/emulated/0/vhud/radio/` como antes.
- Rueda de 13 estaciones en el anillo (apagar la radio va arriba del anillo).
  Soltar sobre un segmento cambia la estación. Doble toque al claxon abre/cierra.
- Botón flotante abajo a la derecha con etiqueta (nombre + género de la estación):
  un toque = siguiente estación, dos toques = panel de personalización.
- Sonido: clic al abrir la rueda y al seleccionar una estación (OpenSL ES).
- Panel de personalización propio (azul casi oscuro, sin barra de título
  de ImGui, deslizadores, recuadros y botones dibujados a mano): oscuridad
  del fondo, tamaño de la rueda y dos recuadros de color (anillo y texto del
  centro); al tocar un recuadro se abre el panel RGB táctil (cuadro de
  saturación/valor + barra de tono, con valores R G B y hex)
- Al abrir la rueda el juego va lento (ralentización estilo GTA V) y vuelve
  a velocidad normal al cerrarla: los valores de slow-motion se escriben en
  cada frame mientras está abierta (la v2.6 solo los escribía al abrir y el
  juego los borraba), con un registro en el log cada segundo para confirmar
  que la escala se mantiene en 0.30.
- Radio en vehículos sin radio (bicis, tractores, patrullas): al entrar a
  un vehículo se lee su entidad de audio (veh+0x458 o 0x1B0); si su radioID
  (0x9A) es 0 o -1 el vehículo no tiene radio y la rueda NO se abre (ni por
  claxon ni por zona) y el botón flotante no cambia de estación. El panel de
  personalización sigue disponible. El log registra la lectura y el bloqueo.
- La pausa se detecta por varias señales (CMenuManager, CTimer::m_UserPause
  y CMenuManager::DrawFrontEnd) para que la UI no se vea en el menú de pausa.

  de selección, vista previa (solo se ve) y restablecer todo. Se guarda en
  `Android/data/com.rockstargames.gtasa/configs/VHudRadio.ini` (o en
  `Android_unprotected/data/com.rockstargames.gtasa/configs/`, la ruta AML
  desprotegida). Si existe un .ini viejo en `files/configs/`, se muda solo.
- La rueda, el botón y el panel no se ven en el menú de pausa del juego.
  (Si el teléfono no resuelve el menú, el panel sigue siendo usable y se
  puede cerrar con su botón Cerrar.)
- Log: `VHudRadioLog.txt` junto al juego.
- Si ningún símbolo de radio resolvió (ver `probe/`), la rueda se dibuja pero no
  cambia la estación y lo deja escrito en el log. No se llama a nada que no resolvió.
