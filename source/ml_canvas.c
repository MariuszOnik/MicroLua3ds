#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#define LUA_LIB
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <citro2d.h>
#include "vars.h"
#include "ml_canvas.h"

static const float DRAW_DEPTH = 0.5f;
static const float TEXT_SCALE = 0.5f;

// ---- data structures (same layout as MicroLua NDS canvas.c) -------------

typedef struct {
    u8    type;
    int   x1, y1, x2, y2, x3, y3;
    u32   color, color1, color2, color3, color4;
    char *text;
    bool  visible;
    // font field ignored — no custom font support yet
    C2D_Image *image;
} CanvasObject;

typedef struct {
    int           nb;
    CanvasObject *list[MAX_OBJECTS];
} Canvas;

// ---- drawing helpers shared with screen module --------------------------

static void cv_drawLine(float x0, float y0, float x1, float y1, u32 color) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.5f) {
        C2D_DrawRectSolid(x0, y0, DRAW_DEPTH, 1.0f, 1.0f, color);
        return;
    }
    float nx = -dy / len * 0.5f, ny = dx / len * 0.5f;
    C2D_DrawTriangle(x0+nx, y0+ny, color, x0-nx, y0-ny, color, x1+nx, y1+ny, color, DRAW_DEPTH);
    C2D_DrawTriangle(x0-nx, y0-ny, color, x1-nx, y1-ny, color, x1+nx, y1+ny, color, DRAW_DEPTH);
}

static void cv_drawText(float x, float y, const char *text, u32 color) {
    static C2D_TextBuf buf = NULL;
    if (!buf) buf = C2D_TextBufNew(2048);
    C2D_TextBufClear(buf);
    C2D_Text t;
    C2D_TextParse(&t, buf, text);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor | C2D_AtBaseline, x, y, DRAW_DEPTH,
                 TEXT_SCALE, TEXT_SCALE, color);
}

// ---- Lua bindings -------------------------------------------------------

static int canvas_new(lua_State *L) {
    Canvas *canvas = malloc(sizeof(Canvas));
    canvas->nb = 0;
    lua_pushlightuserdata(L, canvas);
    return 1;
}

static int canvas_destroy(lua_State *L) {
    Canvas *canvas = lua_touserdata(L, 1);
    for (int i = 0; i < canvas->nb; i++) {
        if (canvas->list[i]->text) free(canvas->list[i]->text);
        free(canvas->list[i]);
    }
    free(canvas);
    return 0;
}

static int canvas_newLine(lua_State *L) {
    CanvasObject *co = calloc(1, sizeof(CanvasObject));
    co->visible = true;
    co->type   = CANVAS_TYPE_LINE;
    co->x1 = (int)luaL_checknumber(L, 1);
    co->y1 = (int)luaL_checknumber(L, 2);
    co->x2 = (int)luaL_checknumber(L, 3);
    co->y2 = (int)luaL_checknumber(L, 4);
    co->color = (u32)luaL_checknumber(L, 5);
    lua_pushlightuserdata(L, co);
    return 1;
}

static int canvas_newPoint(lua_State *L) {
    CanvasObject *co = calloc(1, sizeof(CanvasObject));
    co->visible = true;
    co->type   = CANVAS_TYPE_POINT;
    co->x1 = (int)luaL_checknumber(L, 1);
    co->y1 = (int)luaL_checknumber(L, 2);
    co->color = (u32)luaL_checknumber(L, 3);
    lua_pushlightuserdata(L, co);
    return 1;
}

static int canvas_newRect(lua_State *L) {
    CanvasObject *co = calloc(1, sizeof(CanvasObject));
    co->visible = true;
    co->type   = CANVAS_TYPE_RECT;
    co->x1 = (int)luaL_checknumber(L, 1);
    co->y1 = (int)luaL_checknumber(L, 2);
    co->x2 = (int)luaL_checknumber(L, 3);
    co->y2 = (int)luaL_checknumber(L, 4);
    co->color = (u32)luaL_checknumber(L, 5);
    lua_pushlightuserdata(L, co);
    return 1;
}

static int canvas_newFillRect(lua_State *L) {
    CanvasObject *co = calloc(1, sizeof(CanvasObject));
    co->visible = true;
    co->type   = CANVAS_TYPE_FILLRECT;
    co->x1 = (int)luaL_checknumber(L, 1);
    co->y1 = (int)luaL_checknumber(L, 2);
    co->x2 = (int)luaL_checknumber(L, 3);
    co->y2 = (int)luaL_checknumber(L, 4);
    co->color = (u32)luaL_checknumber(L, 5);
    lua_pushlightuserdata(L, co);
    return 1;
}

