#include "Input.hpp"
#include <algorithm>

namespace terram {

void Input::update() {
    m_previousKeys = m_currentKeys;
    m_previousMouse = m_currentMouse;

    // Update mouse position
    m_currentMouse = SDL_GetMouseState(&m_mouseX, &m_mouseY);

    // Update keyboard state
    const Uint8* state = SDL_GetKeyboardState(nullptr);
    for (int i = 0; i < SDL_NUM_SCANCODES; i++) {
        m_currentKeys[static_cast<SDL_Scancode>(i)] = state[i];
    }
}

void Input::processEvent(const SDL_Event& event) {
    // Additional event processing if needed
}

bool Input::isKeyDown(const std::string& key) const {
    SDL_Scancode sc = keyNameToScancode(key);
    auto it = m_currentKeys.find(sc);
    return it != m_currentKeys.end() && it->second;
}

bool Input::isKeyPressed(const std::string& key) const {
    SDL_Scancode sc = keyNameToScancode(key);
    auto curr = m_currentKeys.find(sc);
    auto prev = m_previousKeys.find(sc);
    bool currDown = curr != m_currentKeys.end() && curr->second;
    bool prevDown = prev != m_previousKeys.end() && prev->second;
    return currDown && !prevDown;
}

bool Input::isKeyReleased(const std::string& key) const {
    SDL_Scancode sc = keyNameToScancode(key);
    auto curr = m_currentKeys.find(sc);
    auto prev = m_previousKeys.find(sc);
    bool currDown = curr != m_currentKeys.end() && curr->second;
    bool prevDown = prev != m_previousKeys.end() && prev->second;
    return !currDown && prevDown;
}

bool Input::isMouseDown(int button) const {
    return m_currentMouse & SDL_BUTTON(button);
}

bool Input::isMousePressed(int button) const {
    return (m_currentMouse & SDL_BUTTON(button)) && !(m_previousMouse & SDL_BUTTON(button));
}

SDL_Scancode Input::keyNameToScancode(const std::string& name) const {
    // Common key mappings
    if (name == "a" || name == "A") return SDL_SCANCODE_A;
    if (name == "b" || name == "B") return SDL_SCANCODE_B;
    if (name == "c" || name == "C") return SDL_SCANCODE_C;
    if (name == "d" || name == "D") return SDL_SCANCODE_D;
    if (name == "e" || name == "E") return SDL_SCANCODE_E;
    if (name == "f" || name == "F") return SDL_SCANCODE_F;
    if (name == "g" || name == "G") return SDL_SCANCODE_G;
    if (name == "h" || name == "H") return SDL_SCANCODE_H;
    if (name == "i" || name == "I") return SDL_SCANCODE_I;
    if (name == "j" || name == "J") return SDL_SCANCODE_J;
    if (name == "k" || name == "K") return SDL_SCANCODE_K;
    if (name == "l" || name == "L") return SDL_SCANCODE_L;
    if (name == "m" || name == "M") return SDL_SCANCODE_M;
    if (name == "n" || name == "N") return SDL_SCANCODE_N;
    if (name == "o" || name == "O") return SDL_SCANCODE_O;
    if (name == "p" || name == "P") return SDL_SCANCODE_P;
    if (name == "q" || name == "Q") return SDL_SCANCODE_Q;
    if (name == "r" || name == "R") return SDL_SCANCODE_R;
    if (name == "s" || name == "S") return SDL_SCANCODE_S;
    if (name == "t" || name == "T") return SDL_SCANCODE_T;
    if (name == "u" || name == "U") return SDL_SCANCODE_U;
    if (name == "v" || name == "V") return SDL_SCANCODE_V;
    if (name == "w" || name == "W") return SDL_SCANCODE_W;
    if (name == "x" || name == "X") return SDL_SCANCODE_X;
    if (name == "y" || name == "Y") return SDL_SCANCODE_Y;
    if (name == "z" || name == "Z") return SDL_SCANCODE_Z;

    if (name == "0") return SDL_SCANCODE_0;
    if (name == "1") return SDL_SCANCODE_1;
    if (name == "2") return SDL_SCANCODE_2;
    if (name == "3") return SDL_SCANCODE_3;
    if (name == "4") return SDL_SCANCODE_4;
    if (name == "5") return SDL_SCANCODE_5;
    if (name == "6") return SDL_SCANCODE_6;
    if (name == "7") return SDL_SCANCODE_7;
    if (name == "8") return SDL_SCANCODE_8;
    if (name == "9") return SDL_SCANCODE_9;

    if (name == "space") return SDL_SCANCODE_SPACE;
    if (name == "return" || name == "enter") return SDL_SCANCODE_RETURN;
    if (name == "escape") return SDL_SCANCODE_ESCAPE;
    if (name == "tab") return SDL_SCANCODE_TAB;
    if (name == "backspace") return SDL_SCANCODE_BACKSPACE;

    if (name == "up") return SDL_SCANCODE_UP;
    if (name == "down") return SDL_SCANCODE_DOWN;
    if (name == "left") return SDL_SCANCODE_LEFT;
    if (name == "right") return SDL_SCANCODE_RIGHT;

    if (name == "lshift") return SDL_SCANCODE_LSHIFT;
    if (name == "rshift") return SDL_SCANCODE_RSHIFT;
    if (name == "lctrl") return SDL_SCANCODE_LCTRL;
    if (name == "rctrl") return SDL_SCANCODE_RCTRL;
    if (name == "lalt") return SDL_SCANCODE_LALT;
    if (name == "ralt") return SDL_SCANCODE_RALT;

    return SDL_SCANCODE_UNKNOWN;
}

} // namespace terram
