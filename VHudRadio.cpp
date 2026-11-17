// VHudRadio.cpp - Rueda de radio estilo GTA V para GTA SA Android 2.10 (AML .so), v1.8 1.8.0-beta.
// Repo aparte: Gomezsl/VHudRadio-aml-mod. ARCX no se toca.
// Idea: v-hud de PC leia los iconos de VHud\radio\radio_1..13.png sin importarlos.
// Aqui igual: el plugin lee /storage/emulated/0/vhud/radio/radio_1.png ... radio_13.png
// (+ radio_current.png) al arrancar, con stb_image, y los dibuja con ImGui.
// La rueda es NUEVA (en PC la radio no era rueda, solo un icono 2 s al cambiar):
// 13 segmentos en circulo, se abre con un boton flotante dentro de un vehiculo,
// se arrastra el dedo y al soltar cambia la estacion.
//
// Simbolos de radio: VERIFICADOS por VHudRadioProbe 1.0.0 en el telefono de Daniel
// (VHudRadioProbe.txt, 2026-11-16, 18/23 ok): TODOS los candidatos moviles resolvieron,
// las variantes de PC dieron FALTA (como se esperaba).
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
// rueda trae un boton "Calibrar claxon": lo tocas, luego tocas el boton de claxon
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

MYMODCFG(net.hunter09.vhudradio, VHud Radio Wheel, 1.8.0-beta, Hunter09)
NEEDGAME(com.rockstargames.gtasa)
#define VHUDRADIO_VERSION "1.8.0-beta"

// ---- log: VHudRadioLog.txt junto al juego ----
static std::string g_logPath;
static void Log(const char* fmt, ...) {
    // v1.7: buf 8192 (antes 600 y el volcado CPad de 2048 bytes se truncaba a ~295)
    char buf[8192];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    __android_log_print(ANDROID_LOG_INFO, "VHudRadio", "%s", buf);
    if (!g_logPath.empty()) { FILE* f = fopen(g_logPath.c_str(), "a"); if (f) { fprintf(f, "%s\n", buf); fclose(f); } }
}

// ---- estaciones 1..13 (VERIFIED por plugin-sdk eRadioID + GTAMods + v-hud) ----
static const char* kStationName[14] = { "", "Playback FM", "K-Rose", "K-DST", "Bounce FM", "SF-UR",
    "Radio Los Santos", "Radio X", "CSR 103.9", "K-Jah West", "Master Sounds 98.3",
    "WCTR", "User tracks", "Radio Off" };

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
// v1.6: el centro ya no es fijo: se calibra con un toque (boton "Calibrar claxon"
// bajo la rueda) y se guarda en horn_pos.txt. Si mueves el boton de claxon en los
// controles del juego, lo recalibras. El toque pasa al juego igual: el claxon
// suena y la rueda se abre.
static float g_hornCx = 0.938f, g_hornCy = 0.683f;  // centro del claxon (fraccion de W/H)
static bool g_calibrating = false;  // v1.6: esperando el toque al boton de claxon
static bool InHornRegion(float x, float y, float W, float H) {
    float cx = W*g_hornCx, cy = H*g_hornCy, r = H*0.07f;
    float dx = x - cx, dy = y - cy;
    return dx*dx + dy*dy <= r*r;
}
static void LoadHornPos() {
    FILE* f = fopen("/storage/emulated/0/vhud/horn_pos.txt", "r");
    if (f) { float x = 0, y = 0; if (fscanf(f, "%f %f", &x, &y) == 2 && x > 0.05f && x < 0.99f && y > 0.05f && y < 0.99f) { g_hornCx = x; g_hornCy = y; } fclose(f); }
}
static void SaveHornPos() {
    FILE* f = fopen("/storage/emulated/0/vhud/horn_pos.txt", "w");
    if (f) { fprintf(f, "%.3f %.3f\n", g_hornCx, g_hornCy); fclose(f); }
}
// rectangulo del boton "Calibrar claxon" bajo la rueda (misma geometria que el dibujo)
static bool InCalButton(float x, float y, float W, float H) {
    float cx = W*0.5f, cy = H*0.90f;
    return x >= cx - W*0.13f && x <= cx + W*0.13f && y >= cy - H*0.03f && y <= cy + H*0.03f;
}

