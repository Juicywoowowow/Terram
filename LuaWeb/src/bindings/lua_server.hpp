#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "../core/server.hpp"
#include <memory>

namespace luaweb {

/**
 * Lua bindings for the Server class
 */
class LuaServer {
public:
    // Register the luaweb module with Lua
    static int luaopen_luaweb(lua_State* L);
    
    // Helper to get Server* from userdata (public for use in callbacks)
    static Server* check_server(lua_State* L, int index = 1);
    
    // Helper to create response/request tables (public for use in callbacks)
    static void push_request(lua_State* L, Request& req);
    static void setup_response(lua_State* L, Response& res);

private:
    // Module functions
    static int lua_create_server(lua_State* L);
    
    // Server methods
    static int lua_server_get(lua_State* L);
    static int lua_server_post(lua_State* L);
    static int lua_server_put(lua_State* L);
    static int lua_server_delete(lua_State* L);
    static int lua_server_route(lua_State* L);
    static int lua_server_run(lua_State* L);
    static int lua_server_stop(lua_State* L);
    static int lua_server_enable_web_lua(lua_State* L);
    
    // Garbage collection
    static int lua_server_gc(lua_State* L);
};

} // namespace luaweb

// C entry point for require("luaweb")
extern "C" int luaopen_luaweb(lua_State* L);
