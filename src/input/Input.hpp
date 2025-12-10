#pragma once

#include <SDL.h>
#include <unordered_map>
#include <string>

namespace terram {

class Input {
public:
    Input() = default;

    void update();
    void processEvent(const SDL_Event& event);

    // Keyboard
    bool isKeyDown(const std::string& key) const;
    bool isKeyPressed(const std::string& key) const;
    bool isKeyReleased(const std::string& key) const;

    // Mouse
    int getMouseX() const { return m_mouseX; }
    int getMouseY() const { return m_mouseY; }
    bool isMouseDown(int button) const;
    bool isMousePressed(int button) const;

private:
    SDL_Scancode keyNameToScancode(const std::string& name) const;

    std::unordered_map<SDL_Scancode, bool> m_currentKeys;
    std::unordered_map<SDL_Scancode, bool> m_previousKeys;

    int m_mouseX = 0;
    int m_mouseY = 0;
    uint32_t m_currentMouse = 0;
    uint32_t m_previousMouse = 0;
};

} // namespace terram
