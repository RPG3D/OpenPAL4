#include "pal4/audio_playback_handle.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

struct Runtime final : pal4::IAudioPlaybackHandleRuntime {
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;

    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        writes.emplace_back(address, value);
    }
};

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

}  // namespace

int main() {
    try {
        Runtime runtime;
        pal4::AudioPlaybackHandle::Reset(runtime);
        pal4::AudioPlaybackHandle::ResetThunk(runtime);
        Check(runtime.writes ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x8edf80u, 0u}, {0x8edf80u, 0u}}),
              "direct and thunk entries both clear the audio handle");
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
