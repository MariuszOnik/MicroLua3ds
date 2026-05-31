#include <lua.h>
#include <lauxlib.h>
#include <3ds.h>
#include "vars.h"
#include "ml_constants.h"

// Push all MicroLua-compatible global constants into the Lua state
void ml_pushConstants(lua_State *L) {
    // Screen identifiers (same values as MicroLua NDS)
    lua_pushnumber(L, 1); lua_setglobal(L, "SCREEN_UP");
    lua_pushnumber(L, 0); lua_setglobal(L, "SCREEN_DOWN");

    // 3DS screen dimensions (top screen)
    lua_pushnumber(L, 400); lua_setglobal(L, "SCREEN_WIDTH");
    lua_pushnumber(L, 240); lua_setglobal(L, "SCREEN_HEIGHT");
    // Bottom screen
    lua_pushnumber(L, 320); lua_setglobal(L, "SCREEN_WIDTH_DOWN");
    lua_pushnumber(L, 240); lua_setglobal(L, "SCREEN_HEIGHT_DOWN");

    // Canvas attribute IDs (same as MicroLua NDS)
    lua_pushnumber(L, ATTR_X1);      lua_setglobal(L, "ATTR_X1");
    lua_pushnumber(L, ATTR_Y1);      lua_setglobal(L, "ATTR_Y1");
    lua_pushnumber(L, ATTR_X2);      lua_setglobal(L, "ATTR_X2");
    lua_pushnumber(L, ATTR_Y2);      lua_setglobal(L, "ATTR_Y2");
    lua_pushnumber(L, ATTR_X3);      lua_setglobal(L, "ATTR_X3");
    lua_pushnumber(L, ATTR_Y3);      lua_setglobal(L, "ATTR_Y3");
    lua_pushnumber(L, ATTR_COLOR);   lua_setglobal(L, "ATTR_COLOR");
    lua_pushnumber(L, ATTR_COLOR1);  lua_setglobal(L, "ATTR_COLOR1");
    lua_pushnumber(L, ATTR_COLOR2);  lua_setglobal(L, "ATTR_COLOR2");
    lua_pushnumber(L, ATTR_COLOR3);  lua_setglobal(L, "ATTR_COLOR3");
    lua_pushnumber(L, ATTR_COLOR4);  lua_setglobal(L, "ATTR_COLOR4");
    lua_pushnumber(L, ATTR_TEXT);    lua_setglobal(L, "ATTR_TEXT");
    lua_pushnumber(L, ATTR_VISIBLE); lua_setglobal(L, "ATTR_VISIBLE");
    lua_pushnumber(L, ATTR_FONT);    lua_setglobal(L, "ATTR_FONT");
    lua_pushnumber(L, ATTR_IMAGE);   lua_setglobal(L, "ATTR_IMAGE");

    // MicroLua version string
    lua_pushstring(L, "3DS-4.7"); lua_setglobal(L, "MICROLUA_VERSION");
}
