#pragma once

extern "C" {
#include <lua-5.4/lua.h>
}

namespace terram {

class LuaWindow {
public:
    static void registerAPI(lua_State* L);

private:
    static int setTitle(lua_State* L);
    static int getWidth(lua_State* L);
    static int getHeight(lua_State* L);
};

} // namespace terram
