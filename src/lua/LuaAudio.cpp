#include "LuaAudio.hpp"
#include "audio/Audio.hpp"

extern "C" {
#include <lua-5.4/lauxlib.h>
}

#include <memory>
#include <unordered_map>

namespace terram {

static std::unordered_map<int, std::shared_ptr<Sound>> s_sounds;
static std::unordered_map<int, std::shared_ptr<Music>> s_musics;
static int s_nextSoundId = 1;
static int s_nextMusicId = 1;

void LuaAudio::registerAPI(lua_State* L) {
    lua_newtable(L);

    lua_pushcfunction(L, newSource);
    lua_setfield(L, -2, "newSource");

    lua_pushcfunction(L, play);
    lua_setfield(L, -2, "play");

    lua_pushcfunction(L, stop);
    lua_setfield(L, -2, "stop");

    lua_pushcfunction(L, setVolume);
    lua_setfield(L, -2, "setVolume");

    lua_pushcfunction(L, newMusic);
    lua_setfield(L, -2, "newMusic");

    lua_pushcfunction(L, playMusic);
    lua_setfield(L, -2, "playMusic");

    lua_pushcfunction(L, stopMusic);
    lua_setfield(L, -2, "stopMusic");

    lua_pushcfunction(L, pauseMusic);
    lua_setfield(L, -2, "pauseMusic");

    lua_pushcfunction(L, resumeMusic);
    lua_setfield(L, -2, "resumeMusic");

    lua_pushcfunction(L, setMasterVolume);
    lua_setfield(L, -2, "setMasterVolume");

    lua_setfield(L, -2, "audio");
}

int LuaAudio::newSource(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);

    auto sound = std::make_shared<Sound>();
    if (!sound->load(path)) {
        lua_pushnil(L);
        return 1;
    }

    int id = s_nextSoundId++;
    s_sounds[id] = sound;

    lua_newtable(L);
    lua_pushinteger(L, id);
    lua_setfield(L, -2, "_soundId");
    lua_pushstring(L, "sound");
    lua_setfield(L, -2, "type");

    return 1;
}

int LuaAudio::play(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "_soundId");
    int id = lua_tointeger(L, -1);
    lua_pop(L, 1);

    auto it = s_sounds.find(id);
    if (it == s_sounds.end()) {
        return luaL_error(L, "Invalid sound source");
    }

    int loops = luaL_optinteger(L, 2, 0);
    it->second->play(loops);
    return 0;
}

int LuaAudio::stop(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "_soundId");
    int id = lua_tointeger(L, -1);
    lua_pop(L, 1);

    auto it = s_sounds.find(id);
    if (it != s_sounds.end()) {
        it->second->stop();
    }
    return 0;
}

int LuaAudio::setVolume(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    float volume = luaL_checknumber(L, 2);

    lua_getfield(L, 1, "_soundId");
    int id = lua_tointeger(L, -1);
    lua_pop(L, 1);

    auto it = s_sounds.find(id);
    if (it != s_sounds.end()) {
        it->second->setVolume(volume);
    }
    return 0;
}

int LuaAudio::newMusic(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);

    auto music = std::make_shared<Music>();
    if (!music->load(path)) {
        lua_pushnil(L);
        return 1;
    }

    int id = s_nextMusicId++;
    s_musics[id] = music;

    lua_newtable(L);
    lua_pushinteger(L, id);
    lua_setfield(L, -2, "_musicId");
    lua_pushstring(L, "music");
    lua_setfield(L, -2, "type");

    return 1;
}

int LuaAudio::playMusic(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "_musicId");
    int id = lua_tointeger(L, -1);
    lua_pop(L, 1);

    auto it = s_musics.find(id);
    if (it == s_musics.end()) {
        return luaL_error(L, "Invalid music");
    }

    int loops = luaL_optinteger(L, 2, -1);
    it->second->play(loops);
    return 0;
}

int LuaAudio::stopMusic(lua_State* L) {
    Music::stop();
    return 0;
}

int LuaAudio::pauseMusic(lua_State* L) {
    Music::pause();
    return 0;
}

int LuaAudio::resumeMusic(lua_State* L) {
    Music::resume();
    return 0;
}

int LuaAudio::setMasterVolume(lua_State* L) {
    float volume = luaL_checknumber(L, 1);
    Audio::setMasterVolume(volume);
    return 0;
}

} // namespace terram
