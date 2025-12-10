#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#include "Texture.hpp"
#include <stb_image.h>
#include <OpenGL/gl3.h>
#include <iostream>

namespace terram {

Texture::~Texture() {
    if (m_id) {
        glDeleteTextures(1, &m_id);
    }
}

Texture::Texture(Texture&& other) noexcept
    : m_id(other.m_id), m_width(other.m_width), m_height(other.m_height) {
    other.m_id = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (m_id) glDeleteTextures(1, &m_id);
        m_id = other.m_id;
        m_width = other.m_width;
        m_height = other.m_height;
        other.m_id = 0;
    }
    return *this;
}

bool Texture::load(const std::string& path) {
    stbi_set_flip_vertically_on_load(true);

    int channels;
    unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &channels, 4);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return false;
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    return true;
}

void Texture::bind() const {
    glBindTexture(GL_TEXTURE_2D, m_id);
}

} // namespace terram
