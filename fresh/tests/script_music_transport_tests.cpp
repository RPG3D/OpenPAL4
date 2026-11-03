#include "pal4/script_music_transport.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Runtime final : pal4::IScriptMusicTransportRuntime {
    std::vector<std::string> calls;
    std::uint32_t player = 0x4000u;

    std::uint32_t BackgroundMusicPlayer() override {
        calls.emplace_back("player");
        return player;
    }
    std::uint8_t StopSound(std::uint32_t sound_slot) override {
        if (sound_slot != player + 0x30u)
            throw std::runtime_error("music pause sound-slot offset");
        calls.emplace_back("stop");
        return 0xf1u;
    }
    std::uint8_t LoadAndPlay(std::uint32_t music_player) override {
        if (music_player != player)
            throw std::runtime_error("music resume player receiver");
        calls.emplace_back("play");
        return 0x89u;
    }
};

}  // namespace

int main() {
    try {
        Runtime pause;
        if (pal4::ScriptMusicTransport::Pause(pause) != 0xf1u ||
            pause.calls != std::vector<std::string>{"player", "stop"})
            throw std::runtime_error("pause order or byte return");
        Runtime resume;
        if (pal4::ScriptMusicTransport::Resume(resume) != 0x89u ||
            resume.calls != std::vector<std::string>{"player", "play"})
            throw std::runtime_error("resume order or byte return");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
