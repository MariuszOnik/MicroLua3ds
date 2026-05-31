#pragma once
#include <3ds.h>
#include <citro2d.h>
#include <stdbool.h>

// Drop-in replacement for the MicroLua assert macro
#define ml_assert(L, cond, msg) do { if(!(cond)){ luaL_error(L, msg); return 0; } } while(0)

// Canvas object types (same values as MicroLua NDS)
#define CANVAS_TYPE_LINE          0
#define CANVAS_TYPE_POINT         1
#define CANVAS_TYPE_RECT          2
#define CANVAS_TYPE_FILLRECT      3
#define CANVAS_TYPE_GRADIENTRECT  4
#define CANVAS_TYPE_TEXT          5
#define CANVAS_TYPE_TEXTFONT      6
#define CANVAS_TYPE_TEXTBOX       7
#define CANVAS_TYPE_IMAGE         8

// Canvas object attribute IDs (same as MicroLua NDS)
#define ATTR_X1      0
#define ATTR_Y1      1
#define ATTR_X2      2
#define ATTR_Y2      3
#define ATTR_X3      4
#define ATTR_Y3      5
#define ATTR_COLOR   6
#define ATTR_COLOR1  7
#define ATTR_COLOR2  8
#define ATTR_COLOR3  9
#define ATTR_COLOR4  10
#define ATTR_TEXT    11
#define ATTR_VISIBLE 12
#define ATTR_FONT    13
#define ATTR_IMAGE   14

#define MAX_OBJECTS 2000

// Global render targets – defined in main.c
extern C3D_RenderTarget* topTarget;
extern C3D_RenderTarget* bottomTarget;

// Logical display mapping – 1 = top screen, 0 = bottom screen
// screen.switch() swaps these, matching MicroLua NDS behaviour
extern int SCREEN_UP_DISPLAY;
extern int SCREEN_DOWN_DISPLAY;

// Returns the render target for a logical screen number
static inline C3D_RenderTarget* ml_getTarget(int screen) {
    return (screen == 1) ? topTarget : bottomTarget;
}
