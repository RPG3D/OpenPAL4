#include "pal4/combat_transition_overlay.h"
#include "pal4/crt_noop_atexit_registration.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatTransitionRuntime,
                       pal4::ICrtNoopAtExitRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    double ftol_input = 0.0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    float ReadFloat(std::uint32_t address) override {
        calls.push_back("float " + std::to_string(address));
        return floats[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write " + std::to_string(address) + " " +
                        std::to_string(value));
        words[address] = value;
    }
    void WriteFloat(std::uint32_t address, float value) override {
        calls.push_back("write-float " + std::to_string(address));
        floats[address] = value;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
    std::uint32_t ScreenEffect() override {
        calls.emplace_back("screen-effect");
        return 0x8000u;
    }
    void InitializeEffectParameters(std::uint32_t effect) override {
        calls.push_back("effect-params " + std::to_string(effect));
    }
    void ProcessScript6() override { calls.emplace_back("script-6"); }
    std::uint32_t TextureDictionary() override {
        calls.emplace_back("texture-dict");
        return 0x9000u;
    }
    std::uint32_t LoadTexture(std::uint32_t dictionary,
                              std::uint32_t name) override {
        calls.push_back("load " + std::to_string(dictionary) + " " +
                        std::to_string(name));
        return 0xa000u;
    }
    void CleanupScripts() override { calls.emplace_back("script-cleanup"); }
    std::uint32_t GameConfig() override {
        calls.emplace_back("game-config");
        return 0xb000u;
    }
    std::int32_t Ftol(double value) override {
        calls.emplace_back("ftol");
        ftol_input = value;
        return std::isfinite(value) ? static_cast<std::int32_t>(value) : 0;
    }
    std::uint32_t DrawRectangle(
        std::uint32_t frame, std::uint32_t x, std::uint32_t y,
        std::uint32_t width, std::uint32_t height,
        std::uint32_t texture, std::uint32_t red,
        std::uint32_t green, std::uint32_t blue,
        std::int32_t alpha) override {
        Check(frame == 0xc000u && x == 0u && y == 0u &&
              width == 1920u && height == 1080u && texture == 0xa000u &&
              red == 255u && green == 255u && blue == 255u &&
              alpha == 127,
              "draw forwards rectangle, texture, white tint and converted alpha");
        calls.emplace_back("draw");
        return 0xd000u;
    }
    std::uint32_t GameInstance() override {
        calls.emplace_back("game-instance");
        return 0xe000u;
    }
    void InitializeStateAndUI(std::uint32_t game,
                              std::uint32_t state,
                              std::uint32_t document) override {
        calls.push_back("transition " + std::to_string(game) + " " +
                        std::to_string(state) + " " +
                        std::to_string(document));
    }
    int RegisterAtExit(std::uint32_t callback) override {
        calls.push_back("atexit " + std::to_string(callback));
        return -1;
    }
};

void Lifecycle() {
    Runtime runtime;
    using pal4::CombatTransitionOverlay;
    Check(CombatTransitionOverlay::Construct(0x7000u, runtime) == 0x7000u &&
          runtime.words[0x7000u] == 0x8485ecu &&
          runtime.words[0x7000u + 4u] == 0u &&
          runtime.words[0x7000u + 8u] == 0u &&
          runtime.words[0x7000u + 12u] == 0u &&
          runtime.words[0x7000u + 16u] == 0u,
          "overlay constructor installs vtable and clears four fields");
    runtime.calls.clear();
    Check(CombatTransitionOverlay::Initialize(0x7000u, runtime) == 1u &&
          runtime.words[0x7000u + 12u] == 1u &&
          runtime.words[0x7000u + 16u] == 0xa000u &&
          runtime.floats[0x7000u + 4u] == 5.0f &&
          runtime.floats[0x7000u + 8u] == 5.0f &&
          runtime.calls == std::vector<std::string>{
              "screen-effect", "effect-params 32768", "write 28684 1",
              "write-float 28680", "write-float 28676", "script-6",
              "texture-dict", "load 36864 9147328", "write 28688 40960",
              "script-cleanup"},
          "initialization loads CombatOver texture after script setup");
    runtime.calls.clear();
    Check(CombatTransitionOverlay::ResetProperties(0x7000u, runtime) == 1u &&
          runtime.words[0x7000u + 16u] == 0u &&
          runtime.words[0x7000u + 12u] == 0u &&
          runtime.calls == std::vector<std::string>{
              "write 28688 0", "write 28684 0"},
          "property reset clears texture then active flag");
    for (std::uint32_t flags = 0u; flags < 4u; ++flags) {
        runtime.calls.clear();
        Check(CombatTransitionOverlay::Destroy(0x7000u, flags, runtime) ==
                  0x7000u &&
              runtime.words[0x7000u] == 0x8485c8u &&
              runtime.calls.size() == 2u + (flags & 1u) &&
              runtime.calls[0] == "write 28676 0" &&
              runtime.calls[1] == "write 28672 8685000",
              "deleting destructor clears timer, sets base vtable and frees by bit zero");
    }
}

