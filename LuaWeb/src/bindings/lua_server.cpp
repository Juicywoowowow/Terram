#include "lua_server.hpp"
#include <iostream>
#include <functional>

namespace luaweb {

// Metatables
static const char* SERVER_MT = "LuaWeb.Server";
static const char* RESPONSE_MT = "LuaWeb.Response";

Server* LuaServer::check_server(lua_State* L, int index) {
    void* ud = luaL_checkudata(L, index, SERVER_MT);
    luaL_argcheck(L, ud != nullptr, index, "'Server' expected");
    return *static_cast<Server**>(ud);
}

void LuaServer::push_request(lua_State* L, Request& req) {
    lua_newtable(L);
    
    // Method
    lua_pushstring(L, req.method.c_str());
    lua_setfield(L, -2, "method");
    
    // Path
    lua_pushstring(L, req.path.c_str());
    lua_setfield(L, -2, "path");
    
    // Body
    lua_pushstring(L, req.body.c_str());
    lua_setfield(L, -2, "body");
    
    // Client IP
    lua_pushstring(L, req.client_ip.c_str());
    lua_setfield(L, -2, "ip");
    
    // Headers table
    lua_newtable(L);
    for (const auto& [key, value] : req.headers) {
        lua_pushstring(L, value.c_str());
        lua_setfield(L, -2, key.c_str());
    }
    lua_setfield(L, -2, "headers");
    
    // URL Parameters table (from :param patterns)
    lua_newtable(L);
    for (const auto& [key, value] : req.params) {
        lua_pushstring(L, value.c_str());
        lua_setfield(L, -2, key.c_str());
    }
    lua_setfield(L, -2, "params");
    
    // Query parameters table
    lua_newtable(L);
    for (const auto& [key, value] : req.query_params) {
        lua_pushstring(L, value.c_str());
        lua_setfield(L, -2, key.c_str());
    }
    lua_setfield(L, -2, "query");
}

// Response userdata structure
struct ResponseUD {
    Response* response;
};

static ResponseUD* check_response(lua_State* L, int index = 1) {
    void* ud = luaL_checkudata(L, index, RESPONSE_MT);
    luaL_argcheck(L, ud != nullptr, index, "'Response' expected");
    return static_cast<ResponseUD*>(ud);
}

static int lua_response_status(lua_State* L) {
    ResponseUD* rud = check_response(L, 1);
    int code = luaL_checkinteger(L, 2);
    rud->response->status(code);
    lua_pushvalue(L, 1);  // Return self for chaining
    return 1;
}

static int lua_response_header(lua_State* L) {
    ResponseUD* rud = check_response(L, 1);
    const char* key = luaL_checkstring(L, 2);
    const char* value = luaL_checkstring(L, 3);
    rud->response->header(key, value);
    lua_pushvalue(L, 1);
    return 1;
}

static int lua_response_send(lua_State* L) {
    ResponseUD* rud = check_response(L, 1);
    const char* content = luaL_checkstring(L, 2);
    rud->response->html(content);
    rud->response->mark_sent();
    return 0;
}

static int lua_response_text(lua_State* L) {
    ResponseUD* rud = check_response(L, 1);
    const char* content = luaL_checkstring(L, 2);
    rud->response->text(content);
    rud->response->mark_sent();
    return 0;
}

static int lua_response_json(lua_State* L) {
    ResponseUD* rud = check_response(L, 1);
    
    // If string provided, use directly
    if (lua_isstring(L, 2)) {
        const char* json_str = lua_tostring(L, 2);
        rud->response->json(json_str);
    } else if (lua_istable(L, 2)) {
        // Simple table to JSON conversion
        // For a full implementation, you'd want a proper JSON library
        std::string json = "{";
        bool first = true;
        
        lua_pushnil(L);
        while (lua_next(L, 2) != 0) {
            if (!first) json += ",";
            first = false;
            
            // Key (must be string for JSON object)
            if (lua_isstring(L, -2)) {
                json += "\"";
                json += lua_tostring(L, -2);
                json += "\":";
            }
            
            // Value
            if (lua_isstring(L, -1)) {
                json += "\"";
                json += lua_tostring(L, -1);
                json += "\"";
            } else if (lua_isnumber(L, -1)) {
                json += std::to_string(lua_tonumber(L, -1));
            } else if (lua_isboolean(L, -1)) {
                json += lua_toboolean(L, -1) ? "true" : "false";
            } else {
                json += "null";
            }
            
            lua_pop(L, 1);
        }
        json += "}";
        
        rud->response->json(json);
    }
    
    rud->response->mark_sent();
    return 0;
}

void LuaServer::setup_response(lua_State* L, Response& res) {
    ResponseUD* rud = static_cast<ResponseUD*>(lua_newuserdata(L, sizeof(ResponseUD)));
    rud->response = &res;
    
    luaL_getmetatable(L, RESPONSE_MT);
    lua_setmetatable(L, -2);
}

// Server methods

int LuaServer::lua_create_server(lua_State* L) {
    int port = luaL_optinteger(L, 1, 8080);
    
    // Create userdata
    Server** pserver = static_cast<Server**>(lua_newuserdata(L, sizeof(Server*)));
    *pserver = new Server(port);
    
    luaL_getmetatable(L, SERVER_MT);
    lua_setmetatable(L, -2);
    
    return 1;
}

int LuaServer::lua_server_gc(lua_State* L) {
    Server* server = check_server(L);
    delete server;
    return 0;
}

// Helper to register a route with a Lua callback
static int register_route(lua_State* L, const std::string& method) {
    Server* server = LuaServer::check_server(L, 1);
    const char* path = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TFUNCTION);
    
