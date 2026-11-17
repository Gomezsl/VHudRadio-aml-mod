# VHud Radio Wheel 2.2.0-beta

Rueda de radio estilo GTA V para GTA SA Android 2.10 (AML .so).
Repo aparte: Gomezsl/VHudRadio-aml-mod. ARCX no se toca.

- Lee los iconos de `Android/data/com.rockstargames.gtasa/files/vhud/radio/radio_1.png` ...
  `radio_13.png` (+ `radio_current.png`), sin importarlos ni incrustarlos. Si no están
  ahí, los busca en `/storage/emulated/0/vhud/radio/` como antes.
- Rueda de 12 estaciones en el anillo + apagar la radio en el centro (tocar el
  icono del centro apaga la radio). Soltar sobre un segmento cambia la estación.
- Botón flotante abajo a la derecha con etiqueta (nombre + género de la estación):
  un toque = siguiente estación, dos toques = panel de personalización.
  Doble toque al claxon abre/cierra la rueda.
- Sonido: clic al abrir la rueda y al seleccionar una estación (OpenSL ES).
- Panel de personalización: oscuridad del fondo, tamaño de la rueda, color del
  anillo de selección, vista previa (solo se ve) y restablecer todo. Se guarda
  en `Android/data/com.rockstargames.gtasa/files/configs/VHudRadio.ini`.
- Log: `VHudRadioLog.txt` junto al juego.
- Si ningún símbolo de radio resolvió (ver `probe/`), la rueda se dibuja pero no
  cambia la estación y lo deja escrito en el log. No se llama a nada que no resolvió.
