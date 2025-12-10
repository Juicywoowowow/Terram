#pragma once

extern "C" {
#include <lua-5.4/lua.h>
#include <lua-5.4/lualib.h>
#include <lua-5.4/lauxlib.h>
}

#include <string>
#include <memory>

namespace terram {

class LuaState {
public:
    LuaState() = default;
    ~LuaState();

    bool init();
    bool loadFile(const std::string& path);

    void callLoad();
    void callUpdate(float dt);
    void callDraw();

    lua_State* getState() const { return m_state; }

private:
    void registerAPI();
    bool callGlobalFunction(const char* name);

    lua_State* m_state = nullptr;
};

} // namespace terram