    // Store the function reference
    lua_pushvalue(L, 3);
    int callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    // Store L in registry for callback access
    lua_pushlightuserdata(L, L);
    lua_setfield(L, LUA_REGISTRYINDEX, "luaweb_state");
    
    server->route(method, path, [L, callback_ref](Request& req, Response& res) {
        // Get the callback function
        lua_rawgeti(L, LUA_REGISTRYINDEX, callback_ref);
        
        // Push request table
        LuaServer::push_request(L, req);
        
        // Push response userdata
        LuaServer::setup_response(L, res);
        
        // Call the function
        if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
            std::cerr << "[LuaWeb] Handler error: " << lua_tostring(L, -1) << std::endl;
            lua_pop(L, 1);
            res.status(500).text("Internal Server Error");
        }
    });
    
    lua_pushvalue(L, 1);  // Return server for chaining
    return 1;
}

int LuaServer::lua_server_get(lua_State* L) {
    return register_route(L, "GET");
}

int LuaServer::lua_server_post(lua_State* L) {
    return register_route(L, "POST");
}

int LuaServer::lua_server_put(lua_State* L) {
    return register_route(L, "PUT");
}

int LuaServer::lua_server_delete(lua_State* L) {
    return register_route(L, "DELETE");
}

int LuaServer::lua_server_route(lua_State* L) {
    const char* method = luaL_checkstring(L, 2);
    // Shift arguments and call helper
    lua_remove(L, 2);
    return register_route(L, method);
}

int LuaServer::lua_server_run(lua_State* L) {
    Server* server = check_server(L);
    bool result = server->run();
    lua_pushboolean(L, result);
    return 1;
}

int LuaServer::lua_server_stop(lua_State* L) {
    Server* server = check_server(L);
    server->stop();
    return 0;
}

int LuaServer::lua_server_enable_web_lua(lua_State* L) {
    Server* server = check_server(L);
    bool enabled = lua_toboolean(L, 2);
    server->enable_web_lua(enabled);
    lua_pushvalue(L, 1);
    return 1;
}

int LuaServer::luaopen_luaweb(lua_State* L) {
    // Create Response metatable
    luaL_newmetatable(L, RESPONSE_MT);
    
    lua_pushcfunction(L, lua_response_status);
    lua_setfield(L, -2, "status");
    lua_pushcfunction(L, lua_response_header);
    lua_setfield(L, -2, "header");
    lua_pushcfunction(L, lua_response_send);
    lua_setfield(L, -2, "send");
    lua_pushcfunction(L, lua_response_text);
    lua_setfield(L, -2, "text");
    lua_pushcfunction(L, lua_response_json);
    lua_setfield(L, -2, "json");
    
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
    
    // Create Server metatable
    luaL_newmetatable(L, SERVER_MT);
    
    lua_pushcfunction(L, lua_server_get);
    lua_setfield(L, -2, "get");
    lua_pushcfunction(L, lua_server_post);
    lua_setfield(L, -2, "post");
    lua_pushcfunction(L, lua_server_put);
    lua_setfield(L, -2, "put");
    lua_pushcfunction(L, lua_server_delete);
    lua_setfield(L, -2, "delete");
    lua_pushcfunction(L, lua_server_route);
    lua_setfield(L, -2, "route");
    lua_pushcfunction(L, lua_server_run);
    lua_setfield(L, -2, "run");
    lua_pushcfunction(L, lua_server_stop);
    lua_setfield(L, -2, "stop");
    lua_pushcfunction(L, lua_server_enable_web_lua);
    lua_setfield(L, -2, "enable_web_lua");
    lua_pushcfunction(L, lua_server_gc);
    lua_setfield(L, -2, "__gc");
    
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
    
    // Create module table
    lua_newtable(L);
    
    lua_pushcfunction(L, lua_create_server);
    lua_setfield(L, -2, "server");
    
    // Version
    lua_pushstring(L, "1.0.0");
    lua_setfield(L, -2, "_VERSION");
    
    return 1;
}

} // namespace luaweb

// C entry point
extern "C" int luaopen_luaweb(lua_State* L) {
    return luaweb::LuaServer::luaopen_luaweb(L);
}
