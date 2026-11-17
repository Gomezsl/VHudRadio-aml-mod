// VHudRadio.cpp - Rueda de radio estilo GTA V para GTA SA Android 2.10 (AML .so), v1.1 1.1.0-beta.
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
// las variantes de PC dieron FALTA (como se esperaba). Orden de cambio: RetuneRadio
// (AudioEngine, luego TrackManager) -> StartRadio AudioEngine -> StartRadio TrackManager
// (Eaafh, el que usa el mod GTA6Radio 4.0 en 2.10 para cambiar de estacion).
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

MYMODCFG(net.hunter09.vhudradio, VHud Radio Wheel, 1.1.0-beta, Hunter09)
NEEDGAME(com.rockstargames.gtasa)
#define VHUDRADIO_VERSION "1.1.0-beta"

// ---- log: VHudRadioLog.txt junto al juego ----
static std::string g_logPath;
static void Log(const char* fmt, ...) {
    char buf[600]; va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
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
typedef signed char (*RetuneFn)(signed char);  // en la practica devuelve void/char; solo se llama si resolvio
static RetuneFn g_retuneAudio = nullptr, g_retuneTrack = nullptr;
typedef signed char (*GetStationFn)();
static GetStationFn g_getStationAudio = nullptr, g_getStationTrack = nullptr;
typedef void (*StartRadioAudioFn)(signed char, signed char);
static StartRadioAudioFn g_startAudio = nullptr;
typedef void (*StartRadioTrackFn)(signed char, signed char, float, unsigned char);
static StartRadioTrackFn g_startTrack = nullptr;  // el que usa el mod GTA6Radio 4.0 en 2.10 (verificado por Probe: ok)

// ---- claxon (v1.1): doble toque al boton de claxon del juego abre/cierra la rueda ----
// CPad::GetPad(0) = pad del jugador; CPad::GetHorn() = true mientras suena el claxon.
// Se sondea cada frame en DrawFrame; si GetHorn no resuelve, solo queda el boton flotante.
typedef void* (*GetPadFn)(int);
static GetPadFn g_getPad = nullptr;
typedef bool (*GetHornFn)(void*);
static GetHornFn g_getHorn = nullptr;

static std::string g_vhudDir;   // /storage/emulated/0/vhud
static int g_current = 1;       // estacion actual conocida (1..13)
static int g_selected = 1;      // segmento bajo el dedo mientras la rueda esta abierta
static bool g_open = false;
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
    if (g_retuneAudio) { Log("cambio a %d (%s) por CAudioEngine::RetuneRadio", id, kStationName[id]); g_retuneAudio((signed char)id); g_current = id; return; }
    if (g_retuneTrack) { Log("cambio a %d (%s) por CAERadioTrackManager::RetuneRadio", id, kStationName[id]); g_retuneTrack((signed char)id); g_current = id; return; }
    if (g_startAudio) { Log("cambio a %d (%s) por CAudioEngine::StartRadio(id,0) (fallback)", id, kStationName[id]); g_startAudio((signed char)id, 0); g_current = id; return; }
    if (g_startTrack) { Log("cambio a %d (%s) por CAERadioTrackManager::StartRadio(id,0,1.0,0) (fallback, el que usa GTA6Radio 4.0)", id, kStationName[id]); g_startTrack((signed char)id, 0, 1.0f, 0); g_current = id; return; }
    Log("!! sin RetuneRadio ni StartRadio: la rueda se ve pero NO puede cambiar la estacion (ver VHudRadioProbe.txt)");
}
static void RefreshCurrent() {
    if (g_getStationAudio) { int v = (int)g_getStationAudio(); if (v >= 1 && v <= 13) g_current = v; return; }
    if (g_getStationTrack) { int v = (int)g_getStationTrack(); if (v >= 1 && v <= 13) g_current = v; }
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
    // doble toque al claxon: abre/cierra la rueda (solo dentro del vehiculo)
    if (g_getPad && g_getHorn && g_inVehicle) {
        static bool prevHorn = false;
        static auto lastTap = std::chrono::steady_clock::now() - std::chrono::seconds(5);
        void* pad = g_getPad(0);
        bool horn = pad ? g_getHorn(pad) : false;
        if (horn && !prevHorn) {
            auto now = std::chrono::steady_clock::now();
            if (now - lastTap < std::chrono::milliseconds(500)) {
                if (!g_open) { g_open = true; RefreshCurrent(); g_selected = g_current; Log("rueda abierta por doble claxon (estacion actual %d %s)", g_current, kStationName[g_current]); }
                else { g_open = false; Log("rueda cerrada por doble claxon"); }
            }
            lastTap = now;
        }
        prevHorn = horn;
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
                float dx = e.x - (float)w*0.5f, dy = e.y - (float)h*0.5f;
                float dist = sqrtf(dx*dx + dy*dy);
                if (dist > (float)h*0.10f) { // fuera del centro: el angulo elige el segmento
                    float ang = atan2f(dy, dx) + 3.14159265f/2; if (ang < 0) ang += 2*3.14159265f;
                    int seg = (int)(ang / (2*3.14159265f/13.0f)) + 1; if (seg > 13) seg = 13;
                    if (seg != g_selected) { g_selected = seg; Log("rueda: segmento %d (%s)", seg, kStationName[seg]); }
                }
            }
        } else if (e.kind == 1) { io.AddMouseButtonEvent(0, true); mouseDown = true;
            if (!g_open && g_inVehicle) {
                float bs = (float)h * 0.06f; ImVec2 bp((float)w - bs*1.15f, (float)h*0.62f);
                // el toque que abre se detecta por la posicion del ultimo evento 0 (io.MousePos)
                if (io.MousePos.x >= bp.x && io.MousePos.x <= bp.x+bs && io.MousePos.y >= bp.y && io.MousePos.y <= bp.y+bs) {
                    g_open = true; g_selected = g_current; RefreshCurrent(); g_selected = g_current;
                    Log("rueda abierta (estacion actual %d %s)", g_current, kStationName[g_current]);
                }
            }
        } else if (e.kind == 2) { io.AddMouseButtonEvent(0, false); io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
            if (mouseDown && g_open) { ApplyStation(g_selected); Log("rueda cerrada -> %d (%s)", g_selected, kStationName[g_selected]); g_open = false; }
            mouseDown = false; }
    }
    if (!g_inVehicle) g_open = false;  // fuera del coche la rueda se cierra sola
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
    SYM(g_getPad, "_ZN4CPad6GetPadEi");
    SYM(g_getHorn, "_ZN4CPad7GetHornEv");
    #undef SYM
    if (!g_getPad || !g_getHorn) Log("  !! doble claxon no disponible (CPad::GetPad/GetHorn FALTA): solo boton flotante");
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
