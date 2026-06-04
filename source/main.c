#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "vars.h"
#include "ml_screen.h"
#include "ml_canvas.h"
#include "ml_color.h"
#include "ml_controls.h"
#include "ml_system.h"
#include "ml_image.h"
#include "ml_constants.h"

C3D_RenderTarget *topTarget;
C3D_RenderTarget *bottomTarget;
int SCREEN_UP_DISPLAY   = 1;
int SCREEN_DOWN_DISPLAY = 0;

// ---- Blad krytyczny PRZED inicjalizacja C2D --------------------------------
// consoleInit uzyty tylko tutaj — app i tak sie konczy, wiec nie ma konfliktu
// z C2D (ktore nie zdazylo sie uruchomic).

static void fatalError(const char *msg) {
    PrintConsole con;
    consoleInit(GFX_TOP, &con);
    printf("\n*** BLAD KRYTYCZNY ***\n%s\n\nSTART = wyjscie\n", msg);
    while (aptMainLoop()) {
        hidScanInput();
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
        if (hidKeysDown() & KEY_START) break;
    }
}

// ---- Bezpieczna rejestracja modulu Lua ------------------------------------

static void safeRegisterModule(lua_State *L, lua_CFunction opener) {
    int top_before = lua_gettop(L);
    opener(L);
    int pushed = lua_gettop(L) - top_before;
    if (pushed > 0) lua_pop(L, pushed);
}

void ml_setup_state(lua_State *L) {
    luaL_openlibs(L);
    ml_pushConstants(L);
    safeRegisterModule(L, luaopen_screen);
    safeRegisterModule(L, luaopen_canvas);
    safeRegisterModule(L, luaopen_color);
    safeRegisterModule(L, luaopen_controls);
    safeRegisterModule(L, luaopen_system);
    safeRegisterModule(L, luaopen_image);
}

// ---------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    gfxInitDefault();
    romfsInit();

    // Citro3D / Citro2D
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        fatalError("C3D_Init failed");
        romfsExit();
        gfxExit();
        return 1;
    }

    if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) {
        fatalError("C2D_Init failed");
        C3D_Fini();
        romfsExit();
        gfxExit();
        return 1;
    }
    C2D_Prepare();

    // Oba targety od razu — bez konsoli printf na zadnym ekranie
    topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    if (!topTarget) {
        fatalError("C2D_CreateScreenTarget(TOP) failed");
        goto cleanup_c2d;
    }

    bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (!bottomTarget) {
        fatalError("C2D_CreateScreenTarget(BOTTOM) failed");
        goto cleanup_c2d;
    }

    // Lua state
    lua_State *L = luaL_newstate();
    if (!L) {
        fatalError("luaL_newstate() failed");
        goto cleanup_c2d;
    }

    ml_setup_state(L);
    ml_screen_init_resources();

    // Launcher z romfs
    if (luaL_loadfile(L, "romfs:/lua/boot.lua") != 0) {
        fatalError(lua_tostring(L, -1));
        lua_pop(L, 1);
        goto cleanup_lua;
    }

    if (lua_pcall(L, 0, 0, 0) != 0) {
        const char *err = lua_tostring(L, -1);
        // Pokaz blad przez C2D (consoleInit tu crashuje — C2D juz dziala)
        C2D_TextBuf errBuf = C2D_TextBufNew(512);
        C2D_Text errText;
        C2D_TextParse(&errText, errBuf, err ? err : "unknown error");
        C2D_TextOptimize(&errText);
        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & KEY_START) break;
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            C2D_TargetClear(topTarget,    C2D_Color32(0x10, 0x00, 0x00, 0xFF));
            C2D_TargetClear(bottomTarget, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
            C2D_SceneBegin(topTarget);
            C2D_DrawText(&errText, C2D_WithColor, 4, 4, 0.5f, 0.45f, 0.45f,
                         C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
            C2D_SceneBegin(bottomTarget);
            C3D_FrameEnd(0);
        }
        C2D_TextBufDelete(errBuf);
        lua_pop(L, 1);
    }

cleanup_lua:
    ml_screen_free_resources();
    lua_close(L);

cleanup_c2d:
    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();
    return 0;
}