static std::string g_vhudDir;   // /storage/emulated/0/vhud
static int g_current = 1;       // estacion actual conocida (1..13)
static int g_selected = 1;      // segmento bajo el dedo mientras la rueda esta abierta
static bool g_open = false;
static bool g_wheelTouch = false;   // v1.5: el toque actual empezo/sobre el anillo de la rueda
static bool g_ignoreNextUp = false; // v1.5: el toque que ABRIO la rueda no debe cerrarla al soltar

// v1.5: segmento bajo un punto de la pantalla (0 = fuera del anillo: centro o fuera de la rueda)
static int SegmentAt(float x, float y, float W, float H) {
    float dx = x - W*0.5f, dy = y - H*0.5f;
    float dist = sqrtf(dx*dx + dy*dy);
    if (dist < H*0.10f || dist > H*0.35f) return 0;
    float ang = atan2f(dy, dx) + 3.14159265f/2; if (ang < 0) ang += 2*3.14159265f;
    int seg = (int)(ang / (2*3.14159265f/13.0f)) + 1; if (seg > 13) seg = 13;
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
    int ok = 0;
    for (int i = 1; i <= 13; i++) {
        std::string p = g_vhudDir + "/radio/radio_" + std::to_string(i) + ".png";
        g_tex[i] = LoadPngToGl(p);
        if (g_tex[i]) ok++;
        Log("  %s radio_%d.png", g_tex[i] ? "ok    " : "FALTA ", i);
    }
    g_tex[0] = LoadPngToGl(g_vhudDir + "/radio/radio_current.png");
    Log("  %s radio_current.png", g_tex[0] ? "ok    " : "FALTA ");
    Log("iconos de radio cargados: %d/13 (+marco %s)", ok, g_tex[0] ? "si" : "no");
    if (ok == 0) Log("  !! sin iconos: copia vhud/radio/ a /storage/emulated/0/vhud/ (ver README)");
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
    if (!g_open) { g_open = true; g_ignoreNextUp = true; RefreshCurrent(); g_selected = g_current; Log("rueda abierta por %s (estacion actual %d %s)", why, g_current, kStationName[g_current]); }
    else { g_open = false; Log("rueda cerrada por %s", why); }
}

