// VHudRadio.cpp - Rueda de radio estilo GTA V para GTA SA Android 2.10 (AML .so), v2.6 2.12.0-beta.
// Repo aparte: Gomezsl/VHudRadio-aml-mod. ARCX no se toca.
// Idea: v-hud de PC leia los iconos de VHud\radio\radio_1..13.png sin importarlos.
// Aqui igual: el plugin lee radio_1.png ... radio_13.png de files del juego (vhud/radio/),
// (+ radio_current.png) al arrancar, con stb_image, y los dibuja con ImGui.
// La rueda es NUEVA (en PC la radio no era rueda, solo un icono 2 s al cambiar):
// 13 segmentos en circulo, se abre con un boton flotante dentro de un vehiculo,
// se arrastra el dedo y al soltar cambia la estacion.
//
// Simbolos de radio: VERIFICADOS por VHudRadioProbe 1.0.0 en el telefono de Daniel
// (VHudRadioProbe.txt, 2026-11-16, 18/23 ok): TODOS los candidatos moviles resolvieron,
// las variantes de PC dieron FALTA (como se esperaba).
// v2.12: QUITAR EL BLOQUEO POR rid + BUSCAR EL MODELO DEL VEHICULO
// - La v2.11 salio AL REVES: bloqueo dos carros y dejo pasar la bici. El log
//   lo explica: rid(0x9A) NO es estable, en un mismo carro dio 0, 2, 0, 0 en
//   lecturas seguidas (es un byte que cambia, no la radio asignada). Asi que
//   el bloqueo por rid se quita por completo: la rueda vuelve a abrir en
//   todos los vehiculos como en la v2.10. El audio queda solo como
//   diagnostico (registra rt/rid solo cuando cambian).
// - Para bloquear de forma fiable se usara el MODELO del vehiculo (estable:
//   bici 481/509/510, tractor 531, patrulla 596-599). Esta version vuelca los
//   primeros 64 bytes del vehiculo al entrar, para encontrar m_nModelIndex.
// v2.11: BLOQUEO DE LA RUEDA EN VEHICULOS SIN RADIO
// - La v2.10 confirmo el discriminador: en una bici rid(0x9A)=0 (sin radio)
//   y en un carro rid=2; rt(0x9B)=0 en los dos, asi que el que distingue es
//   rid. Al entrar se asume sin radio hasta que la lectura lo confirme; si
//   en 120 frames no aparece el puntero se asume que si tiene (no dejar la
//   rueda bloqueada por un fallo de lectura). La rueda no se abre (ni por
//   claxon ni por zona) y el boton flotante no cambia de estacion cuando el
//   vehiculo no tiene radio. El panel de personalizacion sigue disponible.
// v2.10: LECTURA PERSISTENTE DE LA ENTIDAD DE AUDIO
// - La v2.9 SI registro todo y NO se cerro: 3 entradas, escaneo completo.
//   El candidato 0x458 es la entidad de audio (estable por vehiculo, zona
//   de ajustes en +0x90..0xAF, byte 0x9B = 0 = radio civil en un carro).
//   Pero en la entrada 1 (otro carro) el puntero 0x458 no aparecia todavia:
//   puede no estar puesto en el primer frame al entrar. Ahora se reintenta
//   cada frame hasta que aparece, y se registra RadioType (0x9B) y radioID
//   (0x9A). Se prueban 0x458 y 0x1B0 (los dos apuntaron al mismo objeto).
//   Aun no bloquea nada: falta una bici para confirmar que rt=-1 o 3.
// v2.9: ESCANEO SEGURO (mincore) - LA v2.8 CERRO EL JUEGO
// - La v2.8 se cerro (SIGSEGV en libVHudRadio64.so) al entrar a un auto de
//   policia: el escaneo leyo un valor (0x23F800000) que "parecia" un puntero
//   (cae en el rango 2^32..2^48) pero no esta mapeado, y al leerlo el juego
//   se cerro. Ahora cada puntero se valida con mincore (IsMapped) ANTES de
//   leerlo, y tambien la ventana que se vuelca. Rango mas amplio (0x100..
//   0x500) y hasta 20 candidatos. Aun no bloquea nada.
// - CEntity::GetModelIndex NO existe como simbolo en 2.10 (FALTA en las dos
//   formas): se quito; el modelo se sabe por el orden (carro primero, bici
//   despues) hasta encontrar la entidad de audio.
// v2.8: BUSCAR LA ENTIDAD DE AUDIO (ESCANEO DE CVehicle) + MODELO DEL VEHICULO
// - La v2.7 no encontro el puntero de audio en 0x138/0x140/0x148 (todo ceros
//   en 0x120..0x15F, y un flotante pequeno en 0x120): el offset del telefono
//   es otro. Ahora se escanea CVehicle (0x100..0x380) buscando punteros a
//   objetos reales (puntero creible + vtable creible) y se vuelca la zona de
//   ajustes de cada uno (0x70..0xAF); el que tenga el byte 0 en un carro y -1
//   en una bici es la entidad de audio (RadioType). Aun no bloquea nada.
// - La ralentizacion de la v2.7 SI se mantiene: el log mostro
//   "escala leida=0.30 activa=1", o sea que el juego no borra los valores.
// - Nuevo: CEntity::GetModelIndex para registrar el modelo del vehiculo en
//   el log (481 BMX, 509 bici, 510 bici de montana, 531 tractor, 596-599
//   patrullas...) y saber si es carro o bici sin preguntar.
// v2.7: RALENTIZACION DE VERDAD (CADA FRAME) + RADIOTYPE POR PUNTERO
// - La v2.6 decia "ralentizacion: activada" en el log pero NO se veia nada:
//   solo escribia los valores al abrir/cerrar y el juego los borraba en el
//   siguiente frame (como el opcode 015D de CLEO, la velocidad se pone en un
//   bucle, no una vez). Ahora se escriben EN CADA FRAME mientras la rueda
//   esta abierta (bandera + escala del slow-motion + escala de tiempo), y al
//   cerrar se restauran los valores que habia antes. El log lee la escala
//   cada segundo para ver si el juego la esta borrando.
// - RadioType: la v2.6 leia veh+0x1D3 directo y salio todo ceros (el radioID
//   debia ser 5): el audio NO esta dentro de CVehicle, es un PUNTERO en
//   CVehicle (0x138 en PC) a CAEVehicleAudioEntity. Ahora se lee ese puntero
//   (probando 0x138/0x140/0x148) y se vuelca la zona de ajustes de la entidad.
// - La pausa YA funciona (el log de la v2.6 la detecto por m_UserPause=1).
// v2.6: PANEL PROPIO AZUL + 2 COLORES + RALENTIZACION + PAUSA DE VERDAD + RADIOTYPE
// - La pausa SEGUIA sin esconder la UI en la v2.5: ahora se detecta por varias
//   senales a la vez (byte 0x5C de CMenuManager, CTimer::m_UserPause /
//   m_CodePause, y si CMenuManager::DrawFrontEnd se llamo en este frame), y el
//   log registra que senal disparo y que bytes del objeto CMenuManager cambian
//   (para encontrar el byte real de pausa en este telefono).
// - El panel ya no es la ventana de ImGui por defecto: es un panel propio azul
//   casi oscuro, sin barra de titulo, con deslizadores, recuadros y botones
//   dibujados a mano (como ARCX, no como ImGui).
// - El color del anillo ya no tiene el panel RGB siempre abierto: se toca un
//   recuadro pequeno para abrirlo, y hay un segundo recuadro para el color del
//   texto del centro de la rueda (nuevo ajuste texto=r,g,b en el .ini).
// - Al abrir la rueda el juego va lento (ralentizacion estilo GTA V, con el
//   slow-motion propio del juego: CTimer::bSlowMotionActive /
//   ms_fSlowMotionScale, o ms_fTimeScale si no resuelven); al cerrarla vuelve
//   a velocidad normal.
// - RadioType (del texto de la imagen): se lee el byte candidato
//   (CVehicle 0x138 + audio 0x80 + radio 0x1B = 0x1D3) y se registra en el
//   log al entrar a un vehiculo, con una ventana de bytes y la estacion
//   actual, para confirmar el offset del telefono antes de bloquear la rueda
//   en bicis/tractores/patrullas (en la v2.6 aun no bloquea nada).
// v2.5: PANEL ESTILO ARCX + PANEL RGB TACTIL
// - El panel ahora usa el diseno de ARCX (3.1.35-3.1.40): violeta ARCX
//   (0.45, 0.30, 0.95), fondo oscuro tintado, titulo centrado y borde en
//   degradado azul -> morado alrededor de toda la ventana.
// - Los 8 colores predeterminados y el ColorEdit3 se quitaron: ahora hay un
//   panel RGB tactil como el selector de ARCX 3.1.38 - cuadro grande de
//   saturacion/valor + barra vertical de tono, se arrastra con el dedo, con
//   muestra del color y valores R G B + hexadecimal. El color se guarda al
//   soltar (y al cerrar), como los deslizadores.
// - El anillo sigue siendo el color que elija el usuario (por defecto cian);
//   el violeta es solo el diseno del panel.
// v2.4: PANEL FUNCIONAL + PAUSA DE VERDAD
// - El panel NO funcionaba: Touch_hook solo pasaba los eventos de soltar/mover
//   a ImGui cuando la rueda estaba abierta (g_open || g_cap, y g_cap nunca se
//   pone en true). Con el panel abierto y la rueda cerrada, ImGui recibia el
//   toque de bajar pero nunca el de soltar: los botones no se disparaban, los
//   deslizadores no se movian y "Cerrar" no funcionaba. Ahora tambien se pasan
//   cuando el panel o la vista previa estan abiertos.
// - La pausa no escondia la UI: FrontEndMenuManager es una REFERENCIA (puntero
//   al objeto), no el objeto; hay que desreferenciarla. Ademas, si el claxon
//   se pulsa mientras el byte de menu dice 1, el offset 0x5C no es el menu en
//   este telefono y la deteccion se desactiva sola (con mensaje en el log).
// v2.3: APAGAR ARRIBA DEL ANILLO + PANEL USABLE + SIN UI EN PAUSA
// - Apagar NO va en el centro: vuelve al anillo, arriba (donde Daniel lo marco
//   con el circulo en la captura de la v2.1); el centro vuelve a ser el nombre
//   y el genero de la estacion. El anillo vuelve a tener 13 segmentos.
// - Panel redisenado para usarse al tacto: letra grande (32 px), etiquetas
//   ENCIMA de los deslizadores, colores rapidos (8 botones grandes) + color
//   propio, botones a todo lo ancho y ventana mas estrecha.
// - La rueda, el boton y el panel ya no se ven en el menu de pausa del juego
//   (se lee CMenuManager::m_bMenuActive, en 0x5C segun plugin-sdk).
// v2.2: APAGAR AL CENTRO + BOTON CON ETIQUETA + SONIDO + PANEL DE PERSONALIZACION
// - La estacion 13 (Radio Off) ya no va en el anillo: va AL CENTRO de la rueda
//   (tocarla apaga la radio); el anillo queda con 12 estaciones.
// - Boton flotante mas abajo (0.86H) y con etiqueta: nombre de la estacion y su
//   genero a la izquierda del icono. Un toque = siguiente estacion; dos toques
//   = panel de personalizacion (la rueda ya solo se abre con el claxon).
// - Sonido: clic corto al abrir la rueda y al seleccionar una estacion, generado
//   con OpenSL ES (sin depender del juego).
// - Panel de personalizacion: oscuridad del fondo, tamano de la rueda, color del
//   anillo de seleccion, vista previa (solo se ve, no cambia la estacion) y
//   restablecer todo. Se guarda en configs/VHudRadio.ini (data del juego).
// v2.1: LOGOS CON AIRE + MEJOR LETRA + BOTON CON OTRA FUNCION + ICONOS EN FILES
// - Los logos se veian muy juntos: anillo mas grande (0.83R) e iconos mas chicos
//   (0.175R) para que no se toquen.
// - Letra del centro: Roboto Condensed negrita de /system/fonts (la mas parecida
//   a la de GTA V que trae el telefono); si no esta, Roboto Bold o la de ImGui.
// - Boton flotante: un toque = siguiente estacion; dos toques = abrir la rueda.
// - Iconos: ahora se buscan primero en files del juego
//   (Android/data/com.rockstargames.gtasa/files/vhud/radio/) y si no estan ahi,
//   en /storage/emulated/0/vhud/radio/ como antes.
// v2.0: DISENO GTA V + SIN CALIBRAR - el claxon ya se detecta por el boton 7
// (confirmado en el log de la v1.9: 11 aperturas por "boton 7"), asi que el boton
// "Calibrar claxon" bajo la rueda se quito (la zona tactil queda solo de
// respaldo). La rueda ahora es estilo GTA V: logos grandes flotando en circulo
// sin disco, la seleccionada con anillo cian y las demas atenuadas, y en el
// centro el nombre de la estacion + su genero.
// v1.9: CLAXON POR BOTON 7 - la sonda de la v1.8 lo confirmo: el byte 84 (0x54)
// del widget m_pWidgets[7] es su bandera de "tocado" (1 al pulsarlo, 0 al
// soltarlo; 16/17 volcados con 1 caen a +-1 volcado de un toque al claxon, y
// el float de 80-83 es el tiempo que lleva pulsado en segundos). Esta version
// lee esa bandera cada frame y el doble toque al claxon abre/cierra la rueda
// sin depender de la posicion del boton. La zona tactil queda
// como respaldo por si el arreglo no resuelve o el widget 7 es null.
// v1.8: BOTON 7 DIRECTO - el escaneo de la v1.7 revelo que IsTouchDown(int)
// cuenta DEDOS (indice multi-touch 0..7), no botones: solo el 0 se encendia
// con cualquier toque. El claxon SI es el widget 7 (GTAForums: 7 = claxon/horn),
// asi que esta version lee el boton 7 directo del arreglo CTouchInterface::
// m_pWidgets[7] y vuelca sus primeros 128 bytes (y los del widget 2, acelerar,
// como control) para encontrar su bandera de "tocado". Si aparece, la v1.9 la
// leera directo y el claxon sera independiente de la posicion del boton.
// v1.7: CLAXON POR WIDGET - verificado desensamblando el mod de rueda de armas que
// mando Daniel (libweapwheeldliiartworks64.so, 1.5 MB): ese mod resuelve por GetSym
// CTouchInterface::IsTouchDown(int) y lo llama como funcion estatica con el id del
// widget en w0 (sin 'this': en su bucle lo llama con w0 = 0..7 y tambien
// GetTouchPosition(id) e IsReleased(id,...) igual). El claxon seria el widget 7
// (WIDGET_HORN, segun la otra IA; se confirma con el escaneo de widgets que trae
// esta version: registra que ids estan pulsados cuando cambia el conjunto).
// La zona tactil/calibrada queda como respaldo. Si mueves el boton de claxon, el
// widget 7 lo sigue detectando igual.
// v1.6: CALIBRAR CLAXON - el volcado CPad de la v1.5 salio IDENTICO en los 120
// volcados: el estado del claxon NO esta en los primeros 256 bytes de CPad (o no
// esta en CPad). Mientras se sigue buscando (volcado ampliado a 2048 bytes), la
// v1.6 (retirado en v2.0): la rueda traia un boton "Calibrar claxon"
// del juego, y la zona queda guardada en /storage/emulated/0/vhud/horn_pos.txt.
// Si mueves el boton de claxon en los controles del juego, lo recalibras con un
// toque. La zona ya no es fija: es la que tu calibraste.
// v1.5: la rueda ahora se queda ABIERTA hasta tocar una estacion (antes solo el
// arrastre elegia: un toque directo a un segmento no lo seleccionaba, asi que habia
// que mantener el dedo puesto). Tocar un segmento lo aplica y cierra; tocar fuera
// de la rueda la cierra sin cambiar; el arrastre sigue funcionando.
// v1.5 tambien vuelca la memoria de CPad para encontrar donde el juego guarda el
// estado del claxon: si se mueve el boton de claxon en los controles del juego, la
// zona tactil (v1.2-v1.4) deja de funcionar; leer el claxon desde CPad no depende
// de la posicion del boton. El volcado es solo diagnostico para la v1.6.
// v1.2: RetuneRadio NO cambia a una estacion concreta (en la prueba de Daniel la
// radio se quedo en 10 aunque se llamo RetuneRadio(4/7/13/6/3): parece que solo
// avanza/retrocede un paso). El cambio real es StartRadio: primero TrackManager
// (Eaafh, el que usa el mod GTA6Radio 4.0 en 2.10), luego AudioEngine (Eaa),
// y RetuneRadio queda solo como ultimo recurso.
// v1.3: BUG DE LA v1.2 - las funciones de radio son metodos C++ y necesitan 'this'
// (el objeto global AudioEngine / AERadioTrackManager). Sin 'this', el id de la
// estacion caia en el registro de 'this' y la funcion tocaba memoria basura:
// por eso StartRadio(12) cerro el juego en la prueba de Daniel, y por eso
// RetuneRadio nunca cambio nada (su argumento real era basura) y GetCurrent
// devolvia valores raros (10, 13...). Ahora todo se llama con su 'this' real.
// NO usar los mangleds de PC (Ea vs Ec, Eific...). CMenuManager::RetuneRadio NO existe.
// Por eso la v1 NUNCA llama a una funcion que no resolvio con GetSym: si RetuneRadio
// falta, la rueda se abre, se ve, pero al soltar solo lo registra en el log.
// El mini-plugin VHudRadioProbe (probe/) prueba estos mismos simbolos y escribe
// ok/FALTA en VHudRadioProbe.txt; la v1 se ajusta a lo que el probe diga.
#include <mod/config.h>
#include <mod/amlmod.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <SLES/OpenSLES.h>  // v2.2: sonido (la libreria se abre con dlopen, no se enlaza)
#include <SLES/OpenSLES_Android.h>  // v2.2: SLDataLocator_AndroidSimpleBufferQueue vive aqui, no en OpenSLES.h
#include <GLES2/gl2.h>
#include <dlfcn.h>
#include <link.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_SIMD
#define STBI_ONLY_PNG
#include "stb_image.h"

