#include <stdlib.h>
#define LUA_LIB
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <3ds.h>
#include "ml_controls.h"

// Controls.read()
// Sets globals Keys and Stylus, matching MicroLua NDS API.
// New 3DS buttons (ZL, ZR, C-Stick) are added as extras.
static int controls_read(lua_State *L) {
    hidScanInput();
    u32 held     = hidKeysHeld();
    u32 pressed  = hidKeysDown();
    u32 released = hidKeysUp();

    touchPosition touch;
    hidTouchRead(&touch);

    // --- Stylus table ---
    lua_newtable(L);
    lua_pushnumber(L, touch.px);  lua_setfield(L, -2, "X");
    lua_pushnumber(L, touch.py);  lua_setfield(L, -2, "Y");
    lua_pushboolean(L, (held & KEY_TOUCH) != 0);     lua_setfield(L, -2, "held");
    lua_pushboolean(L, (released & KEY_TOUCH) != 0); lua_setfield(L, -2, "released");
    lua_pushboolean(L, (pressed  & KEY_TOUCH) != 0); lua_setfield(L, -2, "newPress");
    lua_pushboolean(L, 0);         lua_setfield(L, -2, "doubleClick");
    lua_pushinteger(L, 0);         lua_setfield(L, -2, "deltaX");
    lua_pushinteger(L, 0);         lua_setfield(L, -2, "deltaY");
    lua_setglobal(L, "Stylus");

    // --- Keys table (held / released / newPress sub-tables) ---
    lua_newtable(L);

#define PUSH_KEY_TABLE(mask_var) \
    lua_newtable(L); \
    lua_pushboolean(L, ((mask_var) & KEY_A)      != 0); lua_setfield(L, -2, "A"); \
    lua_pushboolean(L, ((mask_var) & KEY_B)      != 0); lua_setfield(L, -2, "B"); \
    lua_pushboolean(L, ((mask_var) & KEY_X)      != 0); lua_setfield(L, -2, "X"); \
    lua_pushboolean(L, ((mask_var) & KEY_Y)      != 0); lua_setfield(L, -2, "Y"); \
    lua_pushboolean(L, ((mask_var) & KEY_L)      != 0); lua_setfield(L, -2, "L"); \
    lua_pushboolean(L, ((mask_var) & KEY_R)      != 0); lua_setfield(L, -2, "R"); \
    lua_pushboolean(L, ((mask_var) & KEY_ZL)     != 0); lua_setfield(L, -2, "ZL"); \
    lua_pushboolean(L, ((mask_var) & KEY_ZR)     != 0); lua_setfield(L, -2, "ZR"); \
    lua_pushboolean(L, ((mask_var) & KEY_START)  != 0); lua_setfield(L, -2, "Start"); \
    lua_pushboolean(L, ((mask_var) & KEY_SELECT) != 0); lua_setfield(L, -2, "Select"); \
    lua_pushboolean(L, ((mask_var) & KEY_DUP)    != 0); lua_setfield(L, -2, "Up"); \
    lua_pushboolean(L, ((mask_var) & KEY_DDOWN)  != 0); lua_setfield(L, -2, "Down"); \
    lua_pushboolean(L, ((mask_var) & KEY_DLEFT)  != 0); lua_setfield(L, -2, "Left"); \
    lua_pushboolean(L, ((mask_var) & KEY_DRIGHT) != 0); lua_setfield(L, -2, "Right");

    PUSH_KEY_TABLE(held)     lua_setfield(L, -2, "held");
    PUSH_KEY_TABLE(released) lua_setfield(L, -2, "released");
    PUSH_KEY_TABLE(pressed)  lua_setfield(L, -2, "newPress");

#undef PUSH_KEY_TABLE

    lua_setglobal(L, "Keys");
    return 0;
}

// Controls.isRunning() — replaces aptMainLoop() in the game loop
static int controls_isRunning(lua_State *L) {
    lua_pushboolean(L, aptMainLoop());
    return 1;
}

static const luaL_Reg controlslib[] = {
    {"read",      controls_read},
    {"isRunning", controls_isRunning},
    {NULL, NULL}
};

LUALIB_API int luaopen_controls(lua_State *L) {
    luaL_register(L, LUA_CONTROLSLIBNAME, controlslib);
    return 1;
}
