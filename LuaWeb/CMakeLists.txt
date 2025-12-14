cmake_minimum_required(VERSION 3.16)
project(LuaWeb VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type defaults to Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find Lua
find_package(Lua REQUIRED)
message(STATUS "Found Lua: ${LUA_VERSION_STRING}")
message(STATUS "Lua include: ${LUA_INCLUDE_DIR}")
message(STATUS "Lua libraries: ${LUA_LIBRARIES}")

# Core library (static)
add_library(luaweb_core STATIC
    src/core/server.cpp
    src/core/request.cpp
    src/core/response.cpp
)
target_include_directories(luaweb_core PUBLIC 
    ${CMAKE_SOURCE_DIR}/src/core
)
target_compile_options(luaweb_core PRIVATE -Wall -Wextra)

# Lua bindings library (shared for Lua require)
add_library(luaweb SHARED
    src/bindings/lua_server.cpp
)
target_include_directories(luaweb PRIVATE 
    ${LUA_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(luaweb PRIVATE 
    luaweb_core 
    ${LUA_LIBRARIES}
)
target_compile_options(luaweb PRIVATE -Wall -Wextra)

# Remove lib prefix for Lua module loading (luaweb.so not libluaweb.so)
set_target_properties(luaweb PROPERTIES 
    PREFIX ""
    OUTPUT_NAME "luaweb"
)

# On macOS, use .so extension for Lua modules (not .dylib)
if(APPLE)
    set_target_properties(luaweb PROPERTIES SUFFIX ".so")
endif()

# Main executable
add_executable(luaweb_runner src/main.cpp)
target_include_directories(luaweb_runner PRIVATE 
    ${LUA_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(luaweb_runner PRIVATE 
    luaweb
    luaweb_core 
    ${LUA_LIBRARIES}
)
target_compile_options(luaweb_runner PRIVATE -Wall -Wextra)

# Install targets
install(TARGETS luaweb_runner DESTINATION bin)
install(TARGETS luaweb DESTINATION lib/lua/${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR})
install(DIRECTORY lua/ DESTINATION share/luaweb/lua)
install(DIRECTORY scripts/ DESTINATION share/luaweb/scripts)

# Custom target to copy files for local testing
add_custom_target(setup_local
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lua
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/scripts
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/lua ${CMAKE_BINARY_DIR}/lua
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/scripts ${CMAKE_BINARY_DIR}/scripts
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:luaweb> ${CMAKE_BINARY_DIR}/
    COMMENT "Setting up local test environment"
)
add_dependencies(setup_local luaweb)

message(STATUS "")
message(STATUS "=== LuaWeb Build Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
