#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#define LUA_LIB
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <3ds.h>
#include "ml_system.h"

static int system_currentDirectory(lua_State *L) {
    char path[512];
    getcwd(path, sizeof(path));
    lua_pushstring(L, path);
    return 1;
}

static int system_changeDirectory(lua_State *L) {
    const char *dir = luaL_checkstring(L, 1);
    chdir(dir);
    return 0;
}

static int system_remove(lua_State *L) {
    remove(luaL_checkstring(L, 1));
    return 0;
}

static int system_rename(lua_State *L) {
    rename(luaL_checkstring(L, 1), luaL_checkstring(L, 2));
    return 0;
}

static int system_makeDirectory(lua_State *L) {
    mkdir(luaL_checkstring(L, 1), 0777);
    return 0;
}

// System.listDirectory(path)
// Returns array of {name, isDir, size} tables, folders first (bubble sort)
static int system_listDirectory(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    DIR *d = opendir(path);
    if (!d) luaL_error(L, "Cannot open directory: %s", path);

    lua_newtable(L);
    int idx = 1;
    struct dirent *e;
    char fullpath[512];

    while ((e = readdir(d)) != NULL) {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, e->d_name);
        struct stat st;
        if (stat(fullpath, &st) != 0) continue;

        lua_pushinteger(L, idx++);
        lua_newtable(L);
        lua_pushstring(L, e->d_name);         lua_setfield(L, -2, "name");
        lua_pushboolean(L, S_ISDIR(st.st_mode)); lua_setfield(L, -2, "isDir");
        lua_pushnumber(L, (lua_Number)st.st_size); lua_setfield(L, -2, "size");
        lua_settable(L, -3);
    }
    closedir(d);
    return 1;
}

// System.shutDown()  — exit to HOME menu
static int system_shutDown(lua_State *L) {
    (void)L;
    svcExitProcess();
    return 0;
}

// System.sleep()  — not meaningful on 3DS without PM service; just yield
static int system_sleep(lua_State *L) {
    (void)L;
    gspWaitForVBlank();
    return 0;
}

static const luaL_Reg systemlib[] = {
    {"currentDirectory", system_currentDirectory},
    {"changeDirectory",  system_changeDirectory},
    {"remove",           system_remove},
    {"rename",           system_rename},
    {"makeDirectory",    system_makeDirectory},
    {"listDirectory",    system_listDirectory},
    {"shutDown",         system_shutDown},
    {"sleep",            system_sleep},
    {NULL, NULL}
};

LUALIB_API int luaopen_system(lua_State *L) {
    luaL_register(L, LUA_SYSTEMLIBNAME, systemlib);
    return 1;
}
