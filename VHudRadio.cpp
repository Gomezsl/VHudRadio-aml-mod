// VHudRadio.cpp - Rueda de radio estilo GTA V para GTA SA Android 2.10 (AML .so), v2.3 2.3.0-beta.
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
//   restablecer todo. Se guarda en configs/VHudRadio.ini (files del juego).
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

MYMODCFG(net.hunter09.vhudradio, VHud Radio Wheel, 2.3.0-beta, Hunter09)
NEEDGAME(com.rockstargames.gtasa)
#define VHUDRADIO_VERSION "2.3.0-beta"

// ---- log: VHudRadioLog.txt junto al juego ----
static std::string g_logPath;
static std::string g_gameFilesDir;  // files del juego (se resuelve en OnModLoad)
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

// ---- ajustes de la rueda (v2.2): se guardan en configs/VHudRadio.ini ----
// [VHudRadio] fondo=30 (0..100, oscuridad del fondo) tamano=36 (25..45, % de H)
// anillo=0,210,255 (color del anillo de seleccion, 0..255)
struct WheelCfg { int fondo = 30; int tamano = 36; int ringR = 0; int ringG = 210; int ringB = 255; };
static WheelCfg g_cfg;
static bool g_settings = false;   // v2.3: panel de personalizacion - controles grandes y legibles al tacto
static void DrawSettings(float W, float H) {
    ImGui::SetNextWindowPos(ImVec2(W * 0.5f, H * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(W * 0.42f, 0), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(H*0.02f, H*0.02f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(H*0.012f, H*0.015f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(H*0.015f, H*0.012f));
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, H*0.03f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
    if (g_fontUI) ImGui::PushFont(g_fontUI);
    bool opened = ImGui::Begin("Personalizar rueda de radio", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    if (!opened) { ImGui::End(); if (g_fontUI) ImGui::PopFont(); ImGui::PopStyleVar(5); return; }
    ImGui::Text("Fondo, tamano y color del anillo.");
    ImGui::Text("Se guardan en configs/VHudRadio.ini");
    ImGui::Separator();
    ImGui::Text("Oscuridad del fondo");
    ImGui::SliderInt("##fondo", &g_cfg.fondo, 0, 100, "%d%%");
    ImGui::Text("Tamano de la rueda");
    ImGui::SliderInt("##tamano", &g_cfg.tamano, 25, 45, "%d%%");
    ImGui::Text("Color del anillo");
    // colores rapidos: botones grandes, faciles de tocar
    static const ImVec4 kQuick[8] = {
        ImVec4(0.0f, 0.82f, 1.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
        ImVec4(1.0f, 0.2f, 0.2f, 1.0f), ImVec4(0.2f, 0.9f, 0.2f, 1.0f),
        ImVec4(1.0f, 0.85f, 0.0f, 1.0f), ImVec4(1.0f, 0.3f, 0.9f, 1.0f),
        ImVec4(1.0f, 0.55f, 0.0f, 1.0f), ImVec4(0.3f, 0.4f, 1.0f, 1.0f)};
    for (int qi = 0; qi < 8; qi++) {
        if (qi > 0) ImGui::SameLine();
        ImGui::PushID(qi);
        if (ImGui::ColorButton("##q", kQuick[qi], ImGuiColorEditFlags_NoTooltip, ImVec2(H*0.045f, H*0.045f))) {
            g_cfg.ringR = (int)(kQuick[qi].x * 255.0f + 0.5f);
            g_cfg.ringG = (int)(kQuick[qi].y * 255.0f + 0.5f);
            g_cfg.ringB = (int)(kQuick[qi].z * 255.0f + 0.5f);
        }
        ImGui::PopID();
    }
    float col[3] = {g_cfg.ringR / 255.0f, g_cfg.ringG / 255.0f, g_cfg.ringB / 255.0f};
    if (ImGui::ColorEdit3("##anillo", col, ImGuiColorEditFlags_NoInputs)) {
        g_cfg.ringR = (int)(col[0] * 255.0f + 0.5f);
        g_cfg.ringG = (int)(col[1] * 255.0f + 0.5f);
        g_cfg.ringB = (int)(col[2] * 255.0f + 0.5f);
    }
    ImGui::Separator();
    if (ImGui::Button("Vista previa de la rueda", ImVec2(-1, 0))) {
        g_preview = true; g_open = true; g_selected = g_current; g_settings = false;
        PlayClick(false);
        Log("vista previa de la rueda");
    }
    if (ImGui::Button("Restablecer todo", ImVec2(-1, 0))) {
        g_cfg.fondo = 30; g_cfg.tamano = 36; g_cfg.ringR = 0; g_cfg.ringG = 210; g_cfg.ringB = 255;
        SaveCfg();
        Log("ajustes restablecidos");
    }
    if (ImGui::Button("Cerrar", ImVec2(-1, 0))) { g_settings = false; SaveCfg(); Log("panel cerrado (ajustes guardados)"); }
    ImGui::Text("Vista previa: toca un segmento para verlo, toca fuera para volver.");
    ImGui::End();
    if (g_fontUI) ImGui::PopFont();
    ImGui::PopStyleVar(5);
}

static void DrawFrame(EGLDisplay dpy, EGLSurface surf) {
    if (g_failed) return;
    EGLContext ctx = eglGetCurrentContext();
    if (ctx == EGL_NO_CONTEXT) return;
    EGLint w = 0, h = 0; eglQuerySurface(dpy, surf, EGL_WIDTH, &w); eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
    if (w < 200 || h < 200) return;
    g_inVehicle = FindPlayerVehicle && FindPlayerVehicle(-1, false) != nullptr;
    // v2.3: menu de pausa abierto -> esconder toda la UI (rueda, boton, panel)
    bool wasPaused = g_paused;
    g_paused = MenuActive();
    if (g_paused != wasPaused) Log("menu de pausa: %s", g_paused ? "abierto (UI oculta)" : "cerrado");
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
    if (g_widgets && g_inVehicle && !g_paused) {
        static bool prevHornDown = false;
        static auto lastHornTap = std::chrono::steady_clock::now() - std::chrono::seconds(5);
        void* w7 = g_widgets[kHornWidgetId];
        bool hornDown = w7 && ((const unsigned char*)w7)[84] != 0;
        if (hornDown && !prevHornDown) {
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
    g_logPath = files + "/VHudRadioLog.txt";
    { FILE* f = fopen(g_logPath.c_str(), "w"); if (f) { fprintf(f, "# VHudRadioLog - VHud Radio Wheel %s\n", VHUDRADIO_VERSION); fclose(f); } }
    g_vhudDir = "/storage/emulated/0/vhud";
    mkdir(g_vhudDir.c_str(), 0777); mkdir((g_vhudDir + "/radio").c_str(), 0777);
    mkdir((g_gameFilesDir + "/vhud").c_str(), 0777); mkdir((g_gameFilesDir + "/vhud/radio").c_str(), 0777);
    mkdir((g_gameFilesDir + "/configs").c_str(), 0777);
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
    if (g_menuManager) Log("  menu de pausa: FrontEndMenuManager=%p (m_bMenuActive en 0x5C)", g_menuManager);
    else Log("  !! FrontEndMenuManager no resolvio: la UI no se ocultara en el menu de pausa");
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
