# VHud Radio Wheel 2.5.0-beta

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
- Panel de personalización estilo ARCX (violeta, borde en degradado
  azul-morado, letra grande, deslizadores y botones grandes): oscuridad del
  fondo, tamaño de la rueda y panel RGB táctil para el color del anillo
  (cuadro de saturación/valor + barra de tono, con valores R G B y hex)
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
