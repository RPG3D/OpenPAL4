#include "pal4/script_combat_config.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptCombatConfigRuntime {
    bool throw_apply = false;
    int throw_text_index = -1;
    bool valid_release = true;
    std::uint32_t expected_type = 0;
    std::vector<std::string> calls;

    std::uint32_t CombatSystem() override {
        calls.emplace_back("combat");
        return 0x1000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address >= 0x11b0u && address <= 0x11c0u &&
              (address - 0x11b0u) % 4u == 0u,
              "five original combat word addresses");
        const auto index = (address - 0x11b0u) / 4u;
        calls.push_back("word:" + std::to_string(index));
        return 10u + index;
    }
    pal4::CombatConfigText ReadTextProperty(
        std::uint32_t address) override {
        Check(address >= 0x11c4u && address <= 0x11e4u &&
              (address - 0x11c4u) % 16u == 0u,
              "three original text property addresses");
        const auto index = (address - 0x11c4u) / 16u;
        calls.push_back("text:" + std::to_string(index));
        if (static_cast<int>(index) == throw_text_index)
            throw std::runtime_error("text copy failed");
        return {static_cast<std::uint8_t>(0x80u + index),
                "text-" + std::to_string(index)};
    }
    void ApplyConfiguration(const pal4::CombatConfigSnapshot& config) override {
        calls.emplace_back("apply");
        Check(config.words == std::array<std::uint32_t, 5>{
                  0xabcdef01u, 0x76543210u, 12u, 13u, expected_type},
              "slot, value, retained words and selected type");
        Check(config.flags == std::array<std::uint8_t, 3>{0x34u, 1u, 1u},
              "enabled byte and two forced property flags");
        for (std::uint32_t i = 0; i < 3u; ++i)
            Check(config.texts[i].marker == 0x80u + i &&
                  config.texts[i].value == "text-" + std::to_string(i),
                  "three copied text buffers retain markers and content");
        if (throw_apply) throw std::runtime_error("apply failed");
    }
    std::uint32_t ReleaseText(pal4::CombatConfigText& text,
                              std::uint32_t index) noexcept override {
        if (index >= 3u || text.value != "text-" + std::to_string(index))
            valid_release = false;
        calls.push_back("release:" + std::to_string(index));
        return 0x41u + index;
    }
};

std::vector<std::string> Prefix() {
    return {"combat", "word:0", "word:1", "word:2", "word:3",
            "word:4", "text:0", "text:1", "text:2"};
}

void TypeBranchesAndCleanup() {
    for (const auto type : {0u, 1u, 2u, 3u, 0xffffffffu}) {
        Runtime runtime;
        runtime.expected_type = type <= 2u ? type : 14u;
        Check(pal4::ScriptCombatConfig::Configure(
                  0x1234u, 0xabcdef01u, 0x76543210u, type,
                  runtime) == 0x41u && runtime.valid_release,
              "first text release result and valid cleanup");
        auto expected = Prefix();
        expected.insert(expected.end(), {
            "apply", "release:2", "release:1", "release:0"});
        Check(runtime.calls == expected,
              "copy and reverse cleanup order for combat type");
    }
}

void FailureCleanup() {
    Runtime apply;
    apply.expected_type = 1u;
    apply.throw_apply = true;
    try {
        (void)pal4::ScriptCombatConfig::Configure(
            0x1234u, 0xabcdef01u, 0x76543210u, 1u, apply);
        throw std::runtime_error("missing apply failure");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) == "apply failed",
              "apply exception propagates");
    }
    auto expected = Prefix();
    expected.insert(expected.end(), {
        "apply", "release:2", "release:1", "release:0"});
    Check(apply.calls == expected && apply.valid_release,
          "all temporary texts released when apply fails");

    Runtime copy;
    copy.throw_text_index = 2;
    try {
        (void)pal4::ScriptCombatConfig::Configure(
            0x1234u, 0xabcdef01u, 0x76543210u, 1u, copy);
        throw std::runtime_error("missing copy failure");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) == "text copy failed",
              "copy exception propagates");
    }
    expected = Prefix();
    expected.insert(expected.end(), {"release:1", "release:0"});
    Check(copy.calls == expected && copy.valid_release,
          "only constructed text buffers released on copy failure");
}

}  // namespace

int main() {
    try {
        TypeBranchesAndCleanup();
        FailureCleanup();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