MYMODCFG(net.hunter09.vhudradio, VHud Radio Wheel, 2.12.0-beta, Hunter09)
NEEDGAME(com.rockstargames.gtasa)
#define VHUDRADIO_VERSION "2.12.0-beta"

// ---- log: VHudRadioLog.txt junto al juego ----
static std::string g_logPath;
static std::string g_gameFilesDir;  // files del juego (se resuelve en OnModLoad)
static std::string g_gameDataDir;  // v2.3: carpeta data del juego (padre de files) - el .ini va en data/configs
static void Log(const char* fmt, ...) {
    // v1.7: buf 8192 (antes 600 y el volcado CPad de 2048 bytes se truncaba a ~295)
    char buf[8192];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    __android_log_print(ANDROID_LOG_INFO, "VHudRadio", "%s", buf);
    if (!g_logPath.empty()) { FILE* f = fopen(g_logPath.c_str(), "a"); if (f) { fprintf(f, "%s\n", buf); fclose(f); } }
}

// ---- sonido (v2.2): clic corto generado con OpenSL ES (sin depender del juego) ----
static void* g_slHandle = nullptr;
typedef SLresult (*SlCreateEngineFn)(SLObjectItf*, SLuint32, const SLEngineOption*, SLuint32, const SLInterfaceID*, const SLboolean*);
static SlCreateEngineFn g_slCreateEngine = nullptr;
static SLObjectItf g_slEngine = nullptr, g_slMix = nullptr, g_slPlayer = nullptr;
static SLEngineItf g_slEngineItf = nullptr;
static SLPlayItf g_slPlay = nullptr;
static SLBufferQueueItf g_slQueue = nullptr;
static short g_clickLo[1600];
static short g_clickHi[1600];
static bool g_soundTried = false, g_soundOk = false;
static void SoundInit() {
    if (g_soundTried) return; g_soundTried = true;
    g_slHandle = dlopen("libOpenSLES.so", RTLD_NOW);
    if (!g_slHandle) { Log("  sonido: libOpenSLES.so no disponible"); return; }
    g_slCreateEngine = (SlCreateEngineFn)dlsym(g_slHandle, "slCreateEngine");
    if (!g_slCreateEngine) { Log("  sonido: slCreateEngine no disponible"); return; }
    if (g_slCreateEngine(&g_slEngine, 0, nullptr, 0, nullptr, nullptr) != SL_RESULT_SUCCESS || !g_slEngine) return;
    if ((*g_slEngine)->Realize(g_slEngine, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) return;
    if ((*g_slEngine)->GetInterface(g_slEngine, SL_IID_ENGINE, &g_slEngineItf) != SL_RESULT_SUCCESS || !g_slEngineItf) return;
    if ((*g_slEngineItf)->CreateOutputMix(g_slEngineItf, &g_slMix, 0, nullptr, nullptr) != SL_RESULT_SUCCESS) return;
    if ((*g_slMix)->Realize(g_slMix, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) return;
    SLDataLocator_AndroidSimpleBufferQueue locBuf = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM fmt = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_22_05, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16, SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource src = {&locBuf, &fmt};
    SLDataLocator_OutputMix locMix = {SL_DATALOCATOR_OUTPUTMIX, g_slMix};
    SLDataSink sink = {&locMix, nullptr};
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    if ((*g_slEngineItf)->CreateAudioPlayer(g_slEngineItf, &g_slPlayer, &src, &sink, 1, ids, req) != SL_RESULT_SUCCESS) return;
    if ((*g_slPlayer)->Realize(g_slPlayer, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) return;
    if ((*g_slPlayer)->GetInterface(g_slPlayer, SL_IID_PLAY, &g_slPlay) != SL_RESULT_SUCCESS) return;
    if ((*g_slPlayer)->GetInterface(g_slPlayer, SL_IID_BUFFERQUEUE, &g_slQueue) != SL_RESULT_SUCCESS) return;
    if ((*g_slPlay)->SetPlayState(g_slPlay, SL_PLAYSTATE_PLAYING) != SL_RESULT_SUCCESS) return;
    for (int i = 0; i < 1600; i++) {
        float t = (float)i / 22050.0f;
        float env = expf(-t * 60.0f);
        g_clickLo[i] = (short)(sinf(2.0f * 3.14159265f * 880.0f * t) * env * 12000.0f);
        g_clickHi[i] = (short)(sinf(2.0f * 3.14159265f * 1318.0f * t) * env * 12000.0f);
    }
    g_soundOk = true;
    Log("  sonido: OpenSL ES listo");
}
static void PlayClick(bool hi) {
    SoundInit();
    if (!g_soundOk || !g_slQueue) return;
    (*g_slQueue)->Clear(g_slQueue);
    (*g_slQueue)->Enqueue(g_slQueue, hi ? g_clickHi : g_clickLo, 1600 * 2);
}

// ---- estaciones 1..13 (VERIFIED por plugin-sdk eRadioID + GTAMods + v-hud) ----
static const char* kStationName[14] = { "", "Playback FM", "K-Rose", "K-DST", "Bounce FM", "SF-UR",
    "Radio Los Santos", "Radio X", "CSR 103.9", "K-Jah West", "Master Sounds 98.3",
    "WCTR", "User tracks", "Radio Off" };

// v2.0: genero de cada estacion, para el centro de la rueda estilo GTA V
static const char* kStationTag[14] = { "", "Hip hop clasico", "Country", "Rock clasico",
    "Funk y disco", "House", "Hip hop", "Rock alternativo", "R&B y soul", "Reggae",
    "Groove y funk", "Charlas y noticias", "Tu propia musica", "Radio apagada" };

// ---- funciones del juego (resueltas por GetSym, nunca llamadas si faltan) ----
static void* (*FindPlayerPed)(int);
static void* (*FindPlayerVehicle)(int, bool);
// Las funciones de radio son metodos C++: el primer argumento es 'this', el objeto
// global del juego (resuelto por GetSym, el Probe ya confirmo que existen).
static void* g_audioEngine = nullptr;   // global CAudioEngine
static void* g_trackManager = nullptr;  // global CAERadioTrackManager
typedef signed char (*RetuneFn)(void*, signed char);  // en la practica devuelve void/char; solo se llama si resolvio
static RetuneFn g_retuneAudio = nullptr, g_retuneTrack = nullptr;
typedef signed char (*GetStationFn)(void*);
static GetStationFn g_getStationAudio = nullptr, g_getStationTrack = nullptr;
typedef void (*StartRadioAudioFn)(void*, signed char, signed char);
static StartRadioAudioFn g_startAudio = nullptr;
typedef void (*StartRadioTrackFn)(void*, signed char, signed char, float, unsigned char);
static StartRadioTrackFn g_startTrack = nullptr;  // el que usa el mod GTA6Radio 4.0 en 2.10 (verificado por Probe: ok)

// ---- claxon por boton 7 directo (v1.8): CTouchInterface::m_pWidgets[7] ----
// IsTouchDown resulto ser indice de dedo, no de boton (v1.7). El claxon es el
// widget 7 (GTAForums: 7 = claxon/horn); aqui se lee directo del arreglo de
// botones del juego y se vuelca su memoria para encontrar su bandera de "tocado".
static void** g_widgets = nullptr;  // CTouchInterface::m_pWidgets
static const int kHornWidgetId = 7;  // WIDGET_HORN (GTAForums: 7 = claxon/horn)

// ---- claxon: doble toque al boton de claxon del juego abre/cierra la rueda ----
// v1.1 intento CPad::GetHorn, pero NO existe en libGTASA.so 2.10 (FALTA en el log).
// v1.2 uso una zona estimada (y 0.40-0.62H) y fallo: los toques reales de Daniel
// (log v1.3, toques #1-7) caen en (2258, 735) en 2408x1080, o sea (0.938W, 0.683H),
// justo DEBAJO de la zona estimada. v1.4: circulo centrado en ese punto real.
// v1.6 (boton "Calibrar claxon" retirado en v2.0: el claxon ya se detecta por el
// boton 7 del juego): el centro de la zona se guarda en horn_pos.txt y se sigue
// cargando al inicio por si se calibro antes; la zona solo es respaldo del boton 7.
static float g_hornCx = 0.938f, g_hornCy = 0.683f;  // centro del claxon (fraccion de W/H)
static bool InHornRegion(float x, float y, float W, float H) {
    float cx = W*g_hornCx, cy = H*g_hornCy, r = H*0.07f;
    float dx = x - cx, dy = y - cy;
    return dx*dx + dy*dy <= r*r;
}
static void LoadHornPos() {
    // v2.1: horn_pos.txt tambien se busca primero en files del juego
    const std::string hpPaths[2] = {g_gameFilesDir + "/vhud/horn_pos.txt", "/storage/emulated/0/vhud/horn_pos.txt"};
    for (const std::string& hp : hpPaths) {
        FILE* f = fopen(hp.c_str(), "r");
        if (!f) continue;
        float x = 0, y = 0;
        if (fscanf(f, "%f %f", &x, &y) == 2 && x > 0.05f && x < 0.99f && y > 0.05f && y < 0.99f) { g_hornCx = x; g_hornCy = y; }
        fclose(f);
        return;
    }
}

// ---- ajustes de la rueda (v2.2): se guardan en data/configs/VHudRadio.ini ----
// [VHudRadio] fondo=30 (0..100, oscuridad del fondo) tamano=36 (25..45, % de H)
// anillo=0,210,255 (color del anillo de seleccion, 0..255)
// texto=255,255,255 (color del texto del centro de la rueda, 0..255) (v2.6)
struct WheelCfg { int fondo = 30; int tamano = 36; int ringR = 0; int ringG = 210; int ringB = 255; int textoR = 255; int textoG = 255; int textoB = 255; };
static WheelCfg g_cfg;
static bool g_settings = false;   // v2.2: panel de personalizacion abierto
static bool g_preview = false;    // v2.2: vista previa de la rueda (solo se ve, no cambia la estacion)
static std::string CfgPath() { return g_gameDataDir + "/configs/VHudRadio.ini"; }  // v2.3: data/configs (AML), NO files/configs
static void SaveCfg() {
    mkdir((g_gameDataDir + "/configs").c_str(), 0777);
    FILE* f = fopen(CfgPath().c_str(), "w");
    if (!f) return;
    fprintf(f, "[VHudRadio]\n");
    fprintf(f, "fondo=%d\n", g_cfg.fondo);
    fprintf(f, "tamano=%d\n", g_cfg.tamano);
    fprintf(f, "anillo=%d,%d,%d\n", g_cfg.ringR, g_cfg.ringG, g_cfg.ringB);
    fprintf(f, "texto=%d,%d,%d\n", g_cfg.textoR, g_cfg.textoG, g_cfg.textoB);
    fclose(f);
}
static void LoadCfg() {
    g_cfg.fondo = 30; g_cfg.tamano = 36; g_cfg.ringR = 0; g_cfg.ringG = 210; g_cfg.ringB = 255; g_cfg.textoR = 255; g_cfg.textoG = 255; g_cfg.textoB = 255;
    bool fromOld = false;
    FILE* f = fopen(CfgPath().c_str(), "r");
    if (!f) {
        // v2.3: la v2.2 lo guardaba en files/configs - mudarlo si existe ahi
        std::string old = g_gameFilesDir + "/configs/VHudRadio.ini";
        f = fopen(old.c_str(), "r");
        if (f) { fromOld = true; Log("  ajustes: mudando el .ini viejo de %s", old.c_str()); }
    }
    if (!f) { SaveCfg(); Log("  ajustes: creando %s con valores por defecto", CfgPath().c_str()); return; }  // no existe: crearlo con los valores por defecto
    char line[256];
    while (fgets(line, sizeof line, f)) {
        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        char* k = line;
        while (*k == ' ' || *k == '\t') k++;
        size_t kl = strlen(k);
        while (kl > 0 && (k[kl-1] == ' ' || k[kl-1] == '\t' || k[kl-1] == '\n' || k[kl-1] == '\r')) k[--kl] = 0;
        const char* v = eq + 1;
        if (strcmp(k, "fondo") == 0) g_cfg.fondo = atoi(v);
        else if (strcmp(k, "tamano") == 0) g_cfg.tamano = atoi(v);
        else if (strcmp(k, "anillo") == 0) {
            int r = 0, g = 0, b = 0;
            if (sscanf(v, "%d,%d,%d", &r, &g, &b) == 3) { g_cfg.ringR = r; g_cfg.ringG = g; g_cfg.ringB = b; }
        }
        else if (strcmp(k, "texto") == 0) {
            int r = 255, g = 255, b = 255;
            if (sscanf(v, "%d,%d,%d", &r, &g, &b) == 3) { g_cfg.textoR = r; g_cfg.textoG = g; g_cfg.textoB = b; }
        }
    }
    fclose(f);
    if (g_cfg.fondo < 0) g_cfg.fondo = 0; if (g_cfg.fondo > 100) g_cfg.fondo = 100;
    if (g_cfg.tamano < 25) g_cfg.tamano = 25; if (g_cfg.tamano > 45) g_cfg.tamano = 45;
    if (g_cfg.ringR < 0) g_cfg.ringR = 0; if (g_cfg.ringR > 255) g_cfg.ringR = 255;
    if (g_cfg.ringG < 0) g_cfg.ringG = 0; if (g_cfg.ringG > 255) g_cfg.ringG = 255;
    if (g_cfg.ringB < 0) g_cfg.ringB = 0; if (g_cfg.ringB > 255) g_cfg.ringB = 255;
    if (g_cfg.textoR < 0) g_cfg.textoR = 0; if (g_cfg.textoR > 255) g_cfg.textoR = 255;
    if (g_cfg.textoG < 0) g_cfg.textoG = 0; if (g_cfg.textoG > 255) g_cfg.textoG = 255;
    if (g_cfg.textoB < 0) g_cfg.textoB = 0; if (g_cfg.textoB > 255) g_cfg.textoB = 255;
    if (fromOld) SaveCfg();  // escribirlo ya en la ruta nueva
    Log("  ajustes: %s (fondo=%d tamano=%d anillo=%d,%d,%d texto=%d,%d,%d)", CfgPath().c_str(), g_cfg.fondo, g_cfg.tamano, g_cfg.ringR, g_cfg.ringG, g_cfg.ringB, g_cfg.textoR, g_cfg.textoG, g_cfg.textoB);
}

static std::string g_vhudDir;   // /storage/emulated/0/vhud
static int g_current = 1;       // estacion actual conocida (1..13)
static int g_selected = 1;      // segmento bajo el dedo mientras la rueda esta abierta
static bool g_open = false;
static bool g_btnPending = false;  // v2.1: un toque al boton flotante esperando (siguiente estacion si no hay segundo toque)
static std::chrono::steady_clock::time_point g_btnPendingAt;
static bool g_wheelTouch = false;   // v1.5: el toque actual empezo/sobre el anillo de la rueda
static bool g_ignoreNextUp = false; // v1.5: el toque que ABRIO la rueda no debe cerrarla al soltar

// v2.2: geometria de la rueda segun los ajustes (fondo/tamano/anillo)
static float WheelR(float H) { return H * g_cfg.tamano / 100.0f; }
static float WheelRi(float H) { return WheelR(H) * 0.83f; }
static float WheelIconS(float H) { return WheelR(H) * 0.175f; }
static ImU32 RingColor() { return IM_COL32(g_cfg.ringR, g_cfg.ringG, g_cfg.ringB, 255); }

// v2.3: segmento bajo un punto (13 segmentos; apagar va arriba del anillo, no en el centro)
static int SegmentAt(float x, float y, float W, float H) {
    float dx = x - W*0.5f, dy = y - H*0.5f;
    float dist = sqrtf(dx*dx + dy*dy);
    float Ri = WheelRi(H), s = WheelIconS(H);
    if (dist < Ri - s*1.3f || dist > Ri + s*1.3f) return 0;  // centro o fuera de la rueda
    float ang = atan2f(dy, dx) + 3.14159265f/2; if (ang < 0) ang += 2*3.14159265f;
    int seg = (int)(ang / (2*3.14159265f/13.0f)) + 1; if (seg > 13) seg = 13;
    return seg;
}
static bool g_inVehicle = false;
static void* g_menuManager = nullptr;  // v2.3: global CMenuManager (FrontEndMenuManager)
static bool g_paused = false;          // v2.3: menu de pausa (o cualquier menu) abierto
static int g_menuCheck = 0;            // v2.4: 0 = sin verificar, 1 = offset valido, -1 = offset no valido (pausa desactivada)
static bool MenuActive() {
    if (!g_menuManager || g_menuCheck == -1) return false;
    return ((const unsigned char*)g_menuManager)[0x5C] != 0;  // m_bMenuActive (plugin-sdk CMenuManager)
}
// v2.6: mas senales de pausa (el byte 0x5C solo no basto en la v2.5) y ralentizacion
static bool* g_userPause = nullptr;   // CTimer::m_UserPause (pausa del usuario)
static bool* g_codePause = nullptr;   // CTimer::m_CodePause (pausa por codigo/cinematica)
static bool g_menuDrawn = false;      // CMenuManager::DrawFrontEnd se llamo en este frame
static bool* g_slowActive = nullptr;  // CTimer::bSlowMotionActive (slow-motion propio del juego)
static float* g_slowScale = nullptr;  // CTimer::ms_fSlowMotionScale
static float* g_timeScale = nullptr;  // CTimer::ms_fTimeScale (respaldo si no hay slow-motion)
static void (*DrawFrontEnd_orig)() = nullptr;
static void DrawFrontEnd_hook() {
    g_menuDrawn = true;
    if (DrawFrontEnd_orig) DrawFrontEnd_orig();
}
// v2.6: diagnostico - registra que bytes del objeto CMenuManager cambian, para
// encontrar el byte real de pausa en este telefono (el 0x5C no basto en la v2.5)
static unsigned char g_menuSnap[96];
static bool g_menuSnapInit = false;
static int g_menuSnapTick = 0;
static void MenuSnapCheck() {
    if ((++g_menuSnapTick % 30) != 0) return;
    const unsigned char* m = (const unsigned char*)g_menuManager;
    if (!g_menuSnapInit) { memcpy(g_menuSnap, m, 96); g_menuSnapInit = true; return; }
    char buf[1024]; int n = 0;
    n += snprintf(buf + n, sizeof buf - (size_t)n, "menu: bytes que cambiaron:");
    bool diff = false;
    for (int i = 0; i < 96; i++) {
        if (m[i] != g_menuSnap[i]) { diff = true; n += snprintf(buf + n, sizeof buf - (size_t)n, " [%02X]=%02X", i, m[i]); }
    }
    if (diff) Log("%s", buf);
    memcpy(g_menuSnap, m, 96);
}
// v2.4: FrontEndMenuManager es una REFERENCIA (puntero al objeto) en plugin-sdk:
// GetSym devuelve la direccion del puntero, hay que desreferenciarla. Si el
// simbolo ya es el objeto, sus primeros 8 bytes no son una direccion valida
// del juego (son ~0 o un float desplazado), asi que se distingue solo.
static void* ResolveMenuManager(void* sym) {
    if (!sym) return nullptr;
    uintptr_t v = *(uintptr_t*)sym;
    if (v > 0x100000000ULL && v < 0x1000000000000ULL) return (void*)v;  // es una referencia: desreferenciar
    return sym;  // el simbolo ya es el objeto
}

// ---- texturas ImGui de los iconos (se crean en el primer frame, con contexto GL) ----
static GLuint g_tex[14] = {0};  // 1..13 + radio_current en [0] como marco
static bool g_texTried = false;
static GLuint LoadPngToGl(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END); long len = ftell(f); fseek(f, 0, SEEK_SET);
    if (len <= 0 || len > 4*1024*1024) { fclose(f); return 0; }
    std::vector<unsigned char> buf((size_t)len);
    if (fread(buf.data(), 1, buf.size(), f) != buf.size()) { fclose(f); return 0; }
    fclose(f);
    int w = 0, h = 0, ch = 0;
    unsigned char* px = stbi_load_from_memory(buf.data(), (int)buf.size(), &w, &h, &ch, 4);
    if (!px || w <= 0 || h <= 0) { if (px) stbi_image_free(px); return 0; }
    GLint last = 0; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last);
    GLuint tex = 0; glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glBindTexture(GL_TEXTURE_2D, (GLuint)last);
    stbi_image_free(px);
    return glGetError() == GL_NO_ERROR ? tex : 0;
}
static void LoadRadioTextures() {
    if (g_texTried) return; g_texTried = true;
    // v2.1: iconos dentro de files del juego (Android/data/com.rockstargames.gtasa/files/vhud/radio/);
    // si no estan ahi, se usa la carpeta de antes (/storage/emulated/0/vhud/radio/).
    std::string dir = g_gameFilesDir + "/vhud/radio";
    { struct stat st; if (stat((dir + "/radio_1.png").c_str(), &st) != 0) dir = g_vhudDir + "/radio"; }
    Log("carpeta de iconos: %s/ (radio_1.png ... radio_13.png)", dir.c_str());
    int ok = 0;
    for (int i = 1; i <= 13; i++) {
        std::string p = dir + "/radio_" + std::to_string(i) + ".png";
        g_tex[i] = LoadPngToGl(p);
        if (g_tex[i]) ok++;
        Log("  %s radio_%d.png", g_tex[i] ? "ok    " : "FALTA ", i);
    }
    g_tex[0] = LoadPngToGl(dir + "/radio_current.png");
    Log("  %s radio_current.png", g_tex[0] ? "ok    " : "FALTA ");
    Log("iconos de radio cargados: %d/13 (+marco %s)", ok, g_tex[0] ? "si" : "no");
    if (ok == 0) Log("  !! sin iconos: copia vhud/radio/ a files del juego o a /storage/emulated/0/vhud/ (ver README)");
}

// ---- cambiar de estacion: solo con funciones que resolvieron ----
static void ApplyStation(int id) {
    if (id < 1 || id > 13) return;
    // v1.2: StartRadio primero (es el que cambia a una estacion concreta).
    // v1.3: cada llamada lleva su 'this' (el objeto global del juego).
    if (g_startTrack && g_trackManager) { Log("cambio a %d (%s) por CAERadioTrackManager::StartRadio(id,0,1.0,0)", id, kStationName[id]); g_startTrack(g_trackManager, (signed char)id, 0, 1.0f, 0); }
    else if (g_startAudio && g_audioEngine) { Log("cambio a %d (%s) por CAudioEngine::StartRadio(id,0)", id, kStationName[id]); g_startAudio(g_audioEngine, (signed char)id, 0); }
    else if (g_retuneAudio && g_audioEngine) { Log("cambio a %d (%s) por CAudioEngine::RetuneRadio (ultimo recurso)", id, kStationName[id]); g_retuneAudio(g_audioEngine, (signed char)id); }
    else if (g_retuneTrack && g_trackManager) { Log("cambio a %d (%s) por CAERadioTrackManager::RetuneRadio (ultimo recurso)", id, kStationName[id]); g_retuneTrack(g_trackManager, (signed char)id); }
    else { Log("!! sin Retune/Start con su objeto global: la rueda se ve pero NO puede cambiar la estacion (ver VHudRadioProbe.txt)"); return; }
    g_current = id;
    // verificar lo que el juego dice que suena ahora (diagnostico v1.2)
    if (g_getStationAudio && g_audioEngine) { int v = (int)g_getStationAudio(g_audioEngine); Log("  estacion leida tras cambio: %d (%s)%s", v, (v >= 1 && v <= 13) ? kStationName[v] : "?", v == id ? "" : "  <-- NO COINCIDE"); }
}
static void RefreshCurrent() {
    if (g_getStationAudio && g_audioEngine) { int v = (int)g_getStationAudio(g_audioEngine); if (v >= 1 && v <= 13) g_current = v; return; }
    if (g_getStationTrack && g_trackManager) { int v = (int)g_getStationTrack(g_trackManager); if (v >= 1 && v <= 13) g_current = v; }
}

// Abrir/cerrar la rueda con periodo refractario: el doble toque al claxon llega
// por DOS caminos a la vez (widget 7 y zona tactil) y sin esto la abriria y la
// cerraria en el mismo instante.
static void ToggleWheel(const char* why) {
    static auto lastToggle = std::chrono::steady_clock::now() - std::chrono::seconds(5);
    auto now = std::chrono::steady_clock::now();
    if (now - lastToggle < std::chrono::milliseconds(600)) return;
    lastToggle = now;
    if (!g_open) { g_open = true; g_ignoreNextUp = true; RefreshCurrent(); g_selected = g_current; Log("rueda abierta por %s (estacion actual %d %s)", why, g_current, kStationName[g_current]); PlayClick(false); }
    else { g_open = false; Log("rueda cerrada por %s", why); }
}


// ---- toque: como ARCX (type 2 = pulsar, 3 = mover, 1/4 = soltar) ----
static std::mutex g_inMtx;
struct TouchEv { int kind; float x, y; };
static std::vector<TouchEv> g_evs;
static bool g_cap = false;
static uintptr_t (*Touch_orig)(int, int, int, int);
static uintptr_t Touch_hook(int type, int idx, int x, int y) {
    bool down = (type == 2), up = (type == 1 || type == 4);
    if (idx == 0) {
        std::lock_guard<std::mutex> lk(g_inMtx);
        if (down) { g_evs.push_back({0, (float)x, (float)y}); g_evs.push_back({1, 0, 0}); }
        // v2.4: sin esto el panel no funcionaba - los eventos de soltar/mover solo
        // llegaban a ImGui con la rueda abierta; con el panel abierto (rueda cerrada)
        // ImGui recibia el toque de bajar pero nunca el de soltar
        else if (g_open || g_cap || g_settings || g_preview) { g_evs.push_back({0, (float)x, (float)y}); if (up) g_evs.push_back({2, 0, 0}); }
    }
    return Touch_orig ? Touch_orig(type, idx, x, y) : 0;
}

// ---- dibujo ImGui justo antes de eglSwapBuffers (patron ARCX) ----
static bool g_inited = false, g_failed = false;
static EGLBoolean (*Swap_orig)(EGLDisplay, EGLSurface);
static bool PatchGot(const char* libName, const char* sym, void* repl, void** orig) {
    struct Ctx { const char* lib; uintptr_t base; const ElfW(Phdr)* ph; int n; } c = {libName, 0, nullptr, 0};
    dl_iterate_phdr([](struct dl_phdr_info* i, size_t, void* d) -> int {
        Ctx* x = (Ctx*)d;
        if (i->dlpi_name && strstr(i->dlpi_name, x->lib)) { x->base = i->dlpi_addr; x->ph = i->dlpi_phdr; x->n = i->dlpi_phnum; return 1; }
        return 0; }, &c);
    if (!c.ph) return false;
    const ElfW(Dyn)* dyn = nullptr;
    for (int i = 0; i < c.n; i++) if (c.ph[i].p_type == PT_DYNAMIC) dyn = (const ElfW(Dyn)*)(c.base + c.ph[i].p_vaddr);
    if (!dyn) return false;
    auto fix = [&](uintptr_t p) { return p < c.base ? p + c.base : p; };
    const ElfW(Sym)* symtab = nullptr; const char* strtab = nullptr;
    const ElfW(Rela)* jmprel = nullptr; size_t jmpsz = 0; const ElfW(Rela)* rela = nullptr; size_t relasz = 0;
    for (; dyn->d_tag != DT_NULL; dyn++) switch (dyn->d_tag) {
        case DT_SYMTAB: symtab = (const ElfW(Sym)*)fix(dyn->d_un.d_ptr); break;
        case DT_STRTAB: strtab = (const char*)fix(dyn->d_un.d_ptr); break;
        case DT_JMPREL: jmprel = (const ElfW(Rela)*)fix(dyn->d_un.d_ptr); break;
        case DT_PLTRELSZ: jmpsz = dyn->d_un.d_val; break;
        case DT_RELA: rela = (const ElfW(Rela)*)fix(dyn->d_un.d_ptr); break;
        case DT_RELASZ: relasz = dyn->d_un.d_val; break;
        default: break; }
    if (!symtab || !strtab) return false;
    auto scan = [&](const ElfW(Rela)* r, size_t bytes) -> bool {
        if (!r) return false;
        for (size_t k = 0; k < bytes / sizeof(ElfW(Rela)); k++) {
            unsigned type = ELF64_R_TYPE(r[k].r_info); unsigned si = ELF64_R_SYM(r[k].r_info);
            if ((type != 1026 && type != 1025) || si == 0) continue;
            if (strcmp(strtab + symtab[si].st_name, sym) != 0) continue;
            void** slot = (void**)(c.base + r[k].r_offset);
            long pg = sysconf(_SC_PAGESIZE); uintptr_t page = (uintptr_t)slot & ~((uintptr_t)pg - 1);
            if (mprotect((void*)page, (size_t)pg, PROT_READ | PROT_WRITE) != 0) return false;
            *orig = *slot; *slot = repl; return true; }
        return false; };
    return scan(jmprel, jmpsz) || scan(rela, relasz);
}

static ImFont* g_fontTitle = nullptr;  // v2.1: letra del centro (Roboto Condensed negrita del sistema)
static ImFont* g_fontUI = nullptr;  // v2.3: letra del panel (32 px, legible al tacto)

// v2.2: geometria del boton flotante (icono + etiqueta con estacion y genero).
// El icono va abajo a la derecha y la etiqueta a su izquierda; el toque vale
// en todo el conjunto (icono o etiqueta).
static void FloatBtnGeom(float W, float H, ImVec2& iconPos, ImVec2& labelPos, ImVec2& labelSize, float& bs) {
    bs = H * 0.06f;
    iconPos = ImVec2(W - bs * 1.15f, H * 0.86f);  // v2.2: mas abajo
    ImFont* font = g_fontTitle ? g_fontTitle : ImGui::GetFont();
    float ns = H * 0.022f, ts = H * 0.018f;
    ImVec2 nsz = font->CalcTextSizeA(ns, FLT_MAX, 0.0f, kStationName[g_current], nullptr);
    ImVec2 tsz = font->CalcTextSizeA(ts, FLT_MAX, 0.0f, kStationTag[g_current], nullptr);
    float lw = (nsz.x > tsz.x ? nsz.x : tsz.x) + H * 0.02f;
    float lh = ns + ts + H * 0.02f;
    labelSize = ImVec2(lw, lh);
    labelPos = ImVec2(iconPos.x - H * 0.015f - lw, iconPos.y + bs * 0.5f - lh * 0.5f);
}
static bool FloatBtnHit(float x, float y, float W, float H) {
    ImVec2 ip, lp, ls; float bs;
    FloatBtnGeom(W, H, ip, lp, ls, bs);
    if (x >= ip.x && x <= ip.x + bs && y >= ip.y && y <= ip.y + bs) return true;
    if (x >= lp.x && x <= lp.x + ls.x && y >= lp.y && y <= lp.y + ls.y) return true;
    return false;
}

static void DrawWheel(float W, float H) {
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImFont* font = g_fontTitle ? g_fontTitle : ImGui::GetFont();
    // v2.2: boton flotante abajo a la derecha + etiqueta con estacion y genero
    ImVec2 bp, lp, ls; float bs;
    FloatBtnGeom(W, H, bp, lp, ls, bs);
    float ns = H * 0.022f, ts = H * 0.018f;
    dl->AddRectFilled(lp, ImVec2(lp.x + ls.x, lp.y + ls.y), IM_COL32(0, 0, 0, 140), H * 0.01f);
    dl->AddText(font, ns, ImVec2(lp.x + H * 0.01f, lp.y + H * 0.008f), IM_COL32(255,255,255,255), kStationName[g_current]);
    dl->AddText(font, ts, ImVec2(lp.x + H * 0.01f, lp.y + H * 0.008f + ns), IM_COL32(200,225,235,230), kStationTag[g_current]);
    if (g_tex[g_current]) dl->AddImage((ImTextureID)(uintptr_t)g_tex[g_current], bp, ImVec2(bp.x + bs, bp.y + bs));
    else { dl->AddCircleFilled(ImVec2(bp.x + bs/2, bp.y + bs/2), bs/2, IM_COL32(30,30,40,220)); }
    dl->AddCircle(ImVec2(bp.x + bs/2, bp.y + bs/2), bs/2, IM_COL32(255,255,255,200), 0, 2.0f);

    if (!g_open) return;
    // v2.2: rueda estilo GTA V - 12 logos en circulo + apagar la radio en el centro
    ImVec2 c(W * 0.5f, H * 0.5f);
    float R = WheelR(H);
    float Ri = WheelRi(H);
    float s = WheelIconS(H);
    dl->AddRectFilled(ImVec2(0, 0), ImVec2(W, H), IM_COL32(0, 0, 0, g_cfg.fondo * 255 / 100));  // oscuridad ajustable
    dl->AddCircle(c, Ri, IM_COL32(255, 255, 255, 70), 64, 2.0f);          // anillo fino
    for (int i = 1; i <= 13; i++) {
        float a0 = -3.14159265f/2 + (i-1) * 2*3.14159265f/13.0f;
        float a1 = a0 + 2*3.14159265f/13.0f;
        float am = (a0 + a1) * 0.5f;
        ImVec2 p(c.x + cosf(am) * Ri, c.y + sinf(am) * Ri);
        bool sel = (i == g_selected);
        float si = sel ? s * 1.12f : s;
        if (g_tex[i]) {
            ImVec4 tint = sel ? ImVec4(1, 1, 1, 1) : ImVec4(0.55f, 0.55f, 0.55f, 0.75f);
            dl->AddImage((ImTextureID)(uintptr_t)g_tex[i], ImVec2(p.x-si, p.y-si), ImVec2(p.x+si, p.y+si), ImVec2(0, 0), ImVec2(1, 1), ImColor(tint));
        } else dl->AddCircleFilled(p, si*0.5f, IM_COL32(255,255,255,120));
        if (sel) dl->AddCircle(p, si, RingColor(), 48, 3.5f);              // anillo de seleccion (color ajustable)
        else if (i == g_current) dl->AddCircle(p, si, IM_COL32(255, 255, 255, 220), 48, 2.0f);
    }
    // centro: nombre de la estacion seleccionada + genero (estilo GTA V)
    float nameSize = H * 0.05f, tagSize = H * 0.024f;
    const char* name = kStationName[g_selected];
    const char* tag = kStationTag[g_selected];
    ImVec2 nsz = font->CalcTextSizeA(nameSize, FLT_MAX, 0.0f, name, nullptr);
    ImVec2 tsz = font->CalcTextSizeA(tagSize, FLT_MAX, 0.0f, tag, nullptr);
    // v2.6: el texto del centro usa el color elegido (por defecto blanco)
    ImU32 txtCol = IM_COL32(g_cfg.textoR, g_cfg.textoG, g_cfg.textoB, 255);
    ImU32 tagCol = IM_COL32((int)(g_cfg.textoR * 0.8f), (int)(g_cfg.textoG * 0.8f), (int)(g_cfg.textoB * 0.8f), 230);
    dl->AddText(font, nameSize, ImVec2(c.x - nsz.x*0.5f, c.y - nsz.y*0.5f - H*0.015f), txtCol, name);
    dl->AddText(font, tagSize, ImVec2(c.x - tsz.x*0.5f, c.y + nsz.y*0.5f + H*0.005f), tagCol, tag);
    // v2.2: marca de vista previa
    if (g_preview) {
        const char* pv = "VISTA PREVIA";
        ImVec2 ps = font->CalcTextSizeA(H * 0.03f, FLT_MAX, 0.0f, pv, nullptr);
        dl->AddText(font, H * 0.03f, ImVec2(c.x - ps.x*0.5f, H * 0.03f), IM_COL32(255,255,255,230), pv);
    }
}

// ---- diseno del panel (azul casi oscuro + degradado azul -> azul claro) --------
static ImVec4 Mix(const ImVec4& a, const ImVec4& b, float t) {
    return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w);
}
static float Clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static const ImVec4 kPanelAccent(0.10f, 0.45f, 1.0f, 1.0f);   // azul (v2.6: el panel ya no es violeta)
static const ImVec4 kPanelGradBlue(0.05f, 0.55f, 1.00f, 1.0f); // azul del degradado del logo
static const ImVec4 kPanelGradCyan(0.10f, 0.65f, 1.0f, 1.0f); // azul claro del degradado
static ImVec4 PanelGradL() { return Mix(kPanelAccent, kPanelGradBlue, 0.55f); }
static ImVec4 PanelGradR() { return Mix(kPanelAccent, kPanelGradCyan, 0.55f); }
// Borde con degradado (izquierda -> derecha) siguiendo un rectangulo redondeado.
// Se dibuja como segmentos de linea gruesa con el color interpolado por la x;
// un circulo en cada union evita huecos en las esquinas (igual que en ARCX).
static void AddGradientBorder(ImDrawList* dl, const ImVec2& p0, const ImVec2& p1, float radius, float thick, const ImVec4& colL, const ImVec4& colR) {
    const float w = p1.x - p0.x, h = p1.y - p0.y;
    const float r = fminf(radius, fminf(w, h) * 0.5f);
    const int ES = 8, AS = 5;
    ImVec2 pts[64];
    int n = 0;
    auto push = [&](float x, float y) { if (n < 64) pts[n++] = ImVec2(x, y); };
    for (int i = 0; i <= ES; i++) push(p0.x + r + (w - 2.0f * r) * i / (float)ES, p0.y);
    for (int i = 1; i <= AS; i++) { float a = -1.5707963f + 1.5707963f * i / (float)AS; push(p1.x - r + cosf(a) * r, p0.y + r + sinf(a) * r); }
    for (int i = 1; i <= ES; i++) push(p1.x, p0.y + r + (h - 2.0f * r) * i / (float)ES);
    for (int i = 1; i <= AS; i++) { float a = 1.5707963f * i / (float)AS; push(p1.x - r + cosf(a) * r, p1.y - r + sinf(a) * r); }
    for (int i = 1; i <= ES; i++) push(p1.x - r - (w - 2.0f * r) * i / (float)ES, p1.y);
    for (int i = 1; i <= AS; i++) { float a = 1.5707963f + 1.5707963f * i / (float)AS; push(p0.x + r + cosf(a) * r, p1.y - r + sinf(a) * r); }
    for (int i = 1; i <= ES; i++) push(p0.x, p1.y - r - (h - 2.0f * r) * i / (float)ES);
    for (int i = 1; i <= AS; i++) { float a = 3.14159265f + 1.5707963f * i / (float)AS; push(p0.x + r + cosf(a) * r, p0.y + r + sinf(a) * r); }
    for (int i = 0; i < n; i++) {
        ImVec2 a = pts[i], b = pts[(i + 1) % n];
        float f = ((a.x + b.x) * 0.5f - p0.x) / w;
        if (f < 0.0f) f = 0.0f; if (f > 1.0f) f = 1.0f;
        ImU32 col = ImGui::GetColorU32(ImVec4(colL.x + (colR.x - colL.x) * f, colL.y + (colR.y - colL.y) * f,
                                              colL.z + (colR.z - colL.z) * f, colL.w + (colR.w - colL.w) * f));
        dl->AddLine(a, b, col, thick);
        dl->AddCircleFilled(a, thick * 0.5f, col, 6);
    }
}
// ---- panel RGB tactil (como el selector de ARCX 3.1.38) ----------------------
// El anillo se guarda en RGB (g_cfg), pero el panel trabaja en HSV: el cuadro
// grande es saturacion (X) / valor (Y) para el tono actual, y la barra vertical
// es el tono. El HSV se guarda aparte; cuando no se esta arrastrando, se
// sincroniza desde g_cfg (asi refleja tambien el restablecer).
static float g_pickH = 0.52f, g_pickS = 1.0f, g_pickV = 1.0f;
static bool g_cfgDirty = false;  // v2.5: algo cambio y aun no se guardo en el .ini
static int g_pickTarget = -1;  // v2.6: -1 = selector cerrado, 0 = anillo, 1 = texto del centro
static void PickerSyncFromRGB() {
    int r, g, b;
    if (g_pickTarget == 1) { r = g_cfg.textoR; g = g_cfg.textoG; b = g_cfg.textoB; }
    else { r = g_cfg.ringR; g = g_cfg.ringG; b = g_cfg.ringB; }
    ImGui::ColorConvertRGBtoHSV(r / 255.0f, g / 255.0f, b / 255.0f, g_pickH, g_pickS, g_pickV);
}
static void PickerApplyToRGB() {
    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(g_pickH, g_pickS, g_pickV, r, g, b);
    int ri = (int)(r * 255.0f + 0.5f), gi = (int)(g * 255.0f + 0.5f), bi = (int)(b * 255.0f + 0.5f);
    if (g_pickTarget == 1) { g_cfg.textoR = ri; g_cfg.textoG = gi; g_cfg.textoB = bi; }
    else { g_cfg.ringR = ri; g_cfg.ringG = gi; g_cfg.ringB = bi; }
}
// ---- controles propios del panel (no los de ImGui por defecto) ----------------
// Deslizador propio: etiqueta a la izquierda, valor a la derecha, pista
// redondeada con relleno azul y tirador blanco. Se arrastra con el dedo.
static bool PanelSlider(const char* label, int* v, int vmin, int vmax, const char* fmt, float H) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float w = ImGui::GetContentRegionAvail().x;
    ImVec2 lp = ImGui::GetCursorScreenPos();
    dl->AddText(lp, IM_COL32(150, 190, 255, 255), label);
    char vb[16]; snprintf(vb, sizeof vb, fmt, *v);
    ImVec2 vs = ImGui::CalcTextSize(vb);
    dl->AddText(ImVec2(lp.x + w - vs.x, lp.y), IM_COL32(255, 255, 255, 255), vb);
    ImGui::Dummy(ImVec2(w, ImGui::GetTextLineHeight()));
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    float zoneH = H * 0.045f;
    ImGui::InvisibleButton(label, ImVec2(w, zoneH));
    bool active = ImGui::IsItemActive();
    ImGuiIO& io = ImGui::GetIO();
    bool changed = false;
    if (active && io.MouseDown[0]) {
        float f = (io.MousePos.x - p0.x) / w;
        int nv = vmin + (int)(f * (float)(vmax - vmin) + 0.5f);
        if (nv < vmin) nv = vmin; if (nv > vmax) nv = vmax;
        if (nv != *v) { *v = nv; changed = true; }
    }
    float cy = p0.y + zoneH * 0.5f;
    float r = H * 0.013f;
    dl->AddRectFilled(ImVec2(p0.x, cy - r), ImVec2(p0.x + w, cy + r), IM_COL32(8, 14, 32, 255), r);
    float f = (float)(*v - vmin) / (float)(vmax - vmin);
    if (f > 0.0f) dl->AddRectFilled(ImVec2(p0.x, cy - r), ImVec2(p0.x + w * f, cy + r), IM_COL32(20, 110, 255, 255), r);
    float hx = p0.x + w * f;
    dl->AddCircleFilled(ImVec2(hx, cy), r + H*0.006f, IM_COL32(255, 255, 255, 255), 24);
    dl->AddCircle(ImVec2(hx, cy), r + H*0.006f, IM_COL32(20, 110, 255, 255), 24, H*0.002f);
    return changed;
}
// Boton propio: rectangulo redondeado con degradado azul y texto centrado.
// Devuelve true al soltar el dedo sobre el (como un boton de ImGui).
static bool PanelButton(const char* label, float H) {
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    float bh = H * 0.050f;
    bool clicked = ImGui::InvisibleButton(label, ImVec2(w, bh));
    bool active = ImGui::IsItemActive();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 c0 = active ? IM_COL32(40, 140, 255, 255) : IM_COL32(16, 90, 220, 255);
    ImU32 c1 = active ? IM_COL32(10, 90, 230, 255) : IM_COL32(8, 50, 160, 255);
    dl->AddRectFilledMultiColor(p0, ImVec2(p0.x + w, p0.y + bh), c0, c1, c1, c0);
    dl->AddRect(p0, ImVec2(p0.x + w, p0.y + bh), IM_COL32(120, 180, 255, 120), H*0.008f, 0, H*0.0015f);
    ImVec2 ts = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(p0.x + (w - ts.x) * 0.5f, p0.y + (bh - ts.y) * 0.5f), IM_COL32(255, 255, 255, 255), label);
    return clicked;
}
// Recuadro pequeno de color: al tocarlo se abre/cierra el panel RGB para ese color.
static bool PanelSwatch(const char* label, ImVec4 col, bool active, float H) {
    float s = H * 0.055f;
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::InvisibleButton(label, ImVec2(s, s));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p0, ImVec2(p0.x + s, p0.y + s), ImGui::GetColorU32(col), H*0.008f);
    if (active) dl->AddRect(p0, ImVec2(p0.x + s, p0.y + s), IM_COL32(255, 255, 255, 255), H*0.008f, 0, H*0.003f);
    else dl->AddRect(p0, ImVec2(p0.x + s, p0.y + s), IM_COL32(255, 255, 255, 90), H*0.008f, 0, H*0.0015f);
    return clicked;
}

