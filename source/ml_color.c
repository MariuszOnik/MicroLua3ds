#include <stdlib.h>
#include <math.h>
#define LUA_LIB
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <citro2d.h>
#include "vars.h"
#include "ml_color.h"

// Color.new(r, g, b)  -- r/g/b in 0..31, same API as MicroLua NDS
// Returns a u32 RGBA8888 value understood by citro2d
static int color_new(lua_State *L) {
    int r = (int)luaL_checknumber(L, 1);
    int g = (int)luaL_checknumber(L, 2);
    int b = (int)luaL_checknumber(L, 3);
    ml_assert(L, r >= 0 && r <= 31, "Red must be 0..31");
    ml_assert(L, g >= 0 && g <= 31, "Green must be 0..31");
    ml_assert(L, b >= 0 && b <= 31, "Blue must be 0..31");
    // Convert 5-bit (0-31) → 8-bit (0-255)
    lua_pushnumber(L, (lua_Number)(u32)C2D_Color32(r * 255 / 31, g * 255 / 31, b * 255 / 31, 255));
    return 1;
}

// Color.new256(r, g, b)  -- r/g/b in 0..255
static int color_new256(lua_State *L) {
    int r = (int)luaL_checknumber(L, 1);
    int g = (int)luaL_checknumber(L, 2);
    int b = (int)luaL_checknumber(L, 3);
    ml_assert(L, r >= 0 && r <= 255, "Red must be 0..255");
    ml_assert(L, g >= 0 && g <= 255, "Green must be 0..255");
    ml_assert(L, b >= 0 && b <= 255, "Blue must be 0..255");
    lua_pushnumber(L, (lua_Number)(u32)C2D_Color32(r, g, b, 255));
    return 1;
}

static const luaL_Reg colorlib[] = {
    {"new",    color_new},
    {"new256", color_new256},
    {NULL, NULL}
};

LUALIB_API int luaopen_color(lua_State *L) {
    luaL_register(L, LUA_COLORLIBNAME, colorlib);
    return 1;
}
