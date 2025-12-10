#define GL_SILENCE_DEPRECATION
#define STB_TRUETYPE_IMPLEMENTATION
#include "Font.hpp"
#include <stb_truetype.h>
#include <OpenGL/gl3.h>
#include <fstream>
#include <iostream>
#include <vector>

namespace terram {

Font::~Font() {
    if (m_textureId) {
        glDeleteTextures(1, &m_textureId);
    }
}

Font::Font(Font&& other) noexcept
    : m_textureId(other.m_textureId)
    , m_size(other.m_size)
    , m_lineHeight(other.m_lineHeight)
    , m_glyphs(std::move(other.m_glyphs))
    , m_fontData(std::move(other.m_fontData)) {
    other.m_textureId = 0;
}

Font& Font::operator=(Font&& other) noexcept {
    if (this != &other) {
        if (m_textureId) glDeleteTextures(1, &m_textureId);
        m_textureId = other.m_textureId;
        m_size = other.m_size;
        m_lineHeight = other.m_lineHeight;
        m_glyphs = std::move(other.m_glyphs);
        m_fontData = std::move(other.m_fontData);
        other.m_textureId = 0;
    }
    return *this;
}

bool Font::load(const std::string& path, float size) {
    m_size = size;

    // Load font file
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open font: " << path << std::endl;
        return false;
    }

    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    m_fontData = std::make_unique<unsigned char[]>(fileSize);
    file.read(reinterpret_cast<char*>(m_fontData.get()), fileSize);

    // Initialize font
    stbtt_fontinfo fontInfo;
    if (!stbtt_InitFont(&fontInfo, m_fontData.get(), 0)) {
        std::cerr << "Failed to init font: " << path << std::endl;
        return false;
    }

    float scale = stbtt_ScaleForPixelHeight(&fontInfo, size);

    // Get font metrics
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
    m_lineHeight = (ascent - descent + lineGap) * scale;

    // Create texture atlas for ASCII characters
    const int atlasWidth = 512;
    const int atlasHeight = 512;
    std::vector<unsigned char> atlasData(atlasWidth * atlasHeight, 0);

    int x = 1, y = 1;
    int rowHeight = 0;

    // Render ASCII printable characters (32-126)
    for (int c = 32; c < 127; c++) {
        int w, h, xoff, yoff;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(&fontInfo, 0, scale, c, &w, &h, &xoff, &yoff);

        if (!bitmap) continue;

        // Check if we need new row
        if (x + w + 1 >= atlasWidth) {
            x = 1;
            y += rowHeight + 1;
            rowHeight = 0;
        }

        // Check if we ran out of space
        if (y + h + 1 >= atlasHeight) {
            stbtt_FreeBitmap(bitmap, nullptr);
            break;
        }

        // Copy bitmap to atlas
        for (int row = 0; row < h; row++) {
            for (int col = 0; col < w; col++) {
                atlasData[(y + row) * atlasWidth + (x + col)] = bitmap[row * w + col];
            }
        }

        // Get advance
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&fontInfo, c, &advance, &lsb);

        // Store glyph info
        GlyphInfo glyph;
        glyph.x0 = static_cast<float>(x) / atlasWidth;
        glyph.y0 = static_cast<float>(y) / atlasHeight;
        glyph.x1 = static_cast<float>(x + w) / atlasWidth;
        glyph.y1 = static_cast<float>(y + h) / atlasHeight;
        glyph.xoff = static_cast<float>(xoff);
        glyph.yoff = static_cast<float>(yoff);
        glyph.xadvance = advance * scale;
        glyph.width = w;
        glyph.height = h;
        m_glyphs[static_cast<char>(c)] = glyph;

        x += w + 1;
        if (h > rowHeight) rowHeight = h;

        stbtt_FreeBitmap(bitmap, nullptr);
    }

    // Create OpenGL texture
    glGenTextures(1, &m_textureId);
    glBindTexture(GL_TEXTURE_2D, m_textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasWidth, atlasHeight, 0,
                 GL_RED, GL_UNSIGNED_BYTE, atlasData.data());

    return true;
}

const Font::GlyphInfo* Font::getGlyph(char c) const {
    auto it = m_glyphs.find(c);
    if (it != m_glyphs.end()) {
        return &it->second;
    }
    return nullptr;
}

} // namespace terram