// v2.2: panel de personalizacion (se abre con doble toque al boton flotante)
static void DrawSettings(float W, float H) {
    // v2.6: panel propio (no la ventana de ImGui por defecto) - azul casi oscuro,
    // sin barra de titulo, con deslizadores, recuadros y botones dibujados a mano
    const ImVec4 titleCol(0.75f, 0.85f, 1.0f, 1.0f);  // azul claro
    const ImVec4 subCol(0.55f, 0.65f, 0.85f, 1.0f);   // azul grisaceo
    ImGui::SetNextWindowPos(ImVec2(W * 0.5f, H * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(W * 0.44f, 0), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(H*0.014f, H*0.014f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(H*0.010f, H*0.010f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, H*0.015f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.05f, 0.13f, 0.97f));  // azul casi oscuro
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.10f, 0.45f, 1.0f, 0.5f));
    if (g_fontUI) ImGui::PushFont(g_fontUI);
    bool opened = ImGui::Begin("##vhudpanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    if (!opened) { ImGui::End(); if (g_fontUI) ImGui::PopFont(); ImGui::PopStyleColor(3); ImGui::PopStyleVar(4); return; }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    // titulo centrado + linea en degradado azul
    const char* title = "Personalizar rueda de radio";
    ImVec2 ts = ImGui::CalcTextSize(title);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - ts.x) * 0.5f);
    ImGui::TextColored(titleCol, "%s", title);
    ImVec2 lp = ImGui::GetCursorScreenPos();
    float lw = ImGui::GetContentRegionAvail().x;
    dl->AddRectFilledMultiColor(lp, ImVec2(lp.x + lw, lp.y + H*0.003f), IM_COL32(20, 110, 255, 255), IM_COL32(10, 200, 255, 255), IM_COL32(10, 200, 255, 255), IM_COL32(20, 110, 255, 255));
    ImGui::Dummy(ImVec2(lw, H*0.010f));
    ImGui::TextColored(subCol, "Fondo, tamano y colores. Se guardan en configs/VHudRadio.ini");
    ImGui::Separator();
    if (PanelSlider("Oscuridad del fondo", &g_cfg.fondo, 0, 100, "%d%%", H)) g_cfgDirty = true;
    if (PanelSlider("Tamano de la rueda", &g_cfg.tamano, 25, 45, "%d%%", H)) g_cfgDirty = true;
    // recuadros de color: al tocar uno se abre el panel RGB para ese color
    ImGui::BeginGroup();
    if (PanelSwatch("##swAnillo", ImVec4(g_cfg.ringR / 255.0f, g_cfg.ringG / 255.0f, g_cfg.ringB / 255.0f, 1.0f), g_pickTarget == 0, H)) { g_pickTarget = (g_pickTarget == 0) ? -1 : 0; PlayClick(false); }
    ImGui::SameLine(0, H*0.012f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (H*0.055f - ImGui::GetTextLineHeight()) * 0.5f);
    ImGui::Text("Anillo");
    ImGui::EndGroup();
    ImGui::SameLine(0, H*0.05f);
    ImGui::BeginGroup();
    if (PanelSwatch("##swTexto", ImVec4(g_cfg.textoR / 255.0f, g_cfg.textoG / 255.0f, g_cfg.textoB / 255.0f, 1.0f), g_pickTarget == 1, H)) { g_pickTarget = (g_pickTarget == 1) ? -1 : 1; PlayClick(false); }
    ImGui::SameLine(0, H*0.012f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (H*0.055f - ImGui::GetTextLineHeight()) * 0.5f);
    ImGui::Text("Texto del centro");
    ImGui::EndGroup();
    if (g_pickTarget >= 0) {
        ImGui::TextColored(titleCol, "%s", g_pickTarget == 1 ? "Color del texto del centro" : "Color del anillo");
        // ---- panel RGB tactil (como ARCX 3.1.38): cuadro saturacion/valor + barra de tono ----
        const float gap = H * 0.012f;
        const float hueW = H * 0.045f;
        float availW = ImGui::GetContentRegionAvail().x;
        float sq = availW - hueW - gap;
        if (sq > H * 0.20f) sq = H * 0.20f;   // que quepa el panel con el selector abierto
        if (sq < H * 0.14f) sq = H * 0.14f;
        float rowW = sq + gap + hueW;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availW - rowW) * 0.5f);
        ImVec2 sq0 = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##sv", ImVec2(sq, sq));
        bool svActive = ImGui::IsItemActive();
        ImGui::SameLine(0, gap);
        ImVec2 hue0 = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##hue", ImVec2(hueW, sq));
        bool hueActive = ImGui::IsItemActive();
        bool dragging = (svActive || hueActive) && io.MouseDown[0];
        if (!dragging) PickerSyncFromRGB();
        if (svActive && io.MouseDown[0]) {
            g_pickS = Clampf((io.MousePos.x - sq0.x) / sq, 0.0f, 1.0f);
            g_pickV = Clampf(1.0f - (io.MousePos.y - sq0.y) / sq, 0.0f, 1.0f);
            PickerApplyToRGB(); g_cfgDirty = true;
        } else if (hueActive && io.MouseDown[0]) {
            g_pickH = Clampf((io.MousePos.y - hue0.y) / sq, 0.0f, 1.0f);
            PickerApplyToRGB(); g_cfgDirty = true;
        }
        float hr, hg, hb;
        ImGui::ColorConvertHSVtoRGB(g_pickH, 1.0f, 1.0f, hr, hg, hb);
        const ImU32 hueCol = IM_COL32((int)(hr * 255.0f), (int)(hg * 255.0f), (int)(hb * 255.0f), 255);
        dl->AddRectFilledMultiColor(sq0, ImVec2(sq0.x + sq, sq0.y + sq),
            IM_COL32(255, 255, 255, 255), hueCol, IM_COL32(0, 0, 0, 255), IM_COL32(0, 0, 0, 255));
        dl->AddRect(sq0, ImVec2(sq0.x + sq, sq0.y + sq), IM_COL32(255, 255, 255, 60), H*0.006f, 0, H*0.0015f);
        int cr, cg, cb;
        if (g_pickTarget == 1) { cr = g_cfg.textoR; cg = g_cfg.textoG; cb = g_cfg.textoB; }
        else { cr = g_cfg.ringR; cg = g_cfg.ringG; cb = g_cfg.ringB; }
        const ImU32 pickU = ImGui::GetColorU32(ImVec4(cr / 255.0f, cg / 255.0f, cb / 255.0f, 1.0f));
        const ImVec2 mk(sq0.x + g_pickS * sq, sq0.y + (1.0f - g_pickV) * sq);
        dl->AddCircleFilled(mk, H*0.011f, pickU, 24);
        dl->AddCircle(mk, H*0.011f, IM_COL32(255, 255, 255, 255), 24, H*0.003f);
        const float seg = sq / 24.0f;
        for (int k = 0; k < 24; k++) {
            float rr, gg, bb;
            ImGui::ColorConvertHSVtoRGB((float)k / 24.0f, 1.0f, 1.0f, rr, gg, bb);
            dl->AddRectFilled(ImVec2(hue0.x, hue0.y + k * seg), ImVec2(hue0.x + hueW, hue0.y + (k + 1) * seg + 0.5f),
                IM_COL32((int)(rr * 255.0f), (int)(gg * 255.0f), (int)(bb * 255.0f), 255));
        }
        dl->AddRect(hue0, ImVec2(hue0.x + hueW, hue0.y + sq), IM_COL32(255, 255, 255, 60), H*0.006f, 0, H*0.0015f);
        const float hy = hue0.y + g_pickH * sq;
        dl->AddLine(ImVec2(hue0.x - H*0.004f, hy), ImVec2(hue0.x + hueW + H*0.004f, hy), IM_COL32(255, 255, 255, 255), H*0.003f);
        // abajo: muestra del color + valores RGB y hexadecimal
        const float readH = H * 0.045f;
        ImVec2 sw0 = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(H*0.040f, readH));
        dl->AddRectFilled(sw0, ImVec2(sw0.x + H*0.040f, sw0.y + readH), pickU, H*0.008f);
        dl->AddRect(sw0, ImVec2(sw0.x + H*0.040f, sw0.y + readH), IM_COL32(255, 255, 255, 255), H*0.008f, 0, H*0.002f);
        ImGui::SameLine(0, H*0.016f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (readH - ImGui::GetTextLineHeight()) * 0.5f);
        char vals[64];
        snprintf(vals, sizeof vals, "R %d   G %d   B %d    #%02X%02X%02X", cr, cg, cb, cr, cg, cb);
        ImGui::Text("%s", vals);
    }
    ImGui::Separator();
    if (PanelButton("Vista previa de la rueda", H)) {
        g_preview = true; g_open = true; g_selected = g_current; g_settings = false;
        PlayClick(false);
        Log("vista previa de la rueda");
    }
    if (PanelButton("Restablecer todo", H)) {
        g_cfg.fondo = 30; g_cfg.tamano = 36; g_cfg.ringR = 0; g_cfg.ringG = 210; g_cfg.ringB = 255; g_cfg.textoR = 255; g_cfg.textoG = 255; g_cfg.textoB = 255;
        g_cfgDirty = false; SaveCfg();
        Log("ajustes restablecidos");
    }
    if (PanelButton("Cerrar", H)) { g_settings = false; g_cfgDirty = false; SaveCfg(); Log("panel cerrado (ajustes guardados)"); }
    // guardar al soltar (por si se arrastro el color o un deslizador y no se pulso Cerrar)
    if (g_cfgDirty && io.MouseReleased[0]) { SaveCfg(); g_cfgDirty = false; }
    // borde en degradado azul alrededor de toda la ventana
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    ImVec2 wp = ImGui::GetWindowPos(), ws = ImGui::GetWindowSize();
    AddGradientBorder(fg, wp, ImVec2(wp.x + ws.x, wp.y + ws.y), H*0.015f, H*0.004f, PanelGradL(), PanelGradR());
    ImGui::End();
    if (g_fontUI) ImGui::PopFont();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(4);
}

