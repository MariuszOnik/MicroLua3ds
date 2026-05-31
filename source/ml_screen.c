#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#define LUA_LIB
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <citro2d.h>
#include "vars.h"
#include "ml_screen.h"

// Global text buffer, reset each frame in screen.startDrawing2D
static C2D_TextBuf s_textBuf;
// Tracks which target is currently active to avoid redundant SceneBegin calls
static C3D_RenderTarget* s_currentTarget = NULL;
// Per-frame global alpha (0-255), applied to all draw calls
static u8 s_alpha = 255;

static const float TEXT_SCALE = 0.5f;  // ~NDS text size on 3DS screen
static const float DRAW_DEPTH = 0.5f;

// ---- internal helpers ---------------------------------------------------

static void switchTarget(int screen) {
    C3D_RenderTarget* t = ml_getTarget(screen);
    if (t != s_currentTarget) {
        C2D_SceneBegin(t);
        s_currentTarget = t;
    }
}

// Draw a 1-pixel-wide line using two triangles
static void drawLinePrimitive(float x0, float y0, float x1, float y1, u32 color) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.5f) {
        C2D_DrawRectSolid(x0, y0, DRAW_DEPTH, 1.0f, 1.0f, color);
        return;
    }
    float nx = -dy / len * 0.5f;
    float ny =  dx / len * 0.5f;
    // quad as two triangles
    C2D_DrawTriangle(x0 + nx, y0 + ny, color,
                     x0 - nx, y0 - ny, color,
                     x1 + nx, y1 + ny, color, DRAW_DEPTH);
    C2D_DrawTriangle(x0 - nx, y0 - ny, color,
                     x1 - nx, y1 - ny, color,
                     x1 + nx, y1 + ny, color, DRAW_DEPTH);
}

static u32 withAlpha(u32 color) {
    if (s_alpha == 255) return color;
    u8 a = (u8)(((color >> 24) & 0xFF) * s_alpha / 255);
    return (color & 0x00FFFFFF) | ((u32)a << 24);
}

// ---- resource management ------------------------------------------------

void ml_screen_init_resources(void) {
    s_textBuf = C2D_TextBufNew(4096);
}

void ml_screen_free_resources(void) {
    C2D_TextBufDelete(s_textBuf);
}

// ---- Lua bindings -------------------------------------------------------

// screen.init()  — reset logical display assignment
static int screen_init(lua_State *L) {
    (void)L;
    SCREEN_UP_DISPLAY   = 1;
    SCREEN_DOWN_DISPLAY = 0;
    return 0;
}

// screen.switch()  — swap which logical screen is "up"
static int screen_switch(lua_State *L) {
    (void)L;
    int tmp = SCREEN_UP_DISPLAY;
    SCREEN_UP_DISPLAY   = SCREEN_DOWN_DISPLAY;
    SCREEN_DOWN_DISPLAY = tmp;
    return 0;
}

// screen.startDrawing2D()  — begin frame, clear both screens
static int screen_startDrawing2D(lua_State *L) {
    (void)L;
    C2D_TextBufClear(s_textBuf);
    s_currentTarget = NULL;
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(topTarget,    C2D_Color32(0, 0, 0, 255));
    C2D_TargetClear(bottomTarget, C2D_Color32(0, 0, 0, 255));
    return 0;
}

// screen.endDrawing()
static int screen_endDrawing(lua_State *L) {
    (void)L;
    C3D_FrameEnd(0);
    s_currentTarget = NULL;
    return 0;
}

// screen.waitForVBL()  — no-op: C3D_FRAME_SYNCDRAW already waits for VBlank
static int screen_waitForVBL(lua_State *L) {
    (void)L;
    return 0;
}

// screen.getMainLcd()  — on 3DS top screen is always "main"
static int screen_getMainLcd(lua_State *L) {
    lua_pushboolean(L, (SCREEN_UP_DISPLAY == 1));
    return 1;
}

// screen.setAlpha(level [,group])  — level 0..100 (100 = opaque)
// group parameter is ignored on 3DS (no hardware blending groups)
static int screen_setAlpha(lua_State *L) {
    int level = (int)luaL_checknumber(L, 1);
    if (level >= 100)
        s_alpha = 255;
    else
        s_alpha = (u8)(level * 255 / 100);
    return 0;
}

static int screen_getAlphaLevel(lua_State *L) {
    lua_pushnumber(L, s_alpha * 100 / 255);
    return 1;
}

static int screen_getLayer(lua_State *L) {
    lua_pushnumber(L, 1);  // always 1, no layer groups on 3DS
    return 1;
}

