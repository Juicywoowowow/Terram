#include "LuaGraphics.hpp"
#include "core/Application.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/Texture.hpp"

extern "C" {
#include <lua-5.4/lauxlib.h>
}

#include <memory>
#include <unordered_map>
#include <string>

namespace terram {

// Store textures with shared_ptr
static std::unordered_map<int, std::shared_ptr<Texture>> s_textures;
static int s_nextTextureId = 1;

void LuaGraphics::registerAPI(lua_State* L) {
    // Create graphics table
    lua_newtable(L);

    lua_pushcfunction(L, clear);
    lua_setfield(L, -2, "clear");

    lua_pushcfunction(L, setColor);
    lua_setfield(L, -2, "setColor");

    lua_pushcfunction(L, rectangle);
    lua_setfield(L, -2, "rectangle");

    lua_pushcfunction(L, circle);
    lua_setfield(L, -2, "circle");

    lua_pushcfunction(L, line);
    lua_setfield(L, -2, "line");

    lua_pushcfunction(L, newImage);
    lua_setfield(L, -2, "newImage");

    lua_pushcfunction(L, draw);
    lua_setfield(L, -2, "draw");

    // Set graphics as field of terram
    lua_setfield(L, -2, "graphics");
}

int LuaGraphics::clear(lua_State* L) {
    float r = luaL_optnumber(L, 1, 0);
    float g = luaL_optnumber(L, 2, 0);
    float b = luaL_optnumber(L, 3, 0);
    float a = luaL_optnumber(L, 4, 1);

    Application::getInstance().getRenderer().clear({r, g, b, a});
    return 0;
}

int LuaGraphics::setColor(lua_State* L) {
    float r = luaL_checknumber(L, 1);
    float g = luaL_checknumber(L, 2);
    float b = luaL_checknumber(L, 3);
    float a = luaL_optnumber(L, 4, 1);

    Application::getInstance().getRenderer().setColor({r, g, b, a});
    return 0;
}

int LuaGraphics::rectangle(lua_State* L) {
    const char* mode = luaL_checkstring(L, 1);
    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);
    float w = luaL_checknumber(L, 4);
    float h = luaL_checknumber(L, 5);

    Application::getInstance().getRenderer().rectangle(mode, x, y, w, h);
    return 0;
}

int LuaGraphics::circle(lua_State* L) {
    const char* mode = luaL_checkstring(L, 1);
    float x = luaL_checknumber(L, 2);
    float y = luaL_checknumber(L, 3);
    float r = luaL_checknumber(L, 4);
    int segments = luaL_optinteger(L, 5, 32);

    Application::getInstance().getRenderer().circle(mode, x, y, r, segments);
    return 0;
}

int LuaGraphics::line(lua_State* L) {
    float x1 = luaL_checknumber(L, 1);
    float y1 = luaL_checknumber(L, 2);
    float x2 = luaL_checknumber(L, 3);
    float y2 = luaL_checknumber(L, 4);

    Application::getInstance().getRenderer().line(x1, y1, x2, y2);
    return 0;
}

int LuaGraphics::newImage(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);

    auto texture = std::make_shared<Texture>();
    if (!texture->load(path)) {
        lua_pushnil(L);
        return 1;
    }

    int id = s_nextTextureId++;
    s_textures[id] = texture;

    // Create userdata table with id
    lua_newtable(L);
    lua_pushinteger(L, id);
    lua_setfield(L, -2, "_id");
    lua_pushinteger(L, texture->getWidth());
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, texture->getHeight());
    lua_setfield(L, -2, "height");

    return 1;
}

int LuaGraphics::draw(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "_id");
    int id = lua_tointeger(L, -1);
    lua_pop(L, 1);

    auto it = s_textures.find(id);
    if (it == s_textures.end()) {
        return luaL_error(L, "Invalid image");
    }

    float x = luaL_optnumber(L, 2, 0);
    float y = luaL_optnumber(L, 3, 0);
    float r = luaL_optnumber(L, 4, 0);
    float sx = luaL_optnumber(L, 5, 1);
    float sy = luaL_optnumber(L, 6, 1);

    Application::getInstance().getRenderer().draw(*it->second, x, y, r, sx, sy);
    return 0;
}

} // namespace terram
