#pragma once

#include <string>
#include <memory>
#include <unordered_map>

namespace terram {

class Font {
public:
    Font() = default;
    ~Font();

    // Non-copyable
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    // Movable
    Font(Font&& other) noexcept;
    Font& operator=(Font&& other) noexcept;

    bool load(const std::string& path, float size);

    unsigned int getTextureId() const { return m_textureId; }
    float getSize() const { return m_size; }

    // Get glyph metrics
    struct GlyphInfo {
        float x0, y0, x1, y1;  // texture coords
        float xoff, yoff;      // offset from cursor
        float xadvance;        // advance to next character
        int width, height;     // size in pixels
    };

    const GlyphInfo* getGlyph(char c) const;
    float getLineHeight() const { return m_lineHeight; }

private:
    unsigned int m_textureId = 0;
    float m_size = 24.0f;
    float m_lineHeight = 0;

    std::unordered_map<char, GlyphInfo> m_glyphs;
    std::unique_ptr<unsigned char[]> m_fontData;
};

} // namespace terram
