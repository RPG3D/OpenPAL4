#include "pal4/game_object_frame_effects.h"

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

struct Runtime final : pal4::IGameObjectFrameEffectRuntime {
    std::vector<std::string> trace;
    std::int32_t transform_result = -17;
    std::uint32_t draw_result = 0x1abu;
    double received_elapsed = 0.0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x7000u, "draw first argument address");
        trace.push_back("read-first");
        return 0x7788u;
    }
    std::int32_t RenderTransform(double elapsed) override {
        trace.push_back("render-transform");
        received_elapsed = elapsed;
        return transform_result;
    }
    std::uint32_t DrawEffect(
        std::uint32_t effect, std::uint32_t register_b,
        std::uint32_t register_si, std::uint32_t first_argument,
        std::uint32_t second_argument, std::uint32_t third_argument,
        std::uint32_t fourth_argument) override {
        Check(effect == 0x1000u && register_b == 0x2000u &&
                  register_si == 0x3000u && first_argument == 0x7788u &&
                  second_argument == 4u && third_argument == 5u &&
                  fourth_argument == 6u,
              "draw wrapper preserves register/stack arguments");
        trace.push_back("draw-effect");
        return draw_result;
    }
};

void TransformPromotion() {
    Runtime runtime;
    constexpr auto input = std::bit_cast<float>(0x3dccccceu);
    Check(pal4::GameObjectFrameEffects::RenderTransform(input, runtime) ==
              -17 &&
              runtime.received_elapsed == static_cast<double>(input) &&
              runtime.trace == std::vector<std::string>{"render-transform"},
          "float wrapper promotes exact float value and forwards signed result");
    runtime.transform_result = 47;
    Check(pal4::GameObjectFrameEffects::RenderTransform(0.0f, runtime) ==
              47 && runtime.received_elapsed == 0.0,
          "zero transform forwards positive result");
}

void FirstStackArgumentReplacement() {
    Runtime runtime;
    Check(pal4::GameObjectFrameEffects::Draw(
              0x1000u, 0x2000u, 0x3000u, 0x7000u, 4u, 5u, 6u,
              runtime) == 0xabu &&
              runtime.trace == std::vector<std::string>{
                  "read-first", "draw-effect"},
          "draw wrapper dereferences first stack argument and truncates AL");
    runtime.trace.clear();
    runtime.draw_result = 0u;
    Check(pal4::GameObjectFrameEffects::Draw(
              0x1000u, 0x2000u, 0x3000u, 0x7000u, 4u, 5u, 6u,
              runtime) == 0u,
          "draw wrapper preserves zero result");
}

}  // namespace

int main() {
    try {
        TransformPromotion();
        FirstStackArgumentReplacement();
        std::cout << "game_object_frame_effects_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
