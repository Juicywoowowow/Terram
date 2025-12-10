#include "Application.hpp"
#include "Window.hpp"
#include "graphics/Renderer.hpp"
#include "input/Input.hpp"
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
    m_lua.reset();
    m_renderer.reset();
    m_input.reset();
    m_window.reset();
    SDL_Quit();
    s_instance = nullptr;
}

Application& Application::getInstance() {
    return *s_instance;
}

bool Application::init(const std::string& gamePath) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }

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
