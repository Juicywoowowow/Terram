#pragma once

#include <memory>
#include <string>

namespace terram {

class Window;
class Renderer;
class Input;
class LuaState;

class Application {
public:
    Application();
    ~Application();

    // Non-copyable
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool init(const std::string& gamePath);
    void run();
    void quit();

    Window& getWindow() { return *m_window; }
    Renderer& getRenderer() { return *m_renderer; }
    Input& getInput() { return *m_input; }

    static Application& getInstance();

private:
    void handleEvents();
    void update(float dt);
    void render();

    std::unique_ptr<Window> m_window;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<Input> m_input;
    std::unique_ptr<LuaState> m_lua;

    bool m_running = false;
    float m_deltaTime = 0.0f;

    static Application* s_instance;
};

} // namespace terram