// v2.7: un puntero es creible si, quitandole el byte de etiqueta (0xb4...),
// cae en el rango del heap del juego (0x100000000..0x1000000000000)
static bool PlausiblePtr(uintptr_t v) {
    uintptr_t t = v & 0x00FFFFFFFFFFFFFFULL;
    return t > 0x100000000ULL && t < 0x1000000000000ULL;
}
// v2.9: true si la pagina de p esta mapeada (mincore devuelve 0); quita el
// byte de etiqueta (0xb4...) antes de preguntar al kernel. Sin esto, leer un
// valor que "parece" puntero pero no esta mapeado cierra el juego (v2.8).
static bool IsMapped(const void* p) {
    uintptr_t a = ((uintptr_t)p & 0x00FFFFFFFFFFFFFFULL) & ~0xFFFULL;
    unsigned char vec = 0;
    return mincore((void*)a, 0x1000, &vec) == 0;
}
// v2.7: vuelca n bytes en hexadecimal a un buffer (para el diagnostico)
static void HexDump(char* out, size_t outSz, const void* p, int n) {
    const unsigned char* b = (const unsigned char*)p;
    int w = 0;
    for (int i = 0; i < n && w < (int)outSz - 4; i++) w += snprintf(out + w, outSz - (size_t)w, "%02X ", b[i]);
    if (w > 0 && w < (int)outSz) out[w - 1] = 0;  // quitar el ultimo espacio
}
static void DrawFrame(EGLDisplay dpy, EGLSurface surf) {
    if (g_failed) return;
    EGLContext ctx = eglGetCurrentContext();
    if (ctx == EGL_NO_CONTEXT) return;
    EGLint w = 0, h = 0; eglQuerySurface(dpy, surf, EGL_WIDTH, &w); eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
    if (w < 200 || h < 200) return;
    void* veh = FindPlayerVehicle ? FindPlayerVehicle(-1, false) : nullptr;
    bool wasInVehicle = g_inVehicle;
    g_inVehicle = veh != nullptr;
    // v2.9: diagnostico RadioType - la v2.8 SE CERRO al entrar a un auto de
    // policia: el escaneo leyo un valor (0x23F800000) que "parecia" un puntero
    // (cae en el rango 2^32..2^48) pero no esta mapeado, y al leerlo el juego
    // hizo SIGSEGV. Ahora cada puntero se valida con mincore (IsMapped) ANTES
    // de leerlo, y tambien la ventana que se vuelca. Rango mas amplio
    // (0x100..0x500) y hasta 20 candidatos. Aun no bloquea nada.
    if (g_inVehicle && !wasInVehicle && veh) {
        Log("  vehiculo: %p estacion=%d", veh, g_current);
        // v2.12: volcar los primeros 64 bytes del vehiculo para buscar
        // m_nModelIndex (el modelo es estable por vehiculo: bici 481/509/510,
        // tractor 531, patrulla 596-599). El bloqueo por rid fallo porque ese
        // byte cambia; el modelo no cambia.
        {
            char hex[64*3+1];
            HexDump(hex, sizeof hex, veh, 64);
            Log("  vehiculo bytes[0x00..0x3F]=%s", hex);
        }
        int cands = 0;
        for (int off = 0x100; off <= 0x500 && cands < 20; off += 8) {
            void* cand = *(void**)((char*)veh + off);
            if (!PlausiblePtr((uintptr_t)cand)) continue;
            if (!IsMapped(cand)) continue;  // v2.9: la pagina debe estar mapeada
            void* vt = *(void**)cand;
            if (!PlausiblePtr((uintptr_t)vt)) continue;
            if (!IsMapped((char*)cand + 0x70) || !IsMapped((char*)cand + 0xAF)) continue;
            char hex[64*3+1];
            HexDump(hex, sizeof hex, (char*)cand + 0x70, 64);
            Log("  ptrCand(0x%X)=%p bytes[0x70..0xAF]=%s", off, cand, hex);
            cands++;
        }
        if (cands == 0) Log("  ptrCand: ninguno en 0x100..0x500");
    }
    // v2.12: diagnostico de audio SIN bloqueo - la v2.11 demostro que rid(0x9A)
    // NO es estable (en un mismo carro dio 0, 2, 0, 0) y bloqueo carros y dejo
    // pasar la bici, al reves de lo que se queria. Se registra rt/rid solo
    // cuando cambian, para entender que es ese byte. El bloqueo por modelo
    // (m_nModelIndex) se hara cuando se encuentre su offset (ver el volcado
    // de los primeros bytes del vehiculo en el bloque de escaneo).
    {
        static void* lastVeh = nullptr;
        static int lastRt = -999, lastRid = -999;
        if (!g_inVehicle) { lastVeh = nullptr; lastRt = -999; lastRid = -999; }
        else if (veh && veh != lastVeh) { lastVeh = veh; lastRt = -999; lastRid = -999; }
        if (g_inVehicle && veh) {
            int aoffs[2] = {0x458, 0x1B0};
            for (int i = 0; i < 2; i++) {
                void* aud = *(void**)((char*)veh + aoffs[i]);
                if (!PlausiblePtr((uintptr_t)aud)) continue;
                if (!IsMapped(aud)) continue;
                if (!IsMapped((char*)aud + 0x9A) || !IsMapped((char*)aud + 0x9B)) continue;
                int rt = (int)*(signed char*)((char*)aud + 0x9B);
                int rid = (int)*(signed char*)((char*)aud + 0x9A);
                if (rt != lastRt || rid != lastRid) {
                    Log("  audio(0x%X): rt(0x9B)=%d rid(0x9A)=%d", aoffs[i], rt, rid);
                    lastRt = rt; lastRid = rid;
                }
                break;
            }
        }
    }
    // v2.6: pausa por varias senales (el byte 0x5C solo no basto en la v2.5):
    // CMenuManager::m_bMenuActive, CTimer::m_UserPause / m_CodePause, y si
    // CMenuManager::DrawFrontEnd se llamo en este frame (menu dibujado)
    bool fe = g_menuDrawn; g_menuDrawn = false;
    bool wasPaused = g_paused;
    g_paused = MenuActive() || (g_userPause && *g_userPause) || (g_codePause && *g_codePause) || fe;
    if (g_paused != wasPaused) { Log("menu de pausa: %s (byte=%d user=%d code=%d frontEnd=%d)", g_paused ? "abierto (UI oculta)" : "cerrado", MenuActive() ? 1 : 0, (g_userPause && *g_userPause) ? 1 : 0, (g_codePause && *g_codePause) ? 1 : 0, fe ? 1 : 0); if (g_menuCheck == 0) g_menuCheck = 1; }  // v2.4: el byte cambio: el offset es valido
    // v2.6: diagnostico del menu - si los bytes del objeto cambian, registrar que offsets
    if (g_menuManager) MenuSnapCheck();
    // v2.7: ralentizacion estilo GTA V - la v2.6 solo escribia los valores al
    // abrir/cerrar y el juego los borraba en el siguiente frame (el log decia
    // "activada" pero no se veia nada). Ahora se escriben EN CADA FRAME
    // mientras la rueda esta abierta: bandera + escala del slow-motion del
    // juego + la escala de tiempo directamente. Al cerrar se restauran los
    // valores que habia antes de abrir.
    if (g_slowActive || g_timeScale) {
        static bool slowOn = false;
        static float savedTimeScale = 1.0f, savedSlowScale = 0.3f;
        static bool savedSlowActive = false;
        static int slowTick = 0;
        if (g_open && !slowOn) {
            slowOn = true;
            if (g_timeScale) savedTimeScale = *g_timeScale;
            if (g_slowScale) savedSlowScale = *g_slowScale;
            if (g_slowActive) savedSlowActive = *g_slowActive;
            Log("ralentizacion: activada (guardada escala=%.2f)", savedTimeScale);
        } else if (!g_open && slowOn) {
            slowOn = false;
            if (g_slowActive) *g_slowActive = savedSlowActive;
            if (g_slowScale) *g_slowScale = savedSlowScale;
            if (g_timeScale) *g_timeScale = savedTimeScale;
            Log("ralentizacion: desactivada (restaurada escala=%.2f)", savedTimeScale);
        }
        if (slowOn) {
            if (g_slowActive) *g_slowActive = true;
            if (g_slowScale) *g_slowScale = 0.3f;
            if (g_timeScale) *g_timeScale = 0.3f;
            // diagnostico: cada segundo, leer lo que el juego tiene puesto
            // (si la escala dice 1.00, el juego esta borrando nuestros valores)
            if (++slowTick >= 60) {
                slowTick = 0;
                Log("ralentizacion: escala leida=%.2f activa=%d (deberia ser 0.30)", g_timeScale ? *g_timeScale : -1.0f, (g_slowActive && *g_slowActive) ? 1 : 0);
            }
        } else slowTick = 0;
    }
    if (!g_inited) {
        if (!FindPlayerPed || !FindPlayerPed(-1)) return;
        ImGui::CreateContext(); ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr; io.LogFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        ImGui::StyleColorsDark();
        // v2.1: letra mas parecida a GTA V - Roboto Condensed negrita del sistema
        io.Fonts->AddFontDefault();
        const char* titlePath = nullptr;
        for (const char* fp : {"/system/fonts/RobotoCondensed-Bold.ttf", "/system/fonts/Roboto-Bold.ttf", "/system/fonts/DroidSans-Bold.ttf"}) {
            g_fontTitle = io.Fonts->AddFontFromFileTTF(fp, 64.0f);
            if (g_fontTitle) { titlePath = fp; Log("fuente del titulo: %s", fp); break; }
        }
        if (!g_fontTitle) Log("  !! sin fuente del sistema: el titulo usa la letra de ImGui");
        // v2.3: segunda talla de la misma fuente para el panel (letra legible al tacto)
        if (titlePath) g_fontUI = io.Fonts->AddFontFromFileTTF(titlePath, 32.0f);
        if (!ImGui_ImplOpenGL3_Init(nullptr)) { Log("ImGui_ImplOpenGL3_Init fallo"); g_failed = true; return; }
        g_inited = true; LoadRadioTextures(); RefreshCurrent();
        Log("ImGui listo (%.0fx%.0f), estacion actual: %d (%s)", (float)w, (float)h, g_current, kStationName[g_current]);
    }
    ImGuiIO& io = ImGui::GetIO();
    static auto last = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - last).count(); last = now;
    io.DeltaTime = dt > 0.0001f ? dt : 1.0f/60.0f;
    io.DisplaySize = ImVec2((float)w, (float)h);
    // eventos de toque -> raton de ImGui; el boton y la rueda se resuelven por posicion
    std::vector<TouchEv> evs; { std::lock_guard<std::mutex> lk(g_inMtx); evs.swap(g_evs); }
    static bool mouseDown = false;
    for (auto& e : evs) {
        if (e.kind == 0) {
            io.AddMousePosEvent(e.x, e.y);
            if (g_open && !g_settings && !g_paused) {
                int seg = SegmentAt(e.x, e.y, (float)w, (float)h);
                if (seg >= 1) { g_wheelTouch = true; if (seg != g_selected) { g_selected = seg; PlayClick(true); Log("rueda: segmento %d (%s)", seg, kStationName[seg]); } }
            }
        } else if (e.kind == 1) { io.AddMouseButtonEvent(0, true); mouseDown = true;
            // v2.3: en el menu de pausa los toques son del juego; con el panel abierto, del panel (ImGui)
            if (g_paused) {
                // nada: el menu de pausa procesa el toque
            } else if (g_settings) {
                // nada: ImGui procesa el toque en NewFrame/Render
            } else if (g_preview) {
                // vista previa: solo se mira - tocar un segmento lo resalta, tocar fuera cierra la vista previa y vuelve el panel
                int seg = SegmentAt(io.MousePos.x, io.MousePos.y, (float)w, (float)h);
                if (seg >= 1) { g_selected = seg; g_wheelTouch = true; }
                else { g_preview = false; g_open = false; g_settings = true; Log("vista previa cerrada"); }
            } else if (g_open) {
                // v1.5: tocar un segmento lo elige al instante (antes solo el arrastre elegia)
                int seg = SegmentAt(io.MousePos.x, io.MousePos.y, (float)w, (float)h);
                if (seg >= 1) { g_selected = seg; g_wheelTouch = true; PlayClick(true); Log("rueda: segmento %d (%s) [toque]", seg, kStationName[seg]); }
                else g_wheelTouch = false;  // toque fuera del anillo: al soltar se cierra sin cambiar
            }
            // v1.2: doble toque al boton de claxon (zona derecha) abre/cierra la rueda
            if (g_inVehicle && !g_settings && !g_paused) {
                static int tapLogN = 0;
                if (tapLogN < 30 && io.MousePos.x > (float)w*0.70f) { tapLogN++; Log("tap vehiculo #%d en (%.0f, %.0f)%s", tapLogN, io.MousePos.x, io.MousePos.y, InHornRegion(io.MousePos.x, io.MousePos.y, (float)w, (float)h) ? "  <- zona claxon" : ""); }
                if (InHornRegion(io.MousePos.x, io.MousePos.y, (float)w, (float)h)) {
                    static auto lastHornTap = std::chrono::steady_clock::now() - std::chrono::seconds(5);
                    auto now = std::chrono::steady_clock::now();
                    if (now - lastHornTap < std::chrono::milliseconds(500)) {
                        if (g_preview) { g_preview = false; g_open = false; g_settings = true; Log("vista previa cerrada (claxon)"); }
                        else ToggleWheel("doble claxon");
                    }
                    lastHornTap = now;
                }
            }
            // v2.2: boton flotante - un toque = siguiente estacion; dos toques = panel de personalizacion
            if (!g_open && !g_preview && !g_settings && !g_paused && g_inVehicle) {
                if (FloatBtnHit(io.MousePos.x, io.MousePos.y, (float)w, (float)h)) {
                    static auto lastBtnTap = std::chrono::steady_clock::now() - std::chrono::seconds(5);
                    auto now = std::chrono::steady_clock::now();
                    if (now - lastBtnTap < std::chrono::milliseconds(400)) {
                        g_btnPending = false;
                        g_settings = true;
                        PlayClick(false);
                        Log("panel de personalizacion abierto");
                    } else {
                        g_btnPending = true; g_btnPendingAt = now;
                    }
                    lastBtnTap = now;
                }
            }
        } else if (e.kind == 2) { io.AddMouseButtonEvent(0, false); io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
            if (g_ignoreNextUp) { g_ignoreNextUp = false; mouseDown = false; }  // v1.5: el toque que abrio la rueda no la cierra
            else if (mouseDown && g_open && !g_paused) {
                if (g_preview) { /* v2.2: en vista previa no se aplica ni se cierra */ }
                else if (g_wheelTouch) { ApplyStation(g_selected); Log("rueda cerrada -> %d (%s)", g_selected, kStationName[g_selected]); g_open = false; }
                else { Log("rueda cerrada sin cambiar"); g_open = false; }
            }
            mouseDown = false; }
    }
    // v2.1: un solo toque al boton flotante = siguiente estacion (si no llega un segundo toque en 400 ms)
    if (g_btnPending) {
        if (g_open || g_settings || g_paused) g_btnPending = false;  // rueda, panel o pausa: no cambiar de estacion
        else if (std::chrono::steady_clock::now() - g_btnPendingAt > std::chrono::milliseconds(400)) {
            g_btnPending = false;
            int next = g_current % 13 + 1;
            ApplyStation(next);
            PlayClick(true);
            Log("boton: siguiente estacion %d (%s)", next, kStationName[next]);
        }
    }
    // v1.4: el juego actualiza la estacion leida con un poco de retraso tras StartRadio
    // (en el log v1.3 la lectura iba una estacion atrasada); re-leer cada ~0.5 s para
    // que el icono del boton flotante no se quede en la estacion anterior.
    if (g_inVehicle && !g_open) {
        static int frameN = 0;
        if (++frameN % 30 == 0) RefreshCurrent();
    }
    // v1.9: claxon por el boton 7 directo - el byte 84 (0x54) de m_pWidgets[7]
    // es 1 mientras el boton esta pulsado (confirmado por el log de la v1.8).
    // Doble toque (<500 ms) abre/cierra la rueda. Si el arreglo no resuelve
    // o el widget 7 es null, el claxon sigue por zona tactil.
    if (g_widgets && g_inVehicle) {
        static bool prevHornDown = false;
        static auto lastHornTap = std::chrono::steady_clock::now() - std::chrono::seconds(5);
        void* w7 = g_widgets[kHornWidgetId];
        bool hornDown = w7 && ((const unsigned char*)w7)[84] != 0;
        // v2.4: validar el offset de pausa - el claxon solo se puede pulsar con el juego activo;
        // si el byte de menu dice 1 mientras se pulsa, el offset 0x5C no es el menu: desactivar
        if (hornDown && !prevHornDown && g_menuCheck == 0 && MenuActive()) {
            g_menuCheck = -1;
            Log("  !! m_bMenuActive=1 con el claxon pulsado: el offset 0x5C no es el menu en este telefono; pausa desactivada");
        }
        if (!g_paused && hornDown && !prevHornDown) {
            auto now = std::chrono::steady_clock::now();
            if (now - lastHornTap < std::chrono::milliseconds(500)) {
                if (g_preview) { g_preview = false; g_open = false; g_settings = true; Log("vista previa cerrada (claxon)"); }
                else if (!g_settings) ToggleWheel("doble claxon (boton 7)");
            }
            lastHornTap = now;
        }
        prevHornDown = hornDown;
    }
    if (!g_inVehicle) { g_open = false; g_preview = false; g_settings = false; }  // v2.2: fuera del coche se cierra todo
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ImGui_ImplOpenGL3_NewFrame(); ImGui::NewFrame();
    if (g_inVehicle && !g_paused) DrawWheel((float)w, (float)h);
    if (g_settings && !g_paused) DrawSettings((float)w, (float)h);
    ImGui::Render(); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