// screen.print(screen, x, y, text [,color])
static int screen_print(lua_State *L) {
    int scr    = (int)luaL_checknumber(L, 1);
    float x    = (float)luaL_checknumber(L, 2);
    float y    = (float)luaL_checknumber(L, 3);
    const char *text = luaL_checkstring(L, 4);
    u32 color  = lua_isnumber(L, 5)
                 ? (u32)luaL_checknumber(L, 5)
                 : C2D_Color32(255, 255, 255, 255);

    switchTarget(scr);
    C2D_Text t;
    C2D_TextParse(&t, s_textBuf, text);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor | C2D_AtBaseline, x, y, DRAW_DEPTH,
                 TEXT_SCALE, TEXT_SCALE, withAlpha(color));
    return 0;
}

// screen.printFont(screen, x, y, text [,color, font])
// font parameter ignored — use same system text
static int screen_printFont(lua_State *L) {
    int scr    = (int)luaL_checknumber(L, 1);
    float x    = (float)luaL_checknumber(L, 2);
    float y    = (float)luaL_checknumber(L, 3);
    const char *text = luaL_checkstring(L, 4);
    u32 color  = lua_isnumber(L, 5)
                 ? (u32)luaL_checknumber(L, 5)
                 : C2D_Color32(255, 255, 255, 255);

    switchTarget(scr);
    C2D_Text t;
    C2D_TextParse(&t, s_textBuf, text);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor | C2D_AtBaseline, x, y, DRAW_DEPTH,
                 TEXT_SCALE, TEXT_SCALE, withAlpha(color));
    return 0;
}

// screen.drawLine(screen, x0, y0, x1, y1, color)
static int screen_drawLine(lua_State *L) {
    int scr = (int)luaL_checknumber(L, 1);
    float x0 = (float)luaL_checknumber(L, 2);
    float y0 = (float)luaL_checknumber(L, 3);
    float x1 = (float)luaL_checknumber(L, 4);
    float y1 = (float)luaL_checknumber(L, 5);
    u32 color = (u32)luaL_checknumber(L, 6);
    switchTarget(scr);
    drawLinePrimitive(x0, y0, x1, y1, withAlpha(color));
    return 0;
}

// screen.drawPoint(screen, x, y, color)
static int screen_drawPoint(lua_State *L) {
    int scr = (int)luaL_checknumber(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    u32 color = (u32)luaL_checknumber(L, 4);
    switchTarget(scr);
    C2D_DrawRectSolid(x, y, DRAW_DEPTH, 1.0f, 1.0f, withAlpha(color));
    return 0;
}

// screen.drawRect(screen, x0, y0, x1, y1, color)  — unfilled rectangle
static int screen_drawRect(lua_State *L) {
    int scr = (int)luaL_checknumber(L, 1);
    float x0 = (float)luaL_checknumber(L, 2);
    float y0 = (float)luaL_checknumber(L, 3);
    float x1 = (float)luaL_checknumber(L, 4);
    float y1 = (float)luaL_checknumber(L, 5);
    u32 color = withAlpha((u32)luaL_checknumber(L, 6));
    float w = x1 - x0, h = y1 - y0;
    switchTarget(scr);
    C2D_DrawRectSolid(x0,      y0,      DRAW_DEPTH, w, 1.0f, color);  // top
    C2D_DrawRectSolid(x0,      y1 - 1, DRAW_DEPTH, w, 1.0f, color);  // bottom
    C2D_DrawRectSolid(x0,      y0,      DRAW_DEPTH, 1.0f, h, color);  // left
    C2D_DrawRectSolid(x1 - 1, y0,      DRAW_DEPTH, 1.0f, h, color);  // right
    return 0;
}

// screen.drawFillRect(screen, x0, y0, x1, y1, color)
static int screen_drawFillRect(lua_State *L) {
    int scr = (int)luaL_checknumber(L, 1);
    float x0 = (float)luaL_checknumber(L, 2);
    float y0 = (float)luaL_checknumber(L, 3);
    float x1 = (float)luaL_checknumber(L, 4);
    float y1 = (float)luaL_checknumber(L, 5);
    u32 color = (u32)luaL_checknumber(L, 6);
    switchTarget(scr);
    C2D_DrawRectSolid(x0, y0, DRAW_DEPTH, x1 - x0, y1 - y0, withAlpha(color));
    return 0;
}

// screen.drawGradientRect(screen, x0,y0,x1,y1, color1,color2,color3,color4)
// MicroLua corner order: top-left, bottom-left, bottom-right, top-right
// citro2d order:         top-left, top-right, bottom-left, bottom-right
static int screen_drawGradientRect(lua_State *L) {
    int scr = (int)luaL_checknumber(L, 1);
    float x0 = (float)luaL_checknumber(L, 2);
    float y0 = (float)luaL_checknumber(L, 3);
    float x1 = (float)luaL_checknumber(L, 4);
    float y1 = (float)luaL_checknumber(L, 5);
    u32 c_tl = withAlpha((u32)luaL_checknumber(L, 6));  // top-left
    u32 c_bl = withAlpha((u32)luaL_checknumber(L, 7));  // bottom-left
    u32 c_br = withAlpha((u32)luaL_checknumber(L, 8));  // bottom-right
    u32 c_tr = withAlpha((u32)luaL_checknumber(L, 9));  // top-right
    switchTarget(scr);
    C2D_DrawRectangle(x0, y0, DRAW_DEPTH, x1 - x0, y1 - y0,
                      c_tl, c_tr, c_bl, c_br);
    return 0;
}

// screen.drawTextBox(screen, x0,y0,x1,y1, text [,color])
// Simple single-line clip; word-wrap is not provided by citro2d natively
static int screen_drawTextBox(lua_State *L) {
    int scr = (int)luaL_checknumber(L, 1);
    float x0 = (float)luaL_checknumber(L, 2);
    float y0 = (float)luaL_checknumber(L, 3);
    const char *text = luaL_checkstring(L, 6);
    u32 color = lua_isnumber(L, 7)
                ? (u32)luaL_checknumber(L, 7)
                : C2D_Color32(255, 255, 255, 255);
    switchTarget(scr);
    C2D_Text t;
    C2D_TextParse(&t, s_textBuf, text);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor | C2D_AtBaseline, x0, y0, DRAW_DEPTH,
                 TEXT_SCALE, TEXT_SCALE, withAlpha(color));
    return 0;
}

