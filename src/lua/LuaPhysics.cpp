#include "LuaPhysics.hpp"
#include "physics/Physics.hpp"

extern "C" {
#include <lua-5.4/lauxlib.h>
}

namespace terram {

void LuaPhysics::registerAPI(lua_State* L) {
    lua_newtable(L);

    lua_pushcfunction(L, rectanglesOverlap);
    lua_setfield(L, -2, "rectanglesOverlap");

    lua_pushcfunction(L, circlesOverlap);
    lua_setfield(L, -2, "circlesOverlap");

    lua_pushcfunction(L, pointInRectangle);
    lua_setfield(L, -2, "pointInRect");

    lua_pushcfunction(L, pointInCircle);
    lua_setfield(L, -2, "pointInCircle");

    lua_pushcfunction(L, rectangleCircleOverlap);
    lua_setfield(L, -2, "rectCircleOverlap");

    lua_pushcfunction(L, distance);
    lua_setfield(L, -2, "distance");

    lua_setfield(L, -2, "physics");
}

int LuaPhysics::rectanglesOverlap(lua_State* L) {
    float x1 = luaL_checknumber(L, 1);
    float y1 = luaL_checknumber(L, 2);
    float w1 = luaL_checknumber(L, 3);
    float h1 = luaL_checknumber(L, 4);
    float x2 = luaL_checknumber(L, 5);
    float y2 = luaL_checknumber(L, 6);
    float w2 = luaL_checknumber(L, 7);
    float h2 = luaL_checknumber(L, 8);

    lua_pushboolean(L, Physics::rectanglesOverlap(x1, y1, w1, h1, x2, y2, w2, h2));
    return 1;
}

int LuaPhysics::circlesOverlap(lua_State* L) {
    float x1 = luaL_checknumber(L, 1);
    float y1 = luaL_checknumber(L, 2);
    float r1 = luaL_checknumber(L, 3);
    float x2 = luaL_checknumber(L, 4);
    float y2 = luaL_checknumber(L, 5);
    float r2 = luaL_checknumber(L, 6);

    lua_pushboolean(L, Physics::circlesOverlap(x1, y1, r1, x2, y2, r2));
    return 1;
}

int LuaPhysics::pointInRectangle(lua_State* L) {
    float px = luaL_checknumber(L, 1);
    float py = luaL_checknumber(L, 2);
    float rx = luaL_checknumber(L, 3);
    float ry = luaL_checknumber(L, 4);
    float rw = luaL_checknumber(L, 5);
    float rh = luaL_checknumber(L, 6);

    lua_pushboolean(L, Physics::pointInRectangle(px, py, rx, ry, rw, rh));
    return 1;
}

int LuaPhysics::pointInCircle(lua_State* L) {
    float px = luaL_checknumber(L, 1);
    float py = luaL_checknumber(L, 2);
    float cx = luaL_checknumber(L, 3);
    float cy = luaL_checknumber(L, 4);
    float r = luaL_checknumber(L, 5);

    lua_pushboolean(L, Physics::pointInCircle(px, py, cx, cy, r));
    return 1;
}

int LuaPhysics::rectangleCircleOverlap(lua_State* L) {
    float rx = luaL_checknumber(L, 1);
    float ry = luaL_checknumber(L, 2);
    float rw = luaL_checknumber(L, 3);
    float rh = luaL_checknumber(L, 4);
    float cx = luaL_checknumber(L, 5);
    float cy = luaL_checknumber(L, 6);
    float cr = luaL_checknumber(L, 7);

    lua_pushboolean(L, Physics::rectangleCircleOverlap(rx, ry, rw, rh, cx, cy, cr));
    return 1;
}

int LuaPhysics::distance(lua_State* L) {
    float x1 = luaL_checknumber(L, 1);
    float y1 = luaL_checknumber(L, 2);
    float x2 = luaL_checknumber(L, 3);
    float y2 = luaL_checknumber(L, 4);

    lua_pushnumber(L, Physics::distance(x1, y1, x2, y2));
    return 1;
}

} // namespace terram
