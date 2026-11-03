#include "pal4/combat_result_audio.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatResultAudioRuntime {
    std::vector<std::string> calls;
    std::uint32_t handle = 0x1357u;
    std::uint32_t saved_handle = 0u;
    std::uint32_t stopped_handle = 0u;

    std::uint32_t PlaySound(std::string_view name,
                            std::uint32_t mode) override {
        calls.emplace_back("play:" + std::string(name) + ":" +
                           std::to_string(mode));
        return handle;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x405cu, "handle destination field");
        calls.emplace_back("read");
        return 0x7000u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x7000u, "handle destination pointer");
        calls.emplace_back("write");
        saved_handle = value;
    }
    void StopStoredHandle(std::uint32_t value) override {
        calls.emplace_back("stop-handle");
        stopped_handle = value;
    }
};

void SoundPaths() {
    Runtime runtime;
    Check(pal4::CombatResultAudio::PlayTransition(runtime) == 1u &&
              runtime.calls ==
                  std::vector<std::string>{"play:WA45.wav:1"},
          "WA45 plays once in mode 1");
    runtime.calls.clear();
    Check(pal4::CombatResultAudio::PlayReward(0x4000u, runtime) == 1u &&
              runtime.saved_handle == runtime.handle &&
              runtime.calls == std::vector<std::string>{
                                   "play:WA47.wav:0", "read", "write"},
          "WA47 saves returned handle through element pointer");
    runtime.calls.clear();
    Check(pal4::CombatResultAudio::StopStored(0x4000u, runtime) == 1u &&
              runtime.stopped_handle == 0x7000u &&
              runtime.calls ==
                  std::vector<std::string>{"read", "stop-handle"},
          "stored handle address at element offset 92 is stopped");
}

}  // namespace

int main() {
    try {
        SoundPaths();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
