#include "Window.hpp"
#include <iostream>

namespace terram {

bool Window::create(const std::string& title, int width, int height) {
    m_width = width;
    m_height = height;

    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Create window
    SDL_Window* window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return false;
    }

    m_window.reset(window);

    // Create OpenGL context
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "Failed to create GL context: " << SDL_GetError() << std::endl;
        m_window.reset();
        return false;
    }

    m_glContext.reset(glContext);

    // Enable VSync
    SDL_GL_SetSwapInterval(1);

    return true;
}

void Window::swap() {
    if (m_window) {
        SDL_GL_SwapWindow(m_window.get());
    }
}

void Window::close() {
    m_glContext.reset();
    m_window.reset();
}

void Window::setTitle(const std::string& title) {
    if (m_window) {
        SDL_SetWindowTitle(m_window.get(), title.c_str());
    }
}

} // namespace terram