void DrawingAndTimer() {
    Runtime runtime;
    using pal4::CombatTransitionOverlay;
    Check(CombatTransitionOverlay::Draw(0x7000u, 0xc000u, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"read 28684"},
          "inactive overlay skips texture and drawing");
    runtime.words[0x7000u + 12u] = 1u;
    runtime.calls.clear();
    Check(CombatTransitionOverlay::Draw(0x7000u, 0xc000u, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{
              "read 28684", "read 28688"},
          "missing texture skips game configuration and drawing");
    runtime.words[0x7000u + 16u] = 0xa000u;
    runtime.words[0xb000u] = 1920u;
    runtime.words[0xb000u + 4u] = 1080u;
    runtime.floats[0x7000u + 4u] = 2.5f;
    runtime.floats[0x7000u + 8u] = 5.0f;
    runtime.calls.clear();
    Check(CombatTransitionOverlay::Draw(0x7000u, 0xc000u, runtime) == 0xd000u &&
          runtime.ftol_input == 127.5 &&
          runtime.calls.back() == "draw",
          "active overlay calculates alpha and forwards draw return");

    runtime.words[0x7000u + 12u] = 0u;
    runtime.calls.clear();
    CombatTransitionOverlay::AdvanceTimer(0x7000u, 0.5f, runtime);
    Check(runtime.calls == std::vector<std::string>{"read 28684"},
          "inactive timer does not read or change remaining time");
    runtime.words[0x7000u + 12u] = 1u;
    runtime.calls.clear();
    CombatTransitionOverlay::AdvanceTimer(0x7000u, 0.5f, runtime);
    Check(runtime.floats[0x7000u + 4u] == 2.0f &&
          runtime.calls.back() == "write-float 28676",
          "positive timer remains in combat overlay");
    runtime.calls.clear();
    CombatTransitionOverlay::AdvanceTimer(0x7000u, 2.0f, runtime);
    Check(runtime.floats[0x7000u + 4u] == 0.0f &&
          runtime.calls.back() == "transition 57344 0 0",
          "zero timer returns to state zero");
    runtime.floats[0x7000u + 4u] =
        std::numeric_limits<float>::quiet_NaN();
    runtime.calls.clear();
    CombatTransitionOverlay::AdvanceTimer(0x7000u, 0.5f, runtime);
    Check(std::isnan(runtime.floats[0x7000u + 4u]) &&
          runtime.calls.back() == "write-float 28676",
          "unordered timer comparison does not switch game state");
}

void CrtRegistrations() {
    Runtime runtime;
    std::uint8_t guard = 0xc0u;
    Check(pal4::CrtNoopAtExitRegistration::Run(guard, runtime) == -1 &&
          pal4::CrtNoopAtExitRegistration::Run(guard, runtime) == -1 &&
          guard == 0xc1u &&
          runtime.calls == std::vector<std::string>{
              "atexit 6161760", "atexit 6161760"},
          "0x58b350 and 0x58b4f0 repeat shared guarded CRT registration");
}

}  // namespace

int main() {
    try {
        Lifecycle();
        DrawingAndTimer();
        CrtRegistrations();
        std::cout << "combat transition overlay tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