static int canvas_newGradientRect(lua_State *L) {
    CanvasObject *co = calloc(1, sizeof(CanvasObject));
    co->visible = true;
    co->type   = CANVAS_TYPE_GRADIENTRECT;
    co->x1 = (int)luaL_checknumber(L, 1);
    co->y1 = (int)luaL_checknumber(L, 2);
    co->x2 = (int)luaL_checknumber(L, 3);
    co->y2 = (int)luaL_checknumber(L, 4);
    co->color1 = (u32)luaL_checknumber(L, 5);
    co->color2 = (u32)luaL_checknumber(L, 6);
    co->color3 = (u32)luaL_checknumber(L, 7);
    co->color4 = (u32)luaL_checknumber(L, 8);
    lua_pushlightuserdata(L, co);
    return 1;
}

static int canvas_newText(lua_State *L) {
    CanvasObject *co = calloc(1, sizeof(CanvasObject));
    co->visible = true;
    co->type = CANVAS_TYPE_TEXT;
    co->x1 = (int)luaL_checknumber(L, 1);
    co->y1 = (int)luaL_checknumber(L, 2);
    const char *text = luaL_checkstring(L, 3);
    co->color = lua_isnumber(L, 4)
                ? (u32)luaL_checknumber(L, 4)
                : C2D_Color32(255, 255, 255, 255);
    co->text = malloc(strlen(text) + 1);
    strcpy(co->text, text);
    lua_pushlightuserdata(L, co);
    return 1;
}

static int canvas_newTextFont(lua_State *L) {
    // font parameter ignored for now
    CanvasObject *co = calloc(1, sizeof(CanvasObject));
    co->visible = true;
    co->type = CANVAS_TYPE_TEXTFONT;
    co->x1 = (int)luaL_checknumber(L, 1);
    co->y1 = (int)luaL_checknumber(L, 2);
    const char *text = luaL_checkstring(L, 3);
    co->color = lua_isnumber(L, 4)
                ? (u32)luaL_checknumber(L, 4)
                : C2D_Color32(255, 255, 255, 255);
    co->text = malloc(strlen(text) + 1);
    strcpy(co->text, text);
    lua_pushlightuserdata(L, co);
    return 1;
}

static int canvas_newTextBox(lua_State *L) {
    CanvasObject *co = calloc(1, sizeof(CanvasObject));
    co->visible = true;
    co->type = CANVAS_TYPE_TEXTBOX;
    co->x1 = (int)luaL_checknumber(L, 1);
    co->y1 = (int)luaL_checknumber(L, 2);
    co->x2 = (int)luaL_checknumber(L, 3);
    co->y2 = (int)luaL_checknumber(L, 4);
    const char *text = luaL_checkstring(L, 5);
    co->color = lua_isnumber(L, 6)
                ? (u32)luaL_checknumber(L, 6)
                : C2D_Color32(255, 255, 255, 255);
    co->text = malloc(strlen(text) + 1);
    strcpy(co->text, text);
    lua_pushlightuserdata(L, co);
    return 1;
}

static int canvas_newImage(lua_State *L) {
    CanvasObject *co = calloc(1, sizeof(CanvasObject));
    co->visible = true;
    co->type = CANVAS_TYPE_IMAGE;
    co->x1 = (int)luaL_checknumber(L, 1);
    co->y1 = (int)luaL_checknumber(L, 2);
    co->image = (C2D_Image *)lua_touserdata(L, 3);
    co->x2 = lua_isnumber(L, 4) ? (int)luaL_checknumber(L, 4) : -1;
    co->y2 = lua_isnumber(L, 5) ? (int)luaL_checknumber(L, 5) : -1;
    co->x3 = lua_isnumber(L, 6) ? (int)luaL_checknumber(L, 6) : -1;
    co->y3 = lua_isnumber(L, 7) ? (int)luaL_checknumber(L, 7) : -1;
    lua_pushlightuserdata(L, co);
    return 1;
}

static int canvas_add(lua_State *L) {
    Canvas *canvas = lua_touserdata(L, 1);
    CanvasObject *co = lua_touserdata(L, 2);
    if (canvas->nb < MAX_OBJECTS)
        canvas->list[canvas->nb++] = co;
    return 0;
}

