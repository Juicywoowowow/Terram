#include "Application.hpp"
#include "Window.hpp"
#include "MemoryTracker.hpp"
#include "graphics/Renderer.hpp"
#include "input/Input.hpp"
#include "audio/Audio.hpp"
#include "lua/LuaState.hpp"
#include <SDL.h>
#include <iostream>
#include <chrono>

namespace terram {

Application* Application::s_instance = nullptr;

Application::Application() {
    s_instance = this;
}

Application::~Application() {
    std::cout << "\n\033[1m[Terram] Shutting down...\033[0m\n" << std::endl;
    m_lua.reset();
    m_renderer.reset();
    m_input.reset();
    m_window.reset();
    Audio::shutdown();
    SDL_Quit();
    MemoryTracker::logStats();
    s_instance = nullptr;
}

Application& Application::getInstance() {
    return *s_instance;
}

bool Application::init(const std::string& gamePath) {
    // Startup banner
    std::cout << "\n\033[1;36m╔══════════════════════════════════════════════════════╗\033[0m" << std::endl;
    std::cout << "\033[1;36m║\033[0m   \033[1;33mTERRAM ENGINE\033[0m v0.2.0                              \033[1;36m║\033[0m" << std::endl;
    std::cout << "\033[1;36m║\033[0m   Memory tracking: \033[32mENABLED\033[0m                          \033[1;36m║\033[0m" << std::endl;
    std::cout << "\033[1;36m╚══════════════════════════════════════════════════════╝\033[0m\n" << std::endl;
    std::cout << "\033[1m[Memory Tracker] Monitoring allocations...\033[0m\n" << std::endl;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO) != 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize Audio
    Audio::init();

    // Create subsystems
    m_window = std::make_unique<Window>();
    if (!m_window->create("Terram", 800, 600)) {
        return false;
    }

    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer->init(m_window->getWidth(), m_window->getHeight())) {
        return false;
    }

    m_input = std::make_unique<Input>();

    // Initialize Lua
    m_lua = std::make_unique<LuaState>();
    if (!m_lua->init()) {
        return false;
    }

    // Load game
    std::string mainLua = gamePath + "/main.lua";
    if (!m_lua->loadFile(mainLua)) {
        std::cerr << "Failed to load: " << mainLua << std::endl;
        return false;
    }

    // Call terram.load()
    m_lua->callLoad();

    m_running = true;
    return true;
}

void Application::run() {
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (m_running) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        m_deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        handleEvents();
        update(m_deltaTime);
        render();
    }
}

void Application::quit() {
    m_running = false;
}

void Application::handleEvents() {
    m_input->update();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            m_running = false;
        }
        m_input->processEvent(event);
    }
}

void Application::update(float dt) {
    m_lua->callUpdate(dt);
}

void Application::render() {
    m_lua->callDraw();
    m_window->swap();
}

} // namespace terram
