#include "pal4/game_state_frame_scale.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameStateFrameScaleRuntime {
    std::vector<std::string> trace;
    std::uint32_t scale_bits = 0u;
    pal4::ActorManagerEffectHandle observed{};
    std::uint32_t reset_result = 0xbeefu;

    std::uint32_t GameState() override {
        trace.push_back("game-state");
        return 0x3000u;
    }
    void WriteFloat(std::uint32_t address, float value) override {
        Check(address == 0x3000u +
                  pal4::GameStateFrameScale::kScaleOffset,
              "frame scale state offset");
        trace.push_back("write-scale");
        scale_bits = std::bit_cast<std::uint32_t>(value);
    }
    std::uint32_t ResetEffectHandle(
        pal4::ActorManagerEffectHandle& handle) override {
        trace.push_back("reset-handle");
        observed = handle;
        handle = {0xffffffffu, 0u};
        return reset_result;
    }
};

}  // namespace

int main() {
    try {
        for (auto bits : {0u, 0x3f800000u, 0xbf400000u}) {
            Runtime runtime;
            const pal4::ActorManagerEffectHandle original{0x1234u, 0x5678u};
            Check(pal4::GameStateFrameScale::SetAndReset(
                      original, std::bit_cast<float>(bits),
                      runtime) == 0xbeefu &&
                      runtime.scale_bits == bits &&
                      runtime.observed.id == original.id &&
                      runtime.observed.owner == original.owner &&
                      original.id == 0x1234u &&
                      original.owner == 0x5678u &&
                      runtime.trace == std::vector<std::string>{
                          "game-state", "write-scale", "reset-handle"},
                  "scale write precedes by-value handle reset");
        }
        Runtime zero;
        zero.reset_result = 0u;
        Check(pal4::GameStateFrameScale::SetAndReset(
                  {0u, 0u}, 0.0f, zero) == 0u,
              "reset boundary result is forwarded");
        std::cout << "game_state_frame_scale_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