// canvas.draw(screen, canvas, offsetX, offsetY)
static int canvas_draw(lua_State *L) {
    int scr      = (int)luaL_checknumber(L, 1);
    Canvas *canvas = lua_touserdata(L, 2);
    int ox       = (int)luaL_checknumber(L, 3);
    int oy       = (int)luaL_checknumber(L, 4);

    ml_assert(L, scr == SCREEN_UP_DISPLAY || scr == SCREEN_DOWN_DISPLAY, "Bad screen number");

    C3D_RenderTarget *target = ml_getTarget(scr);
    C2D_SceneBegin(target);

    for (int i = 0; i < canvas->nb; i++) {
        CanvasObject *o = canvas->list[i];
        if (!o->visible) continue;

        float ax = o->x1 + ox, ay = o->y1 + oy;
        float bx = o->x2 + ox, by = o->y2 + oy;

        switch (o->type) {
            case CANVAS_TYPE_LINE:
                cv_drawLine(ax, ay, bx, by, o->color);
                break;
            case CANVAS_TYPE_POINT:
                C2D_DrawRectSolid(ax, ay, DRAW_DEPTH, 1.0f, 1.0f, o->color);
                break;
            case CANVAS_TYPE_RECT: {
                float w = bx - ax, h = by - ay;
                C2D_DrawRectSolid(ax,      ay,      DRAW_DEPTH, w,    1.0f, o->color);
                C2D_DrawRectSolid(ax,      by - 1, DRAW_DEPTH, w,    1.0f, o->color);
                C2D_DrawRectSolid(ax,      ay,      DRAW_DEPTH, 1.0f, h,    o->color);
                C2D_DrawRectSolid(bx - 1, ay,      DRAW_DEPTH, 1.0f, h,    o->color);
                break;
            }
            case CANVAS_TYPE_FILLRECT:
                C2D_DrawRectSolid(ax, ay, DRAW_DEPTH, bx - ax, by - ay, o->color);
                break;
            case CANVAS_TYPE_GRADIENTRECT:
                // MicroLua order: TL, BL, BR, TR → citro2d order: TL, TR, BL, BR
                C2D_DrawRectangle(ax, ay, DRAW_DEPTH, bx - ax, by - ay,
                                  o->color1, o->color4, o->color2, o->color3);
                break;
            case CANVAS_TYPE_TEXT:
            case CANVAS_TYPE_TEXTFONT:
                cv_drawText(ax, ay, o->text, o->color);
                break;
            case CANVAS_TYPE_TEXTBOX:
                cv_drawText(ax, ay, o->text, o->color);
                break;
            case CANVAS_TYPE_IMAGE:
                if (o->image) {
                    if (o->x3 != -1) {
                        // Sub-region blit
                        float tw = (float)o->image->tex->width;
                        float th = (float)o->image->tex->height;
                        Tex3DS_SubTexture sub = {
                            (u16)o->x3, (u16)o->y3,
                            o->x2 / tw,           1.0f - o->y2 / th,
                            (o->x2 + o->x3) / tw, 1.0f - (o->y2 + o->y3) / th
                        };
                        C2D_Image sub_img = { o->image->tex, &sub };
                        C2D_DrawImageAt(sub_img, ax, ay, DRAW_DEPTH, NULL, 1.0f, 1.0f);
                    } else {
                        C2D_DrawImageAt(*o->image, ax, ay, DRAW_DEPTH, NULL, 1.0f, 1.0f);
                    }
                }
                break;
            default:
                break;
        }
    }
    return 0;
}

static int canvas_setAttr(lua_State *L) {
    CanvasObject *co = lua_touserdata(L, 1);
    int attr = (int)luaL_checknumber(L, 2);
    switch (attr) {
        case ATTR_X1:      co->x1     = (int)luaL_checknumber(L, 3); break;
        case ATTR_Y1:      co->y1     = (int)luaL_checknumber(L, 3); break;
        case ATTR_X2:      co->x2     = (int)luaL_checknumber(L, 3); break;
        case ATTR_Y2:      co->y2     = (int)luaL_checknumber(L, 3); break;
        case ATTR_X3:      co->x3     = (int)luaL_checknumber(L, 3); break;
        case ATTR_Y3:      co->y3     = (int)luaL_checknumber(L, 3); break;
        case ATTR_COLOR:   co->color  = (u32)luaL_checknumber(L, 3); break;
        case ATTR_COLOR1:  co->color1 = (u32)luaL_checknumber(L, 3); break;
        case ATTR_COLOR2:  co->color2 = (u32)luaL_checknumber(L, 3); break;
        case ATTR_COLOR3:  co->color3 = (u32)luaL_checknumber(L, 3); break;
        case ATTR_COLOR4:  co->color4 = (u32)luaL_checknumber(L, 3); break;
        case ATTR_TEXT: {
            const char *text = luaL_checkstring(L, 3);
            free(co->text);
            co->text = malloc(strlen(text) + 1);
            strcpy(co->text, text);
            break;
        }
        case ATTR_VISIBLE: co->visible = lua_toboolean(L, 3); break;
        case ATTR_IMAGE:   co->image   = lua_touserdata(L, 3); break;
        default: luaL_error(L, "Unknown canvas attribute"); return 0;
    }
    return 0;
}

