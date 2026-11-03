#include "pal4/thunder_updater.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IThunderUpdateRuntime {
    std::vector<std::string> events;
    std::array<std::uint32_t, 4u> playback{0x1122u, 0x3344u, 9u, 0x5566u};
    std::uint8_t expected_remaining = 6u;
    double expected_elapsed = 2.0;

    void ReadPlayback(pal4::ThunderObjectState&,
                      std::array<std::uint32_t, 4u>& outputs) override {
        Check(outputs == std::array<std::uint32_t, 4u>{},
              "playback output words initially zero");
        events.emplace_back("read");
        outputs = playback;
    }
    std::int32_t FloatToInt(const double value) override {
        Check(value == static_cast<double>(
                  static_cast<std::int32_t>(playback[2u])) -
                  1.5 * expected_elapsed,
              "native float-to-int receives frame remainder");
        events.emplace_back("ftol");
        return static_cast<std::int32_t>(value);
    }
    std::uint8_t StackByte() override {
        events.emplace_back("stack");
        return 0xaau;
    }
    void WritePlayback(pal4::ThunderObjectState&,
                       const std::array<std::uint8_t, 4u>& bytes) override {
        Check(bytes == std::array<std::uint8_t, 4u>{
                  0xaau, 0x22u, 0x44u, expected_remaining},
              "slot +12 receives stack byte and reordered low bytes");
        events.emplace_back("write");
    }
    void ResolveTarget(pal4::ThunderObjectState&,
                       const std::uint32_t owner,
                       std::array<std::uint32_t, 3u>& output) override {
        Check(owner == 0x7000u &&
                  output == std::array<std::uint32_t, 3u>{},
              "target lookup starts from zero triple");
        events.emplace_back("target");
        output = {0x11u, 0x22u, 0x33u};
    }
    std::uint32_t AdvanceVisual(pal4::ThunderObjectState& state,
                                const double elapsed) override {
        Check(elapsed == expected_elapsed, "visual receives elapsed double");
        events.emplace_back("advance");
        if ((state.words[58u] & 0xffu) == 0u && state.words[3u] != 0u)
            Check(state.words[37u] == 0x11u &&
                      state.words[38u] == 0x22u &&
                      state.words[39u] == 0x33u,
                  "target copied before visual update");
        return 0x9876u;
    }
};

void TestGates() {
    pal4::ThunderObjectState state{};
    Runtime runtime;
    Check(pal4::ThunderUpdater::Update(state, 2.0, 0x1234u, runtime) ==
              0x1234u && runtime.events.empty(),
          "inactive path preserves incoming EAX");
    state.words[1u] = 1u;
    Check(pal4::ThunderUpdater::Update(state, 2.0, 0x5678u, runtime) ==
              0x5678u && runtime.events.empty(),
          "unstarted path preserves incoming EAX");
}

void TestPlaybackAndTarget() {
    pal4::ThunderObjectState state{};
    state.words[1u] = 0x00030002u;
    state.words[2u] = 0x01000000u;
    state.words[3u] = 0x7000u;
    state.words[59u] = std::bit_cast<std::uint32_t>(1.5f);
    Runtime runtime;
    Check(pal4::ThunderUpdater::Update(state, 2.0, 0u, runtime) == 0x9876u &&
              runtime.events == std::vector<std::string>{
                  "read", "ftol", "stack", "write", "target", "advance"},
          "active playback updates bytes, target and visual in IDA order");
    Check(state.words[70u] == 0u && state.words[71u] == 0u,
          "frame update does not overwrite cached target");

    runtime.events.clear();
    runtime.playback[2u] = 1u;
    runtime.expected_remaining = 0u;
    runtime.expected_elapsed = 10.0;
    Check(pal4::ThunderUpdater::Update(state, 10.0, 0u, runtime) == 0x9876u &&
              runtime.events.size() == 6u,
          "negative frame remainder clamps to zero");
}

void TestLocalWithoutPlayback() {
    pal4::ThunderObjectState state{};
    state.words[1u] = 0x00010001u;
    state.words[3u] = 0x7000u;
    state.words[58u] = 1u;
    state.words[37u] = 0xa1u;
    Runtime runtime;
    Check(pal4::ThunderUpdater::Update(state, 2.0, 0u, runtime) == 0x9876u &&
              runtime.events == std::vector<std::string>{"advance"} &&
              state.words[37u] == 0xa1u,
          "local model and disabled playback skip optional work");
}

}  // namespace

int main() {
    TestGates();
    TestPlaybackAndTarget();
    TestLocalWithoutPlayback();
}
