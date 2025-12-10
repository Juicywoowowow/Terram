#pragma once

#include <string>
#include <memory>

namespace terram {

class Texture {
public:
    Texture() = default;
    ~Texture();

    // Non-copyable
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Movable
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool load(const std::string& path);
    void bind() const;

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    unsigned int getId() const { return m_id; }

private:
    unsigned int m_id = 0;
    int m_width = 0;
    int m_height = 0;
};

} // namespace terram
