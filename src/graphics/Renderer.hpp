#pragma once

#include "Color.hpp"
#include <memory>

namespace terram {

class Texture;
class Font;

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    bool init(int width, int height);
    void setViewport(int width, int height);

    // Drawing
    void clear(const Color& color);
    void setColor(const Color& color);

    void rectangle(const char* mode, float x, float y, float w, float h);
    void circle(const char* mode, float x, float y, float radius, int segments = 32);
    void line(float x1, float y1, float x2, float y2);

    void draw(const Texture& texture, float x, float y,
              float rotation = 0, float sx = 1, float sy = 1);

    // Text rendering
    void print(const Font& font, const std::string& text, float x, float y);

private:
    void initShaders();
    void initBuffers();

    unsigned int m_shaderProgram = 0;
    unsigned int m_textShaderProgram = 0;
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;

    int m_width = 800;
    int m_height = 600;
    Color m_currentColor = Color::white();
};

} // namespace terram

