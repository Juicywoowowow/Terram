#pragma once

extern "C" {
#include <lua-5.4/lua.h>
}

namespace terram {

class LuaAudio {
public:
    static void registerAPI(lua_State* L);

private:
    // Sound (short effects)
    static int newSource(lua_State* L);
    static int play(lua_State* L);
    static int stop(lua_State* L);
    static int setVolume(lua_State* L);

    // Music
    static int newMusic(lua_State* L);
    static int playMusic(lua_State* L);
    static int stopMusic(lua_State* L);
    static int pauseMusic(lua_State* L);
    static int resumeMusic(lua_State* L);

    static int setMasterVolume(lua_State* L);
};

} // namespace terram
