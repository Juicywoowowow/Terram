#pragma once

#include <string>
#include <memory>

namespace terram {

class Sound {
public:
    Sound() = default;
    ~Sound();

    Sound(const Sound&) = delete;
    Sound& operator=(const Sound&) = delete;

    Sound(Sound&& other) noexcept;
    Sound& operator=(Sound&& other) noexcept;

    bool load(const std::string& path);

    void play(int loops = 0);  // 0 = play once, -1 = loop forever
    void stop();

    void setVolume(float volume);  // 0.0 - 1.0

    void* getChunk() const { return m_chunk; }

private:
    void* m_chunk = nullptr;
    int m_channel = -1;
};

class Music {
public:
    Music() = default;
    ~Music();

    Music(const Music&) = delete;
    Music& operator=(const Music&) = delete;

    Music(Music&& other) noexcept;
    Music& operator=(Music&& other) noexcept;

    bool load(const std::string& path);

    void play(int loops = -1);  // -1 = loop forever
    static void stop();
    static void pause();
    static void resume();

    void setVolume(float volume);

    static bool isPlaying();

private:
    void* m_music = nullptr;
};

class Audio {
public:
    static bool init();
    static void shutdown();

    static void setMasterVolume(float volume);

private:
    static bool s_initialized;
};

} // namespace terram
