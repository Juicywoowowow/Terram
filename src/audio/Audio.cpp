#include "Audio.hpp"
#include <SDL_mixer.h>
#include <iostream>

namespace terram {

bool Audio::s_initialized = false;

// ============================================================================
// Audio
// ============================================================================

bool Audio::init() {
    if (s_initialized) return true;

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer init failed: " << Mix_GetError() << std::endl;
        return false;
    }

    // Load support for OGG and MP3
    int flags = MIX_INIT_OGG | MIX_INIT_MP3;
    if ((Mix_Init(flags) & flags) != flags) {
        std::cerr << "SDL_mixer format init warning: " << Mix_GetError() << std::endl;
    }

    s_initialized = true;
    std::cout << "[Audio] Initialized (SDL_mixer)" << std::endl;
    return true;
}

void Audio::shutdown() {
    if (s_initialized) {
        Mix_CloseAudio();
        Mix_Quit();
        s_initialized = false;
    }
}

void Audio::setMasterVolume(float volume) {
    int v = static_cast<int>(volume * MIX_MAX_VOLUME);
    Mix_Volume(-1, v);
    Mix_VolumeMusic(v);
}

// ============================================================================
// Sound (short effects)
// ============================================================================

Sound::~Sound() {
    if (m_chunk) {
        Mix_FreeChunk(static_cast<Mix_Chunk*>(m_chunk));
    }
}

Sound::Sound(Sound&& other) noexcept
    : m_chunk(other.m_chunk), m_channel(other.m_channel) {
    other.m_chunk = nullptr;
    other.m_channel = -1;
}

Sound& Sound::operator=(Sound&& other) noexcept {
    if (this != &other) {
        if (m_chunk) Mix_FreeChunk(static_cast<Mix_Chunk*>(m_chunk));
        m_chunk = other.m_chunk;
        m_channel = other.m_channel;
        other.m_chunk = nullptr;
        other.m_channel = -1;
    }
    return *this;
}

bool Sound::load(const std::string& path) {
    m_chunk = Mix_LoadWAV(path.c_str());
    if (!m_chunk) {
        std::cerr << "Failed to load sound: " << path << " - " << Mix_GetError() << std::endl;
        return false;
    }
    return true;
}

void Sound::play(int loops) {
    if (m_chunk) {
        m_channel = Mix_PlayChannel(-1, static_cast<Mix_Chunk*>(m_chunk), loops);
    }
}

void Sound::stop() {
    if (m_channel >= 0) {
        Mix_HaltChannel(m_channel);
        m_channel = -1;
    }
}

void Sound::setVolume(float volume) {
    if (m_chunk) {
        Mix_VolumeChunk(static_cast<Mix_Chunk*>(m_chunk), static_cast<int>(volume * MIX_MAX_VOLUME));
    }
}

// ============================================================================
// Music (streaming)
// ============================================================================

Music::~Music() {
    if (m_music) {
        Mix_FreeMusic(static_cast<Mix_Music*>(m_music));
    }
}

Music::Music(Music&& other) noexcept : m_music(other.m_music) {
    other.m_music = nullptr;
}

Music& Music::operator=(Music&& other) noexcept {
    if (this != &other) {
        if (m_music) Mix_FreeMusic(static_cast<Mix_Music*>(m_music));
        m_music = other.m_music;
        other.m_music = nullptr;
    }
    return *this;
}

bool Music::load(const std::string& path) {
    m_music = Mix_LoadMUS(path.c_str());
    if (!m_music) {
        std::cerr << "Failed to load music: " << path << " - " << Mix_GetError() << std::endl;
        return false;
    }
    return true;
}

void Music::play(int loops) {
    if (m_music) {
        Mix_PlayMusic(static_cast<Mix_Music*>(m_music), loops);
    }
}

void Music::stop() {
    Mix_HaltMusic();
}

void Music::pause() {
    Mix_PauseMusic();
}

void Music::resume() {
    Mix_ResumeMusic();
}

void Music::setVolume(float volume) {
    Mix_VolumeMusic(static_cast<int>(volume * MIX_MAX_VOLUME));
}

bool Music::isPlaying() {
    return Mix_PlayingMusic() != 0;
}

} // namespace terram
