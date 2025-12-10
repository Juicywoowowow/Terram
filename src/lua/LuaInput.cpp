#include "LuaInput.hpp"
#include "core/Application.hpp"
#include "input/Input.hpp"

extern "C" {
#include <lua-5.4/lauxlib.h>
}

namespace terram {

void LuaInput::registerAPI(lua_State* L) {
    lua_newtable(L);

    lua_pushcfunction(L, isKeyDown);
    lua_setfield(L, -2, "isKeyDown");

    lua_pushcfunction(L, isKeyPressed);
    lua_setfield(L, -2, "isKeyPressed");

    lua_pushcfunction(L, isMouseDown);
    lua_setfield(L, -2, "isMouseDown");

    lua_pushcfunction(L, getMouseX);
    lua_setfield(L, -2, "getMouseX");

    lua_pushcfunction(L, getMouseY);
    lua_setfield(L, -2, "getMouseY");

    lua_setfield(L, -2, "input");
}

int LuaInput::isKeyDown(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    lua_pushboolean(L, Application::getInstance().getInput().isKeyDown(key));
    return 1;
}

int LuaInput::isKeyPressed(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    lua_pushboolean(L, Application::getInstance().getInput().isKeyPressed(key));
    return 1;
}

int LuaInput::isMouseDown(lua_State* L) {
    int button = luaL_checkinteger(L, 1);
    lua_pushboolean(L, Application::getInstance().getInput().isMouseDown(button));
    return 1;
}

int LuaInput::getMouseX(lua_State* L) {
    lua_pushinteger(L, Application::getInstance().getInput().getMouseX());
    return 1;
}

int LuaInput::getMouseY(lua_State* L) {
    lua_pushinteger(L, Application::getInstance().getInput().getMouseY());
    return 1;
}

} // namespace terram
