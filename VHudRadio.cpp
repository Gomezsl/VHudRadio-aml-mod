// VHudRadio.cpp - Rueda de radio estilo GTA V para GTA SA Android 2.10 (AML .so), v2.2 2.2.0-beta.
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

MYMODCFG(net.hunter09.vhudradio, VHud Radio Wheel, 2.2.0-beta, Hunter09)
NEEDGAME(com.rockstargames.gtasa)
#define VHUDRADIO_VERSION "2.2.0-beta"

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
static bool g_settings = false;   // v2.2: panel de personalizacion abierto
static bool g_preview = false;    // v2.2: vista previa de la rueda (solo se ve, no cambia la estacion)
static std::string CfgPath() { return g_gameFilesDir + "/configs/VHudRadio.ini"; }
static void SaveCfg() {
    mkdir((g_gameFilesDir + "/configs").c_str(), 0777);
    FILE* f = fopen(CfgPath().c_str(), "w");
    if (!f) return;
    fprintf(f, "[VHudRadio]\n");
    fprintf(f, "fondo=%d\n", g_cfg.fondo);
    fprintf(f, "tamano=%d\n", g_cfg.tamano);
    fprintf(f, "anillo=%d,%d,%d\n", g_cfg.ringR, g_cfg.ringG, g_cfg.ringB);
    fclose(f);
}
static void LoadCfg() {
    g_cfg.fondo = 30; g_cfg.tamano = 36; g_cfg.ringR = 0; g_cfg.ringG = 210; g_cfg.ringB = 255;
    FILE* f = fopen(CfgPath().c_str(), "r");
    if (!f) { SaveCfg(); return; }  // no existe: crearlo con los valores por defecto
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
    }
    fclose(f);
    if (g_cfg.fondo < 0) g_cfg.fondo = 0; if (g_cfg.fondo > 100) g_cfg.fondo = 100;
    if (g_cfg.tamano < 25) g_cfg.tamano = 25; if (g_cfg.tamano > 45) g_cfg.tamano = 45;
    if (g_cfg.ringR < 0) g_cfg.ringR = 0; if (g_cfg.ringR > 255) g_cfg.ringR = 255;
    if (g_cfg.ringG < 0) g_cfg.ringG = 0; if (g_cfg.ringG > 255) g_cfg.ringG = 255;
    if (g_cfg.ringB < 0) g_cfg.ringB = 0; if (g_cfg.ringB > 255) g_cfg.ringB = 255;
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

// v2.2: segmento bajo un punto (13 = centro: apagar la radio; 0 = fuera de la rueda)
static int SegmentAt(float x, float y, float W, float H) {
    float dx = x - W*0.5f, dy = y - H*0.5f;
    float dist = sqrtf(dx*dx + dy*dy);
    if (dist < H*0.11f) return 13;  // centro = apagar la radio
    float Ri = WheelRi(H), s = WheelIconS(H);
    if (dist < Ri - s*1.3f || dist > Ri + s*1.3f) return 0;
    float ang = atan2f(dy, dx) + 3.14159265f/2; if (ang < 0) ang += 2*3.14159265f;
    int seg = (int)(ang / (2*3.14159265f/12.0f)) + 1; if (seg > 12) seg = 12;
    return seg;
}
static bool g_inVehicle = false;

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
        else if (g_open || g_cap) { g_evs.push_back({0, (float)x, (float)y}); if (up) g_evs.push_back({2, 0, 0}); }
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
    for (int i = 1; i <= 12; i++) {
        float a0 = -3.14159265f/2 + (i-1) * 2*3.14159265f/12.0f;
        float a1 = a0 + 2*3.14159265f/12.0f;
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
    // centro: apagar la radio (estacion 13)
    float cs = H * 0.085f;
    if (g_tex[13]) dl->AddImage((ImTextureID)(uintptr_t)g_tex[13], ImVec2(c.x-cs, c.y-cs), ImVec2(c.x+cs, c.y+cs));
    else dl->AddCircleFilled(c, cs, IM_COL32(30,30,40,220));
    if (g_selected == 13) dl->AddCircle(c, cs, RingColor(), 48, 3.5f);
    else if (g_current == 13) dl->AddCircle(c, cs, IM_COL32(255, 255, 255, 220), 48, 2.0f);
    // nombre de la estacion seleccionada + genero, debajo del icono central
    float nameSize = H * 0.05f, tagSize = H * 0.024f;
    const char* name = kStationName[g_selected];
    const char* tag = kStationTag[g_selected];
    ImVec2 nsz = font->CalcTextSizeA(nameSize, FLT_MAX, 0.0f, name, nullptr);
    ImVec2 tsz = font->CalcTextSizeA(tagSize, FLT_MAX, 0.0f, tag, nullptr);
    float ty = c.y + cs + H * 0.015f;
    dl->AddText(font, nameSize, ImVec2(c.x - nsz.x*0.5f, ty), IM_COL32(255,255,255,255), name);
    dl->AddText(font, tagSize, ImVec2(c.x - tsz.x*0.5f, ty + nsz.y + H * 0.002f), IM_COL32(200,225,235,230), tag);
    // v2.2: marca de vista previa
    if (g_preview) {
        const char* pv = "VISTA PREVIA";
        ImVec2 ps = font->CalcTextSizeA(H * 0.03f, FLT_MAX, 0.0f, pv, nullptr);
        dl->AddText(font, H * 0.03f, ImVec2(c.x - ps.x*0.5f, H * 0.03f), IM_COL32(255,255,255,230), pv);
    }
}

