#include "LuaState.hpp"
#include "LuaGraphics.hpp"
#include "LuaWindow.hpp"
#include "LuaInput.hpp"
#include "LuaPhysics.hpp"
#include "LuaAudio.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <cstdlib>

namespace terram {

static size_t s_luaAllocated = 0;
static size_t s_luaAllocs = 0;
static size_t s_luaFrees = 0;

static std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

static std::string formatBytes(size_t bytes) {
    std::ostringstream oss;
    if (bytes >= 1024 * 1024) {
        oss << std::fixed << std::setprecision(2) << (bytes / (1024.0 * 1024.0)) << " MB";
    } else if (bytes >= 1024) {
        oss << std::fixed << std::setprecision(2) << (bytes / 1024.0) << " KB";
    } else {
        oss << bytes << " bytes";
    }
    return oss.str();
}

// Custom Lua allocator with tracking
static void* luaTrackedAlloc(void* ud, void* ptr, size_t osize, size_t nsize) {
    (void)ud;

    if (nsize == 0) {
        // Free
        if (ptr && osize > 0) {
            s_luaAllocated -= osize;
            s_luaFrees++;
            std::cout << "\033[31m[" << getTimestamp() << "] [LUA FREE]\033[0m  "
                      << "\033[1m" << formatBytes(osize) << "\033[0m"
                      << " at \033[36m0x" << std::hex << reinterpret_cast<uintptr_t>(ptr) << std::dec << "\033[0m"
                      << " | lua heap: " << formatBytes(s_luaAllocated)
                      << std::endl;
        }
        std::free(ptr);
        return nullptr;
    }

    void* newPtr;
    if (ptr == nullptr) {
        // New allocation
        newPtr = std::malloc(nsize);
        if (newPtr) {
            s_luaAllocated += nsize;
            s_luaAllocs++;
            std::cout << "\033[32m[" << getTimestamp() << "] [LUA ALLOC]\033[0m "
                      << "\033[1m" << formatBytes(nsize) << "\033[0m"
                      << " at \033[36m0x" << std::hex << reinterpret_cast<uintptr_t>(newPtr) << std::dec << "\033[0m"
                      << " | lua heap: " << formatBytes(s_luaAllocated)
                      << std::endl;
        }
    } else {
        // Realloc
        newPtr = std::realloc(ptr, nsize);
        if (newPtr) {
            s_luaAllocated = s_luaAllocated - osize + nsize;
            std::cout << "\033[33m[" << getTimestamp() << "] [LUA REALLOC]\033[0m "
                      << formatBytes(osize) << " → \033[1m" << formatBytes(nsize) << "\033[0m"
                      << " at \033[36m0x" << std::hex << reinterpret_cast<uintptr_t>(newPtr) << std::dec << "\033[0m"
                      << " | lua heap: " << formatBytes(s_luaAllocated)
                      << std::endl;
        }
    }

    return newPtr;
}

LuaState::~LuaState() {
    if (m_state) {
        std::cout << "\n\033[1m[Lua] Closing state...\033[0m" << std::endl;
        std::cout << "       Total Lua allocs: " << s_luaAllocs << std::endl;
        std::cout << "       Total Lua frees:  " << s_luaFrees << std::endl;
        lua_close(m_state);
    }
}

bool LuaState::init() {
    std::cout << "\033[1m[Lua] Creating state with tracked allocator...\033[0m\n" << std::endl;
    m_state = lua_newstate(luaTrackedAlloc, nullptr);
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
    LuaPhysics::registerAPI(m_state);
    LuaAudio::registerAPI(m_state);

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
