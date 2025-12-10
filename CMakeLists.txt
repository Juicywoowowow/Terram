cmake_minimum_required(VERSION 3.16)
project(Terram VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =============================================================================
# Find Dependencies
# =============================================================================

# SDL2 via Homebrew
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/sdl2")
find_package(SDL2 REQUIRED)

# OpenGL
find_package(OpenGL REQUIRED)

# =============================================================================
# Lua 5.4 (compiled from source)
# =============================================================================
set(LUA_DIR ${CMAKE_SOURCE_DIR}/lib/lua-5.4)
file(GLOB LUA_SOURCES ${LUA_DIR}/*.c)
# Exclude lua.c and luac.c (standalone interpreters)
list(FILTER LUA_SOURCES EXCLUDE REGEX "lua\\.c$")
list(FILTER LUA_SOURCES EXCLUDE REGEX "luac\\.c$")

add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC ${LUA_DIR})
if(APPLE)
    target_compile_definitions(lua PRIVATE LUA_USE_MACOSX)
endif()

# =============================================================================
# Terram Engine
# =============================================================================
set(TERRAM_SOURCES
    src/main.cpp
    src/core/Application.cpp
    src/core/Window.cpp
    src/graphics/Renderer.cpp
    src/graphics/Texture.cpp
    src/input/Input.cpp
    src/lua/LuaState.cpp
    src/lua/LuaGraphics.cpp
    src/lua/LuaWindow.cpp
    src/lua/LuaInput.cpp
)

add_executable(terram ${TERRAM_SOURCES})

target_include_directories(terram PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(terram PRIVATE
    lua
    ${SDL2_LIBRARIES}
    OpenGL::GL
)

# macOS specific
if(APPLE)
    target_link_libraries(terram PRIVATE "-framework Cocoa")
endif()

# =============================================================================
# Output
# =============================================================================
set_target_properties(terram PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

message(STATUS "Terram v${PROJECT_VERSION}")
message(STATUS "SDL2: ${SDL2_LIBRARIES}")
message(STATUS "OpenGL: ${OPENGL_LIBRARIES}")
