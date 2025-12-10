#include "LuaWindow.hpp"
#include "core/Application.hpp"
#include "core/Window.hpp"

extern "C" {
#include <lua-5.4/lauxlib.h>
}

namespace terram {

void LuaWindow::registerAPI(lua_State* L) {
    lua_newtable(L);

    lua_pushcfunction(L, setTitle);
    lua_setfield(L, -2, "setTitle");

    lua_pushcfunction(L, getWidth);
    lua_setfield(L, -2, "getWidth");

    lua_pushcfunction(L, getHeight);
    lua_setfield(L, -2, "getHeight");

    lua_setfield(L, -2, "window");
}

int LuaWindow::setTitle(lua_State* L) {
    const char* title = luaL_checkstring(L, 1);
    Application::getInstance().getWindow().setTitle(title);
    return 0;
}

int LuaWindow::getWidth(lua_State* L) {
    lua_pushinteger(L, Application::getInstance().getWindow().getWidth());
    return 1;
}

int LuaWindow::getHeight(lua_State* L) {
    lua_pushinteger(L, Application::getInstance().getWindow().getHeight());
    return 1;
}

} // namespace terram