// screen.blit(screen, x, y, img [,sx,sy,w,h])
// img must be a C2D_Image* pushed as lightuserdata by the image module
static int screen_blit(lua_State *L) {
    int scr  = (int)luaL_checknumber(L, 1);
    float x  = (float)luaL_checknumber(L, 2);
    float y  = (float)luaL_checknumber(L, 3);
    C2D_Image *img = (C2D_Image *)lua_touserdata(L, 4);
    ml_assert(L, img != NULL, "Bad image resource");
    switchTarget(scr);

    if (lua_isnumber(L, 8)) {
        // Sub-region blit via a custom Tex3DS_SubTexture
        float sx = (float)luaL_checknumber(L, 5);
        float sy = (float)luaL_checknumber(L, 6);
        float sw = (float)luaL_checknumber(L, 7);
        float sh = (float)luaL_checknumber(L, 8);
        float tw = (float)img->tex->width;
        float th = (float)img->tex->height;
        Tex3DS_SubTexture sub = {
            (u16)sw, (u16)sh,
            sx / tw,          1.0f - sy / th,
            (sx + sw) / tw,   1.0f - (sy + sh) / th
        };
        C2D_Image sub_img = { img->tex, &sub };
        C2D_DrawImageAt(sub_img, x, y, DRAW_DEPTH, NULL, 1.0f, 1.0f);
    } else {
        C2D_DrawImageAt(*img, x, y, DRAW_DEPTH, NULL, 1.0f, 1.0f);
    }
    return 0;
}

// screen.setSpaceBetweenScreens(n) — no-op on 3DS (fixed layout)
static int screen_setSpaceBetweenScreens(lua_State *L) {
    (void)L;
    return 0;
}

static const luaL_Reg screenlib[] = {
    {"init",                   screen_init},
    {"switch",                 screen_switch},
    {"getMainLcd",             screen_getMainLcd},
    {"startDrawing2D",         screen_startDrawing2D},
    {"endDrawing",             screen_endDrawing},
    {"waitForVBL",             screen_waitForVBL},
    {"getLayer",               screen_getLayer},
    {"getAlphaLevel",          screen_getAlphaLevel},
    {"setAlpha",               screen_setAlpha},
    {"print",                  screen_print},
    {"printFont",              screen_printFont},
    {"blit",                   screen_blit},
    {"drawLine",               screen_drawLine},
    {"drawPoint",              screen_drawPoint},
    {"drawRect",               screen_drawRect},
    {"drawFillRect",           screen_drawFillRect},
    {"drawGradientRect",       screen_drawGradientRect},
    {"drawTextBox",            screen_drawTextBox},
    {"setSpaceBetweenScreens", screen_setSpaceBetweenScreens},
    {NULL, NULL}
};

LUALIB_API int luaopen_screen(lua_State *L) {
    luaL_register(L, LUA_SCREENLIBNAME, screenlib);
    return 1;
}
