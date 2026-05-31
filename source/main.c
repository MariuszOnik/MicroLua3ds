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
#include "ml_constants.h"

// Global render targets – declared extern in vars.h
C3D_RenderTarget *topTarget;
C3D_RenderTarget *bottomTarget;
int SCREEN_UP_DISPLAY   = 1;
int SCREEN_DOWN_DISPLAY = 0;

// Show a fatal error on the bottom console and wait for START
static void showError(const char *msg) {
    consoleInit(GFX_BOTTOM, NULL);
    printf("\n[MicroLua3DS] Error:\n%s\n\nPress START to exit.\n", msg);
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;
        gspWaitForVBlank();
    }
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    // --- Hardware init ---
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    romfsInit();

    topTarget    = C2D_CreateScreenTarget(GFX_TOP,    GFX_LEFT);
    bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // --- Lua state ---
    lua_State *L = luaL_newstate();
    if (!L) { showError("luaL_newstate() failed"); goto cleanup; }

    luaL_openlibs(L);

    // Push global constants before registering modules
    ml_pushConstants(L);

    // Register modules as globals (luaL_register in Lua 5.1 sets the global automatically)
    luaopen_screen(L);   lua_pop(L, 1);
    luaopen_canvas(L);   lua_pop(L, 1);
    luaopen_color(L);    lua_pop(L, 1);
    luaopen_controls(L); lua_pop(L, 1);
    luaopen_system(L);   lua_pop(L, 1);

    // Init screen text resources
    ml_screen_init_resources();

    // --- Load and run boot.lua from romfs ---
    const char *boot = "romfs:/lua/boot.lua";
    if (luaL_loadfile(L, boot) != 0) {
        showError(lua_tostring(L, -1));
        goto cleanup_lua;
    }
    if (lua_pcall(L, 0, 0, 0) != 0) {
        showError(lua_tostring(L, -1));
        lua_pop(L, 1);
    }

cleanup_lua:
    ml_screen_free_resources();
    lua_close(L);

cleanup:
    romfsExit();
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