static EGLBoolean Swap_hook(EGLDisplay d, EGLSurface s) {
    DrawFrame(d, s);
    return Swap_orig(d, s);
}

extern "C" void OnModLoad() {
    static const char* kFilesDirs[] = {
        "/storage/emulated/0/Android_unprotected/data/com.rockstargames.gtasa/files",
        "/storage/emulated/0/Android/data/com.rockstargames.gtasa/files",
        "/sdcard/Android/data/com.rockstargames.gtasa/files",
        "/storage/self/primary/Android/data/com.rockstargames.gtasa/files",
    };
    std::string files;
    for (auto d : kFilesDirs) { struct stat st; if (stat(d, &st) == 0) { files = d; break; } }
    if (files.empty()) files = kFilesDirs[1];
    g_gameFilesDir = files;
    // v2.3: el .ini va en data/configs (AML: android_unprotected/... o android/.../configs),
    // NO en files/configs - data es la carpeta padre de files
    g_gameDataDir = files;
    if (g_gameDataDir.size() > 6 && g_gameDataDir.compare(g_gameDataDir.size()-6, 6, "/files") == 0)
        g_gameDataDir.resize(g_gameDataDir.size()-6);
    g_logPath = files + "/VHudRadioLog.txt";
    { FILE* f = fopen(g_logPath.c_str(), "w"); if (f) { fprintf(f, "# VHudRadioLog - VHud Radio Wheel %s\n", VHUDRADIO_VERSION); fclose(f); } }
    g_vhudDir = "/storage/emulated/0/vhud";
    mkdir(g_vhudDir.c_str(), 0777); mkdir((g_vhudDir + "/radio").c_str(), 0777);
    mkdir((g_gameFilesDir + "/vhud").c_str(), 0777); mkdir((g_gameFilesDir + "/vhud/radio").c_str(), 0777);
    mkdir((g_gameDataDir + "/configs").c_str(), 0777);
    LoadCfg();
    LoadHornPos();
    Log("  centro del claxon: (%.3f, %.3f)", g_hornCx, g_hornCy);
    Log("== VHud Radio Wheel %s cargado ==", VHUDRADIO_VERSION);

    void* h = aml->GetLibHandle("libGTASA.so");
    #define SYM(var, name) do { *(void**)&var = (void*)aml->GetSym(h, name); Log("  %s %s", var ? "ok    " : "FALTA " , name); } while (0)
    SYM(FindPlayerPed, "_Z13FindPlayerPedi");
    SYM(FindPlayerVehicle, "_Z17FindPlayerVehicleib");
    SYM(g_retuneAudio, "_ZN12CAudioEngine11RetuneRadioEa");
    SYM(g_retuneTrack, "_ZN20CAERadioTrackManager11RetuneRadioEa");
    SYM(g_getStationAudio, "_ZN12CAudioEngine24GetCurrentRadioStationIDEv");
    SYM(g_getStationTrack, "_ZN20CAERadioTrackManager24GetCurrentRadioStationIDEv");
    SYM(g_startAudio, "_ZN12CAudioEngine10StartRadioEaa");
    SYM(g_startTrack, "_ZN20CAERadioTrackManager10StartRadioEaafh");
    SYM(g_audioEngine, "AudioEngine");
    SYM(g_trackManager, "AERadioTrackManager");
    SYM(g_widgets, "_ZN15CTouchInterface10m_pWidgetsE");
    if (g_widgets) Log("  widgets: m_pWidgets=%p w0=%p w2=%p w7=%p", (void*)g_widgets, g_widgets[0], g_widgets[2], g_widgets[7]);
    else Log("  !! m_pWidgets no resolvio: no se puede leer el boton 7 directo; el claxon sigue por zona tactil/calibrada");
    SYM(g_menuManager, "FrontEndMenuManager");
    if (!g_menuManager) SYM(g_menuManager, "_ZN13CMenuManager18FrontEndMenuManagerE");
    if (g_menuManager) {
        void* sym = g_menuManager;
        g_menuManager = ResolveMenuManager(g_menuManager);
        Log("  menu de pausa: FrontEndMenuManager sim=%p obj=%p m_bMenuActive(0x5C)=%d", sym, g_menuManager, ((const unsigned char*)g_menuManager)[0x5C]);
    }
    else Log("  !! FrontEndMenuManager no resolvio: la UI no se ocultara en el menu de pausa");
    SYM(g_userPause, "_ZN6CTimer11m_UserPauseE");
    SYM(g_codePause, "_ZN6CTimer11m_CodePauseE");
    SYM(g_slowActive, "_ZN6CTimer17bSlowMotionActiveE");
    SYM(g_slowScale, "_ZN6CTimer19ms_fSlowMotionScaleE");
    SYM(g_timeScale, "_ZN6CTimer13ms_fTimeScaleE");
    if (!g_slowActive && !g_timeScale) Log("  !! sin escala de tiempo: no habra ralentizacion al abrir la rueda");
    void* dfe = (void*)aml->GetSym(h, "_ZN12CMenuManager12DrawFrontEndEv");
    Log("  %s CMenuManager::DrawFrontEnd", dfe ? "ok" : "FALTA");
    if (dfe) aml->Hook(dfe, (void*)DrawFrontEnd_hook, (void**)&DrawFrontEnd_orig);
    #undef SYM
    Log("  globales: AudioEngine=%p AERadioTrackManager=%p", g_audioEngine, g_trackManager);
    if (!g_audioEngine || !g_trackManager) Log("  !! falta un objeto global de radio: no se llamara a ninguna funcion de radio");
    if (!g_retuneAudio && !g_retuneTrack && !g_startAudio && !g_startTrack)
        Log("!! ningun Retune/Start resolvio: la rueda se dibujara pero no cambiara la radio. Ver VHudRadioProbe.txt");

    void* touch = (void*)aml->GetSym(h, "_Z14AND_TouchEventiiii");
    Log("  %s AND_TouchEvent", touch ? "ok" : "FALTA");
    if (touch) aml->Hook(touch, (void*)Touch_hook, (void**)&Touch_orig);
    bool got = PatchGot("libGTASA.so", "eglSwapBuffers", (void*)Swap_hook, (void**)&Swap_orig);
    Log("  eglSwapBuffers por GOT: %s", got ? "ok" : "no encontrado");
    if (!got) {
        void* fn = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
        if (fn) { aml->Hook(fn, (void*)Swap_hook, (void**)&Swap_orig); Log("  eglSwapBuffers por hook directo"); }
        else Log("  !! no se pudo enganchar eglSwapBuffers: la rueda no se dibujara");
    }
}
