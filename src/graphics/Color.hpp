#pragma once

namespace terram {

struct Color {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    Color() = default;
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

    static Color white() { return {1, 1, 1, 1}; }
    static Color black() { return {0, 0, 0, 1}; }
    static Color red() { return {1, 0, 0, 1}; }
    static Color green() { return {0, 1, 0, 1}; }
    static Color blue() { return {0, 0, 1, 1}; }
};

} // namespace terram
