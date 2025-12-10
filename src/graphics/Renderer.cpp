#define GL_SILENCE_DEPRECATION
#include "Renderer.hpp"
#include "Texture.hpp"
#include "Font.hpp"
#include <OpenGL/gl3.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <string>

namespace terram {

static const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
uniform mat4 projection;
uniform mat4 model;
out vec2 TexCoord;
void main() {
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

static const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform vec4 color;
uniform sampler2D tex;
uniform bool useTexture;
void main() {
    if (useTexture) {
        FragColor = texture(tex, TexCoord) * color;
    } else {
        FragColor = color;
    }
}
)";

// Text shader uses single-channel (RED) texture
static const char* textFragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform vec4 color;
uniform sampler2D tex;
void main() {
    float alpha = texture(tex, TexCoord).r;
    FragColor = vec4(color.rgb, color.a * alpha);
}
)";

Renderer::~Renderer() {
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
    if (m_textShaderProgram) glDeleteProgram(m_textShaderProgram);
}

bool Renderer::init(int width, int height) {
    m_width = width;
    m_height = height;

    initShaders();
    initBuffers();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    setViewport(width, height);
    return true;
}

void Renderer::initShaders() {
    // Compile vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    // Compile fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    // Link main program
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);

    glDeleteShader(fragmentShader);

    // Compile text fragment shader
    unsigned int textFragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(textFragShader, 1, &textFragmentShaderSource, nullptr);
    glCompileShader(textFragShader);

    // Link text program
    m_textShaderProgram = glCreateProgram();
    glAttachShader(m_textShaderProgram, vertexShader);
    glAttachShader(m_textShaderProgram, textFragShader);
    glLinkProgram(m_textShaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(textFragShader);

    glUseProgram(m_shaderProgram);
}

void Renderer::initBuffers() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void Renderer::setViewport(int width, int height) {
    m_width = width;
    m_height = height;
    glViewport(0, 0, width, height);

    // Set orthographic projection for both shaders
    float projection[16] = {
        2.0f / width, 0, 0, 0,
        0, -2.0f / height, 0, 0,
        0, 0, -1, 0,
        -1, 1, 0, 1
    };

    glUseProgram(m_shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, projection);

    glUseProgram(m_textShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(m_textShaderProgram, "projection"), 1, GL_FALSE, projection);
}

void Renderer::clear(const Color& color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::setColor(const Color& color) {
    m_currentColor = color;
}

void Renderer::rectangle(const char* mode, float x, float y, float w, float h) {
    glUseProgram(m_shaderProgram);
    glBindVertexArray(m_vao);

    // Set identity model matrix
    float model[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, model);

    // Set color
    glUniform4f(glGetUniformLocation(m_shaderProgram, "color"),
                m_currentColor.r, m_currentColor.g, m_currentColor.b, m_currentColor.a);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), 0);

    float vertices[] = {
        x,     y,      0, 0,
        x + w, y,      1, 0,
        x + w, y + h,  1, 1,
        x,     y,      0, 0,
        x + w, y + h,  1, 1,
        x,     y + h,  0, 1
    };

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    bool fill = (mode[0] == 'f');
    if (fill) {
        glDrawArrays(GL_TRIANGLES, 0, 6);
    } else {
        float lineVerts[] = {
            x, y, 0, 0,
            x + w, y, 0, 0,
            x + w, y + h, 0, 0,
            x, y + h, 0, 0,
            x, y, 0, 0
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(lineVerts), lineVerts, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINE_STRIP, 0, 5);
    }
}

