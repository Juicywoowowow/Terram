#include "bindings/lua_server.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

void print_usage(const char* program) {
    std::cout << "LuaWeb - Simple HTTP Server for Lua\n\n";
    std::cout << "Usage: " << program << " <script.lua>\n";
    std::cout << "       " << program << " -e \"lua code\"\n";
    std::cout << "       " << program << " -i (interactive mode)\n\n";
    std::cout << "Options:\n";
    std::cout << "  -e <code>    Execute Lua code directly\n";
    std::cout << "  -i           Start interactive REPL\n";
    std::cout << "  -h, --help   Show this help message\n";
}

int run_file(lua_State* L, const std::string& filename) {
    if (luaL_dofile(L, filename.c_str()) != LUA_OK) {
        std::cerr << "Error: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
        return 1;
    }
    return 0;
}

int run_string(lua_State* L, const std::string& code) {
    if (luaL_dostring(L, code.c_str()) != LUA_OK) {
        std::cerr << "Error: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
        return 1;
    }
    return 0;
}

int run_repl(lua_State* L) {
    std::cout << "LuaWeb Interactive Mode (Lua " << LUA_VERSION_MAJOR << "." << LUA_VERSION_MINOR << ")\n";
    std::cout << "Type 'exit' or Ctrl+D to quit\n\n";
    
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            break;
        }
        
        if (line == "exit" || line == "quit") {
            break;
        }
        
        if (line.empty()) {
            continue;
        }
        
        if (luaL_dostring(L, line.c_str()) != LUA_OK) {
            std::cerr << lua_tostring(L, -1) << std::endl;
            lua_pop(L, 1);
        }
    }
    
    std::cout << "\nGoodbye!\n";
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    std::string arg1 = argv[1];
    
    if (arg1 == "-h" || arg1 == "--help") {
        print_usage(argv[0]);
        return 0;
    }
    
    // Initialize Lua
    lua_State* L = luaL_newstate();
    if (!L) {
        std::cerr << "Failed to create Lua state" << std::endl;
        return 1;
    }
    
    luaL_openlibs(L);
    
    // Pre-load the luaweb module
    luaL_requiref(L, "luaweb", luaopen_luaweb, 1);
    lua_pop(L, 1);
    
    int result = 0;
    
    if (arg1 == "-e") {
        if (argc < 3) {
            std::cerr << "Error: -e requires a code argument" << std::endl;
            result = 1;
        } else {
            result = run_string(L, argv[2]);
        }
    } else if (arg1 == "-i") {
        result = run_repl(L);
    } else {
        // Treat as file
        result = run_file(L, arg1);
    }
    
    lua_close(L);
    return result;
}
