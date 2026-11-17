# VHud Radio Probe 1.0.0

Mini-plugin AML que NO cambia nada: solo prueba si los símbolos de radio
existen en libGTASA.so 2.10 arm64 y escribe ok/FALTA en
`VHudRadioProbe.txt` (junto al juego, en `Android/data/com.rockstargames.gtasa/files/`).

Se compila igual que ARCX (GitHub Actions + NDK) y sale `libVHudRadioProbe64.so`.
Se mete en la carpeta de AML como cualquier plugin, se entra al juego una vez,
se sale, y se manda el txt. Con eso se arma la v1 de la rueda solo con los
símbolos que dijeron ok, sin adivinar.