void Renderer::circle(const char* mode, float x, float y, float radius, int segments) {
    glUseProgram(m_shaderProgram);
    glBindVertexArray(m_vao);

    float model[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, model);
    glUniform4f(glGetUniformLocation(m_shaderProgram, "color"),
                m_currentColor.r, m_currentColor.g, m_currentColor.b, m_currentColor.a);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), 0);

    bool fill = (mode[0] == 'f');

    std::vector<float> vertices;
    if (fill) {
        for (int i = 0; i < segments; i++) {
            float angle1 = 2.0f * M_PI * i / segments;
            float angle2 = 2.0f * M_PI * (i + 1) / segments;

            vertices.insert(vertices.end(), {x, y, 0, 0});
            vertices.insert(vertices.end(), {x + radius * cosf(angle1), y + radius * sinf(angle1), 0, 0});
            vertices.insert(vertices.end(), {x + radius * cosf(angle2), y + radius * sinf(angle2), 0, 0});
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, segments * 3);
    } else {
        for (int i = 0; i <= segments; i++) {
            float angle = 2.0f * M_PI * i / segments;
            vertices.insert(vertices.end(), {x + radius * cosf(angle), y + radius * sinf(angle), 0, 0});
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINE_STRIP, 0, segments + 1);
    }
}

void Renderer::line(float x1, float y1, float x2, float y2) {
    glUseProgram(m_shaderProgram);
    glBindVertexArray(m_vao);

    float model[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, model);
    glUniform4f(glGetUniformLocation(m_shaderProgram, "color"),
                m_currentColor.r, m_currentColor.g, m_currentColor.b, m_currentColor.a);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), 0);

    float vertices[] = {x1, y1, 0, 0, x2, y2, 0, 0};
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINES, 0, 2);
}

void Renderer::draw(const Texture& texture, float x, float y, float rotation, float sx, float sy) {
    glUseProgram(m_shaderProgram);
    glBindVertexArray(m_vao);

    float w = texture.getWidth() * sx;
    float h = texture.getHeight() * sy;

    // Build model matrix with rotation
    float c = cosf(rotation);
    float s = sinf(rotation);
    float cx = x + w / 2;
    float cy = y + h / 2;

    float model[16] = {
        c, s, 0, 0,
        -s, c, 0, 0,
        0, 0, 1, 0,
        cx - c * cx + s * cy, cy - s * cx - c * cy, 0, 1
    };
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, model);
    glUniform4f(glGetUniformLocation(m_shaderProgram, "color"),
                m_currentColor.r, m_currentColor.g, m_currentColor.b, m_currentColor.a);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "useTexture"), 1);

    texture.bind();

    float vertices[] = {
        x,     y,      0, 0,
        x + w, y,      1, 0,
        x + w, y + h,  1, 1,
        x,     y,      0, 0,
        x + w, y + h,  1, 1,
        x,     y + h,  0, 1
    };

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::print(const Font& font, const std::string& text, float x, float y) {
    glUseProgram(m_textShaderProgram);
    glBindVertexArray(m_vao);

    float model[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    glUniformMatrix4fv(glGetUniformLocation(m_textShaderProgram, "model"), 1, GL_FALSE, model);
    glUniform4f(glGetUniformLocation(m_textShaderProgram, "color"),
                m_currentColor.r, m_currentColor.g, m_currentColor.b, m_currentColor.a);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font.getTextureId());

    float cursorX = x;
    float cursorY = y + font.getSize();  // Baseline offset

    std::vector<float> vertices;

    for (char c : text) {
        const Font::GlyphInfo* glyph = font.getGlyph(c);
        if (!glyph) {
            if (c == ' ') cursorX += font.getSize() * 0.3f;
            continue;
        }

        float gx = cursorX + glyph->xoff;
        float gy = cursorY + glyph->yoff;
        float gw = static_cast<float>(glyph->width);
        float gh = static_cast<float>(glyph->height);

        // Two triangles per glyph
        vertices.insert(vertices.end(), {
            gx,      gy,      glyph->x0, glyph->y0,
            gx + gw, gy,      glyph->x1, glyph->y0,
            gx + gw, gy + gh, glyph->x1, glyph->y1,
            gx,      gy,      glyph->x0, glyph->y0,
            gx + gw, gy + gh, glyph->x1, glyph->y1,
            gx,      gy + gh, glyph->x0, glyph->y1
        });

        cursorX += glyph->xadvance;
    }

    if (!vertices.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(vertices.size()) / 4);
    }
}

} // namespace terram

