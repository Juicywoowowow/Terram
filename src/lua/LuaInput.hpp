#pragma once

extern "C" {
#include <lua-5.4/lua.h>
}

namespace terram {

class LuaInput {
public:
    static void registerAPI(lua_State* L);

private:
    static int isKeyDown(lua_State* L);
    static int isKeyPressed(lua_State* L);
    static int isMouseDown(lua_State* L);
    static int getMouseX(lua_State* L);
    static int getMouseY(lua_State* L);
};

} // namespace terram
