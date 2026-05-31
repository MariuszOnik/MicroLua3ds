#pragma once
#include <lua.h>

#define LUA_SCREENLIBNAME "screen"

void ml_screen_init_resources(void);
void ml_screen_free_resources(void);

int luaopen_screen(lua_State *L);