// v2.2: panel de personalizacion (se abre con doble toque al boton flotante)
static void DrawSettings(float W, float H) {
    ImGui::SetNextWindowPos(ImVec2(W * 0.5f, H * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(W * 0.55f, 0), ImGuiCond_Always);
    if (!ImGui::Begin("Personalizar rueda de radio", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) { ImGui::End(); return; }
    ImGui::Text("Fondo, tamano y color del anillo. Se guardan en configs/VHudRadio.ini");
    ImGui::SliderInt("Oscuridad del fondo", &g_cfg.fondo, 0, 100, "%d%%");
    ImGui::SliderInt("Tamano de la rueda", &g_cfg.tamano, 25, 45, "%d%%");
    float col[3] = {g_cfg.ringR / 255.0f, g_cfg.ringG / 255.0f, g_cfg.ringB / 255.0f};
    if (ImGui::ColorEdit3("Color del anillo", col)) {
        g_cfg.ringR = (int)(col[0] * 255.0f + 0.5f);
        g_cfg.ringG = (int)(col[1] * 255.0f + 0.5f);
        g_cfg.ringB = (int)(col[2] * 255.0f + 0.5f);
    }
    if (ImGui::Button("Vista previa de la rueda")) {
        g_preview = true; g_open = true; g_selected = g_current; g_settings = false;
        PlayClick(false);
        Log("vista previa de la rueda");
    }
    ImGui::SameLine();
    if (ImGui::Button("Restablecer todo")) {
        g_cfg.fondo = 30; g_cfg.tamano = 36; g_cfg.ringR = 0; g_cfg.ringG = 210; g_cfg.ringB = 255;
        SaveCfg();
        Log("ajustes restablecidos");
    }
    ImGui::SameLine();
    if (ImGui::Button("Cerrar")) { g_settings = false; SaveCfg(); Log("panel cerrado (ajustes guardados)"); }
    ImGui::Text("Vista previa: toca un segmento para verlo, toca fuera para volver.");
    ImGui::End();
}

static void DrawFrame(EGLDisplay dpy, EGLSurface surf) {
    if (g_failed) return;
    EGLContext ctx = eglGetCurrentContext();
    if (ctx == EGL_NO_CONTEXT) return;
    EGLint w = 0, h = 0; eglQuerySurface(dpy, surf, EGL_WIDTH, &w); eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
    if (w < 200 || h < 200) return;
    g_inVehicle = FindPlayerVehicle && FindPlayerVehicle(-1, false) != nullptr;
    if (!g_inited) {
        if (!FindPlayerPed || !FindPlayerPed(-1)) return;
        ImGui::CreateContext(); ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr; io.LogFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        ImGui::StyleColorsDark();
        // v2.1: letra mas parecida a GTA V - Roboto Condensed negrita del sistema
        io.Fonts->AddFontDefault();
        for (const char* fp : {"/system/fonts/RobotoCondensed-Bold.ttf", "/system/fonts/Roboto-Bold.ttf", "/system/fonts/DroidSans-Bold.ttf"}) {
            g_fontTitle = io.Fonts->AddFontFromFileTTF(fp, 64.0f);
            if (g_fontTitle) { Log("fuente del titulo: %s", fp); break; }
        }
        if (!g_fontTitle) Log("  !! sin fuente del sistema: el titulo usa la letra de ImGui");
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
            if (g_open && !g_settings) {
                int seg = SegmentAt(e.x, e.y, (float)w, (float)h);
                if (seg >= 1) { g_wheelTouch = true; if (seg != g_selected) { g_selected = seg; PlayClick(true); Log("rueda: segmento %d (%s)", seg, kStationName[seg]); } }
            }
        } else if (e.kind == 1) { io.AddMouseButtonEvent(0, true); mouseDown = true;
            // v2.2: con el panel abierto, los toques son del panel (ImGui); nada de rueda/claxon/boton
            if (g_settings) {
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
            if (g_inVehicle && !g_settings) {
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
            if (!g_open && !g_preview && !g_settings && g_inVehicle) {
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
            else if (mouseDown && g_open) {
                if (g_preview) { /* v2.2: en vista previa no se aplica ni se cierra */ }
                else if (g_wheelTouch) { ApplyStation(g_selected); Log("rueda cerrada -> %d (%s)", g_selected, kStationName[g_selected]); g_open = false; }
                else { Log("rueda cerrada sin cambiar"); g_open = false; }
            }
            mouseDown = false; }
    }
    // v2.1: un solo toque al boton flotante = siguiente estacion (si no llega un segundo toque en 400 ms)
    if (g_btnPending) {
        if (g_open || g_settings) g_btnPending = false;  // la rueda se abrio o el panel: no cambiar de estacion
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
    if (g_inVehicle) DrawWheel((float)w, (float)h);
    if (g_settings) DrawSettings((float)w, (float)h);
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