// v1.8: vuelca los primeros 128 bytes de un widget (para encontrar su bandera de "tocado")
static void DumpWidget(const char* tag, void* w, int n) {
    if (!w) { Log("%s #%d null", tag, n); return; }
    char hex[260]; const unsigned char* b = (const unsigned char*)w;
    for (int i = 0; i < 128; i++) { hex[i*2] = "0123456789abcdef"[b[i] >> 4]; hex[i*2+1] = "0123456789abcdef"[b[i] & 15]; }
    hex[256] = 0;
    Log("%s #%d %s", tag, n, hex);
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

static void DrawWheel(float W, float H) {
    // boton flotante (abajo a la derecha, encima de los controles del juego)
    float bs = H * 0.06f;  // v1.1: boton mas chico
    ImVec2 bp(W - bs * 1.15f, H * 0.62f);
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    if (g_tex[g_current]) dl->AddImage((ImTextureID)(uintptr_t)g_tex[g_current], bp, ImVec2(bp.x + bs, bp.y + bs));
    else { dl->AddCircleFilled(ImVec2(bp.x + bs/2, bp.y + bs/2), bs/2, IM_COL32(30,30,40,220)); }
    dl->AddCircle(ImVec2(bp.x + bs/2, bp.y + bs/2), bs/2, IM_COL32(255,255,255,200), 0, 2.0f);

    if (!g_open) return;
    // rueda: 13 segmentos alrededor del centro de la pantalla
    ImVec2 c(W * 0.5f, H * 0.5f);
    float R = H * 0.30f, rIn = R * 0.55f;  // v1.1: rueda mas chica
    dl->AddCircleFilled(c, R, IM_COL32(10,10,16,200));
    dl->AddCircle(c, R, IM_COL32(255,255,255,180), 0, 3.0f);
    for (int i = 1; i <= 13; i++) {
        float a0 = -3.14159265f/2 + (i-1) * 2*3.14159265f/13.0f;
        float a1 = a0 + 2*3.14159265f/13.0f;
        float am = (a0 + a1) * 0.5f;
        ImVec2 p(c.x + cosf(am) * (rIn + R) * 0.5f, c.y + sinf(am) * (rIn + R) * 0.5f);
        float s = R * 0.20f;  // v1.1: iconos mas chicos (~110px a 1080p)
        if (i == g_selected) { // segmento seleccionado: marco + fondo claro
            dl->AddCircleFilled(c, R, IM_COL32(120,80,200,60));
            if (g_tex[0]) dl->AddImage((ImTextureID)(uintptr_t)g_tex[0], ImVec2(p.x-s, p.y-s), ImVec2(p.x+s, p.y+s));
        }
        if (g_tex[i]) dl->AddImage((ImTextureID)(uintptr_t)g_tex[i], ImVec2(p.x-s*0.85f, p.y-s*0.85f), ImVec2(p.x+s*0.85f, p.y+s*0.85f));
        else dl->AddCircleFilled(p, s*0.5f, IM_COL32(255,255,255,120));
        if (i == g_current) dl->AddCircle(p, s, IM_COL32(80,255,120,255), 0, 3.0f);
    }
    // nombre de la estacion seleccionada, debajo de la rueda
    ImGui::SetNextWindowPos(ImVec2(c.x, c.y + R + H*0.03f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::Begin("##vhudradio-name", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    ImGui::Text("%s", kStationName[g_selected]);
    ImGui::End();
    // v1.6: boton "Calibrar claxon" (etiqueta visual; el toque se detecta en DrawFrame)
    ImGui::SetNextWindowPos(ImVec2(c.x, c.y + R + H*0.10f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin("##vhudradio-cal", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    ImGui::Text("[ Calibrar claxon ]");
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
            if (g_open) {
                int seg = SegmentAt(e.x, e.y, (float)w, (float)h);
                if (seg >= 1) { g_wheelTouch = true; if (seg != g_selected) { g_selected = seg; Log("rueda: segmento %d (%s)", seg, kStationName[seg]); } }
            }
        } else if (e.kind == 1) { io.AddMouseButtonEvent(0, true); mouseDown = true;
            // v1.6: modo calibracion - este toque ES el boton de claxon del juego
            if (g_calibrating && g_inVehicle) {
                float bs = (float)h * 0.06f; ImVec2 bp((float)w - bs*1.15f, (float)h*0.62f);
                if (io.MousePos.x >= bp.x && io.MousePos.x <= bp.x+bs && io.MousePos.y >= bp.y && io.MousePos.y <= bp.y+bs) {
                    g_calibrating = false; Log("calibracion de claxon cancelada (tocaste el boton flotante)");
                } else {
                    g_hornCx = io.MousePos.x / (float)w; g_hornCy = io.MousePos.y / (float)h;
                    SaveHornPos(); g_calibrating = false;
                    Log("claxon calibrado en (%.0f, %.0f) = (%.3f, %.3f) - guardado en horn_pos.txt", io.MousePos.x, io.MousePos.y, g_hornCx, g_hornCy);
                }
            }
            // v1.6: toque al boton "Calibrar claxon" bajo la rueda -> entrar en modo calibracion
            if (g_open && !g_calibrating && InCalButton(io.MousePos.x, io.MousePos.y, (float)w, (float)h)) {
                g_calibrating = true; g_open = false; g_wheelTouch = false;
                Log("modo calibracion de claxon: toca el boton del claxon del juego");
            }
            // v1.5: tocar un segmento lo elige al instante (antes solo el arrastre elegia)
            if (g_open) {
                int seg = SegmentAt(io.MousePos.x, io.MousePos.y, (float)w, (float)h);
                if (seg >= 1) { g_selected = seg; g_wheelTouch = true; Log("rueda: segmento %d (%s) [toque]", seg, kStationName[seg]); }
                else g_wheelTouch = false;  // toque fuera del anillo: al soltar se cierra sin cambiar
            }
            // v1.2: doble toque al boton de claxon (zona derecha) abre/cierra la rueda
            if (g_inVehicle) {
                static int tapLogN = 0;
                if (tapLogN < 30 && io.MousePos.x > (float)w*0.70f) { tapLogN++; Log("tap vehiculo #%d en (%.0f, %.0f)%s", tapLogN, io.MousePos.x, io.MousePos.y, InHornRegion(io.MousePos.x, io.MousePos.y, (float)w, (float)h) ? "  <- zona claxon" : ""); }
                if (InHornRegion(io.MousePos.x, io.MousePos.y, (float)w, (float)h)) {
                    static auto lastHornTap = std::chrono::steady_clock::now() - std::chrono::seconds(5);
                    auto now = std::chrono::steady_clock::now();
                    if (now - lastHornTap < std::chrono::milliseconds(500)) ToggleWheel("doble claxon");
                    lastHornTap = now;
                }
            }
            if (!g_open && g_inVehicle) {
                float bs = (float)h * 0.06f; ImVec2 bp((float)w - bs*1.15f, (float)h*0.62f);
                // el toque que abre se detecta por la posicion del ultimo evento 0 (io.MousePos)
                if (io.MousePos.x >= bp.x && io.MousePos.x <= bp.x+bs && io.MousePos.y >= bp.y && io.MousePos.y <= bp.y+bs) {
                    g_open = true; g_ignoreNextUp = true; g_selected = g_current; RefreshCurrent(); g_selected = g_current;
                    Log("rueda abierta (estacion actual %d %s)", g_current, kStationName[g_current]);
                }
            }
        } else if (e.kind == 2) { io.AddMouseButtonEvent(0, false); io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
            if (g_ignoreNextUp) { g_ignoreNextUp = false; mouseDown = false; }  // v1.5: el toque que abrio la rueda no la cierra
            else if (mouseDown && g_open) {
                if (g_wheelTouch) { ApplyStation(g_selected); Log("rueda cerrada -> %d (%s)", g_selected, kStationName[g_selected]); }
                else Log("rueda cerrada sin cambiar");
                g_open = false;
            }
            mouseDown = false; }
    }
    // v1.4: el juego actualiza la estacion leida con un poco de retraso tras StartRadio
    // (en el log v1.3 la lectura iba una estacion atrasada); re-leer cada ~0.5 s para
    // que el icono del boton flotante no se quede en la estacion anterior.
    if (g_inVehicle && !g_open) {
        static int frameN = 0;
        if (++frameN % 30 == 0) RefreshCurrent();
    }
    // v1.8: sonda del boton 7 - vuelca el widget 7 (claxon) y el 2 (acelerar,
    // como control) cada ~0.08 s, max 120 veces. Al pulsar el claxon, algun byte
    // del widget 7 tiene que cambiar: esa es su bandera de "tocado" y la v1.9
    // la leera directo, sin depender de la posicion del boton.
    if (g_widgets && g_inVehicle) {
        static int wFrame = 0, wN = 0;
        if (++wFrame % 5 == 0 && wN < 120) {
            wN++;
            DumpWidget("w7", g_widgets[kHornWidgetId], wN);
            DumpWidget("w2", g_widgets[2], wN);
        }
    }
    if (!g_inVehicle) g_open = false;  // fuera del coche la rueda se cierra sola
    // v1.6: aviso en pantalla mientras se calibra el claxon
    if (g_calibrating) {
        ImGui::SetNextWindowPos(ImVec2((float)w*0.5f, (float)h*0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::Begin("##vhudradio-calmsg", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
        ImGui::Text("Toca el boton del claxon del juego");
        ImGui::End();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ImGui_ImplOpenGL3_NewFrame(); ImGui::NewFrame();
    if (g_inVehicle) DrawWheel((float)w, (float)h);
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
    g_logPath = files + "/VHudRadioLog.txt";
    { FILE* f = fopen(g_logPath.c_str(), "w"); if (f) { fprintf(f, "# VHudRadioLog - VHud Radio Wheel %s\n", VHUDRADIO_VERSION); fclose(f); } }
    g_vhudDir = "/storage/emulated/0/vhud";
    mkdir(g_vhudDir.c_str(), 0777); mkdir((g_vhudDir + "/radio").c_str(), 0777);
    LoadHornPos();
    Log("  centro del claxon: (%.3f, %.3f)", g_hornCx, g_hornCy);
    Log("== VHud Radio Wheel %s cargado ==", VHUDRADIO_VERSION);
    Log("carpeta de iconos: %s/radio/ (radio_1.png ... radio_13.png)", g_vhudDir.c_str());

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
