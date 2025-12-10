#pragma once

extern "C" {
#include <lua-5.4/lua.h>
}

namespace terram {

class LuaGraphics {
public:
    static void registerAPI(lua_State* L);

private:
    static int clear(lua_State* L);
    static int setColor(lua_State* L);
    static int rectangle(lua_State* L);
    static int circle(lua_State* L);
    static int line(lua_State* L);
    static int newImage(lua_State* L);
    static int draw(lua_State* L);
};

} // namespace terram
