#pragma once

#include <memory>
#include <string>
#include <SDL.h>

namespace terram {

class Window {
public:
    Window() = default;
    ~Window() = default;

    // Non-copyable, movable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = default;
    Window& operator=(Window&&) = default;

    bool create(const std::string& title, int width, int height);
    void swap();
    void close();

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    void setTitle(const std::string& title);

    SDL_Window* getSDLWindow() const { return m_window.get(); }
    bool isOpen() const { return m_window != nullptr; }

private:
    struct SDLWindowDeleter {
        void operator()(SDL_Window* w) { if (w) SDL_DestroyWindow(w); }
    };
    struct SDLGLContextDeleter {
        void operator()(SDL_GLContext ctx) { if (ctx) SDL_GL_DeleteContext(ctx); }
    };

    std::unique_ptr<SDL_Window, SDLWindowDeleter> m_window;
    std::unique_ptr<std::remove_pointer_t<SDL_GLContext>, SDLGLContextDeleter> m_glContext;
    int m_width = 800;
    int m_height = 600;
};

} // namespace terram
