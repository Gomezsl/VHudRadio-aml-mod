# VHud Radio Wheel 1.2.0-beta

Rueda de radio estilo GTA V para GTA SA Android 2.10 (AML .so).
Repo aparte: Gomezsl/VHudRadio-aml-mod. ARCX no se toca.

- Lee los iconos de `/storage/emulated/0/vhud/radio/radio_1.png` ... `radio_13.png`
  (+ `radio_current.png`), sin importarlos ni incrustarlos.
- Botón flotante dentro de un vehículo -> rueda de 13 segmentos -> soltar cambia la estación.
- Log: `VHudRadioLog.txt` junto al juego.
- Si ningún símbolo de radio resolvió (ver `probe/`), la rueda se dibuja pero no
  cambia la estación y lo deja escrito en el log. No se llama a nada que no resolvió.