static int canvas_getAttr(lua_State *L) {
    CanvasObject *co = lua_touserdata(L, 1);
    int attr = (int)luaL_checknumber(L, 2);
    switch (attr) {
        case ATTR_X1:      lua_pushnumber(L, co->x1);     break;
        case ATTR_Y1:      lua_pushnumber(L, co->y1);     break;
        case ATTR_X2:      lua_pushnumber(L, co->x2);     break;
        case ATTR_Y2:      lua_pushnumber(L, co->y2);     break;
        case ATTR_X3:      lua_pushnumber(L, co->x3);     break;
        case ATTR_Y3:      lua_pushnumber(L, co->y3);     break;
        case ATTR_COLOR:   lua_pushnumber(L, co->color);  break;
        case ATTR_COLOR1:  lua_pushnumber(L, co->color1); break;
        case ATTR_COLOR2:  lua_pushnumber(L, co->color2); break;
        case ATTR_COLOR3:  lua_pushnumber(L, co->color3); break;
        case ATTR_COLOR4:  lua_pushnumber(L, co->color4); break;
        case ATTR_TEXT:    lua_pushstring(L, co->text);   break;
        case ATTR_VISIBLE: lua_pushboolean(L, co->visible); break;
        case ATTR_IMAGE:   lua_pushlightuserdata(L, co->image); break;
        default: luaL_error(L, "Unknown canvas attribute"); return 0;
    }
    return 1;
}

// canvas.setObjOnTop(canvas, obj) — move object to end of draw list
static int canvas_setObjOnTop(lua_State *L) {
    Canvas *canvas = lua_touserdata(L, 1);
    CanvasObject *co = lua_touserdata(L, 2);
    int pos = -1;
    for (int i = 0; i < canvas->nb; i++) {
        if (canvas->list[i] == co) { pos = i; break; }
    }
    if (pos >= 0) {
        for (int i = pos; i < canvas->nb - 1; i++)
            canvas->list[i] = canvas->list[i + 1];
        canvas->list[canvas->nb - 1] = co;
    }
    return 0;
}

// canvas.removeObj(canvas, obj) — remove and free object
static int canvas_removeObj(lua_State *L) {
    Canvas *canvas = lua_touserdata(L, 1);
    CanvasObject *co = lua_touserdata(L, 2);
    int pos = -1;
    for (int i = 0; i < canvas->nb; i++) {
        if (canvas->list[i] == co) { pos = i; break; }
    }
    if (pos >= 0) {
        if (co->text) free(co->text);
        for (int i = pos; i < canvas->nb - 1; i++)
            canvas->list[i] = canvas->list[i + 1];
        free(canvas->list[canvas->nb - 1]);
        canvas->nb--;
    }
    return 0;
}

static const luaL_Reg canvaslib[] = {
    {"new",            canvas_new},
    {"destroy",        canvas_destroy},
    {"newLine",        canvas_newLine},
    {"newPoint",       canvas_newPoint},
    {"newRect",        canvas_newRect},
    {"newFillRect",    canvas_newFillRect},
    {"newGradientRect",canvas_newGradientRect},
    {"newText",        canvas_newText},
    {"newTextFont",    canvas_newTextFont},
    {"newTextBox",     canvas_newTextBox},
    {"newImage",       canvas_newImage},
    {"add",            canvas_add},
    {"draw",           canvas_draw},
    {"setAttr",        canvas_setAttr},
    {"getAttr",        canvas_getAttr},
    {"setObjOnTop",    canvas_setObjOnTop},
    {"removeObj",      canvas_removeObj},
    {NULL, NULL}
};

LUALIB_API int luaopen_canvas(lua_State *L) {
    luaL_register(L, LUA_CANVASLIBNAME, canvaslib);
    return 1;
}
