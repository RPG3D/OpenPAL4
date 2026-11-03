#include "pal4/combat_result_sound_element.h"

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

struct Runtime final : pal4::ICombatResultSoundRuntime {
    std::uint8_t visible = 0u;
    std::uint8_t sound = 0u;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x1000u + 60u, "window field");
        calls.emplace_back("window");
        return 0x9000u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        if (address == 0x1000u + 65u) {
            calls.emplace_back("sound-flag");
            return sound;
        }
        Check(address == 0x1000u + 64u, "visibility field");
        calls.emplace_back("visible-flag");
        return visible;
    }
    void PlaySound(std::string_view name,
                   std::uint32_t mode) override {
        Check(name == "WA46.wav" && mode == 1u,
              "conditional sound identity");
        calls.emplace_back("sound");
    }
    void SetVisible(std::uint32_t window, bool value) override {
        Check(window == 0x9000u && value == (visible != 0u),
              "window visibility forwarded");
        calls.emplace_back("set-visible");
    }
};

void AllFlagCombinations() {
    for (const auto sound : {0u, 1u, 2u}) {
        for (const auto visible : {0u, 1u, 2u}) {
            Runtime runtime;
            runtime.sound = static_cast<std::uint8_t>(sound);
            runtime.visible = static_cast<std::uint8_t>(visible);
            Check(pal4::CombatResultSoundElement::Show(
                      0x1000u, runtime) == 1u,
                  "result byte");
            const std::vector<std::string> expected = sound == 0u ?
                std::vector<std::string>{"sound-flag", "visible-flag",
                    "window", "set-visible"} :
                std::vector<std::string>{"sound-flag", "sound",
                    "visible-flag", "window", "set-visible"};
            Check(runtime.calls == expected,
                  "conditional audio precedes unconditional visibility");
        }
    }
}

}  // namespace

int main() {
    try {
        AllFlagCombinations();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
