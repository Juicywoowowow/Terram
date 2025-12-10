#pragma once

extern "C" {
#include <lua-5.4/lua.h>
}

namespace terram {

class LuaPhysics {
public:
    static void registerAPI(lua_State* L);

private:
    static int rectanglesOverlap(lua_State* L);
    static int circlesOverlap(lua_State* L);
    static int pointInRectangle(lua_State* L);
    static int pointInCircle(lua_State* L);
    static int rectangleCircleOverlap(lua_State* L);
    static int distance(lua_State* L);
};

} // namespace terram
