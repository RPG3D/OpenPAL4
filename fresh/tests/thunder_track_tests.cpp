#include "pal4/thunder_track.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IThunderTrackRuntime {
    std::uint32_t calls = 0u;
    std::array<std::uint32_t, 3u> observed{};

    std::uint32_t UpdateFrame(pal4::ThunderObjectState& state) override {
        ++calls;
        for (std::uint32_t i = 0u; i < 3u; ++i)
            observed[i] = state.words[24u + i];
        return 0x12345678u;
    }
};

}  // namespace

int main() {
    pal4::ThunderObjectState state{};
    Runtime runtime;
    state.words[24u] = 11u;
    state.words[37u] = 22u;
    Check(pal4::ThunderTrack::SetTargetPosition(
              state, {1u, 2u, 3u}, 0xabcd12ef, runtime) == 0xabcd1200u &&
              state.words[24u] == 11u && state.words[37u] == 22u &&
              runtime.calls == 0u,
          "inactive track retains state and entry EAX upper bytes");

    state.words[1u] = 1u;
    state.words[14u] = 0xaabbcc01u;
    Check(pal4::ThunderTrack::SetTargetPosition(
              state, {4u, 0x1234u, 6u}, 0xdeadbeefu, runtime) ==
              0xdeadbe34u &&
              state.words[37u] == 4u && state.words[38u] == 0x1234u &&
              state.words[39u] == 6u && state.words[24u] == 11u &&
              runtime.calls == 0u,
          "tracking branch stores target cache and returns middle low byte");

    state.words[14u] = 0xaabbcc00u;
    Check(pal4::ThunderTrack::SetTargetPosition(
              state, {7u, 8u, 9u}, 0u, runtime) == 0x12345678u &&
              runtime.calls == 1u &&
              runtime.observed == std::array<std::uint32_t, 3u>{7u, 8u, 9u} &&
              state.words[37u] == 4u,
          "nontracking branch writes current position and refreshes frame");
}
