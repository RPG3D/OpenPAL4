#include "pal4/script_combat_vip_monster.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptCombatVipMonsterRuntime {
    std::vector<std::string> calls;
    std::string fail_at;
    std::uint32_t expected_id = 0xabcdef01u;

    void Record(std::string call) {
        calls.push_back(call);
        if (call == fail_at) throw std::runtime_error("injected failure");
    }
    std::uint32_t CombatSystem() override {
        Record("combat");
        return 0x5000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address >= 0x51b0u && address <= 0x51c0u &&
                  (address - 0x51b0u) % 4u == 0u,
              "combat property word address");
        const auto index = (address - 0x51b0u) / 4u;
        Record("word" + std::to_string(index));
        return 0x100u + index;
    }
    pal4::CombatConfigText ReadTextProperty(
        std::uint32_t address) override {
        Check(address >= 0x51c4u && address <= 0x51e4u &&
                  (address - 0x51c4u) % 16u == 0u,
              "combat text property address");
        const auto index = (address - 0x51c4u) / 16u;
        Record("text" + std::to_string(index));
        return {static_cast<std::uint8_t>(index + 1u),
                "value" + std::to_string(index)};
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address >= 0x51f4u && address <= 0x51f6u,
              "combat trailing flag address");
        const auto index = address - 0x51f4u;
        Record("flag" + std::to_string(index));
        return static_cast<std::uint8_t>(0xa0u + index);
    }
    void ApplyConfiguration(
        const pal4::CombatConfigSnapshot& config) override {
        Check(config.words == std::array<std::uint32_t, 5>{
                  0x100u, 0x101u, 0x102u, expected_id, 0x104u},
              "only VIP monster word is replaced");
        for (std::uint32_t i = 0u; i < 3u; ++i)
            Check(config.texts[i].marker == i + 1u &&
                      config.texts[i].value == "value" + std::to_string(i) &&
                      config.flags[i] == 0xa0u + i,
                  "copied text and flag values preserved");
        Record("apply");
    }
    std::uint32_t ReleaseText(pal4::CombatConfigText& text,
                              std::uint32_t index) noexcept override {
        calls.push_back("release" + std::to_string(index));
        text.value.clear();
        return 0x7000u + index;
    }
};

void Complete() {
    Runtime runtime;
    Check(pal4::ScriptCombatVipMonster::Configure(
              runtime.expected_id, runtime) == 0x7000u,
          "first text release result returned");
    Check(runtime.calls == std::vector<std::string>{
              "combat", "word0", "word1", "word2", "word3", "word4",
              "text0", "text1", "text2", "flag0", "flag1", "flag2",
              "apply", "release2", "release1", "release0"},
          "combat snapshot copy, commit and reverse cleanup order");
}

void Failures() {
    for (const auto* fail : {"text1", "flag2", "apply"}) {
        Runtime runtime;
        runtime.fail_at = fail;
        bool caught = false;
        try {
            (void)pal4::ScriptCombatVipMonster::Configure(
                runtime.expected_id, runtime);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught, "injected failure propagated");
        if (std::string(fail) == "text1") {
            Check(runtime.calls.back() == "release0",
                  "partial text copy releases completed text");
        } else {
            Check(runtime.calls[runtime.calls.size() - 3u] == "release2" &&
                      runtime.calls[runtime.calls.size() - 2u] == "release1" &&
                      runtime.calls.back() == "release0",
                  "post-copy failure releases all texts in reverse order");
        }
    }
}

}  // namespace

int main() {
    try {
        Complete();
        Failures();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
