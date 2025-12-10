#include "LuaState.hpp"
#include "LuaGraphics.hpp"
#include "LuaWindow.hpp"
#include "LuaInput.hpp"
#include <iostream>

namespace terram {

LuaState::~LuaState() {
    if (m_state) {
        lua_close(m_state);
    }
}

bool LuaState::init() {
    m_state = luaL_newstate();
    if (!m_state) {
        std::cerr << "Failed to create Lua state" << std::endl;
        return false;
    }

    luaL_openlibs(m_state);
    registerAPI();

    return true;
}

void LuaState::registerAPI() {
    // Create terram table
    lua_newtable(m_state);

    // Register sub-modules
    LuaGraphics::registerAPI(m_state);
    LuaWindow::registerAPI(m_state);
    LuaInput::registerAPI(m_state);

    // Set terram as global
    lua_setglobal(m_state, "terram");
}

bool LuaState::loadFile(const std::string& path) {
    if (luaL_dofile(m_state, path.c_str()) != LUA_OK) {
        std::cerr << "Lua error: " << lua_tostring(m_state, -1) << std::endl;
        lua_pop(m_state, 1);
        return false;
    }
    return true;
}

bool LuaState::callGlobalFunction(const char* name) {
    // Get terram table
    lua_getglobal(m_state, "terram");
    if (!lua_istable(m_state, -1)) {
        lua_pop(m_state, 1);
        return false;
    }

    // Get function
    lua_getfield(m_state, -1, name);
    if (!lua_isfunction(m_state, -1)) {
        lua_pop(m_state, 2);
        return false;
    }

    // Remove terram table from stack
    lua_remove(m_state, -2);
    return true;
}

void LuaState::callLoad() {
    if (callGlobalFunction("load")) {
        if (lua_pcall(m_state, 0, 0, 0) != LUA_OK) {
            std::cerr << "Error in terram.load: " << lua_tostring(m_state, -1) << std::endl;
            lua_pop(m_state, 1);
        }
    }
}

void LuaState::callUpdate(float dt) {
    if (callGlobalFunction("update")) {
        lua_pushnumber(m_state, dt);
        if (lua_pcall(m_state, 1, 0, 0) != LUA_OK) {
            std::cerr << "Error in terram.update: " << lua_tostring(m_state, -1) << std::endl;
            lua_pop(m_state, 1);
        }
    }
}

void LuaState::callDraw() {
    if (callGlobalFunction("draw")) {
        if (lua_pcall(m_state, 0, 0, 0) != LUA_OK) {
            std::cerr << "Error in terram.draw: " << lua_tostring(m_state, -1) << std::endl;
            lua_pop(m_state, 1);
        }
    }
}

} // namespace terram
