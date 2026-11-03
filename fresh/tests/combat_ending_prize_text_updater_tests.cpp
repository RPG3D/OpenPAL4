#include "pal4/combat_ending_prize_text_updater.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::uint32_t kDialog = 0x1000u;
constexpr std::uint32_t kHead = 0x2000u;

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEndingResultTextRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> next;
    std::map<std::uint32_t, std::string> texts;
    std::map<std::uint32_t, float> heights;
    std::vector<std::string> calls;
    float left_text_height = 20.0f;
    float right_text_height = 30.0f;

    Runtime() {
        words[kDialog + 104u] = 1040u;
        words[kDialog + 108u] = 1080u;
        words[kDialog + 112u] = 1120u;
        words[kDialog + 88u] = 880u;
        words[kDialog + 96u] = 960u;
        words[kDialog + 100u] = 1000u;
        words[kDialog + 76u] = 760u;
        words[kDialog + 444u] = kHead + 0x100u;
        words[kDialog + 460u] = kHead;
        words[kHead] = kHead;
        words[kHead + 0x100u] = kHead + 0x100u;
        heights[1040u] = 60.0f;
        heights[1080u] = 60.0f;
        heights[880u] = 100.0f;
        heights[960u] = 60.0f;
        heights[1000u] = 60.0f;
        heights[760u] = 100.0f;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint32_t NextTreeNode(std::uint32_t node) override {
        calls.push_back("next:" + std::to_string(node));
        return next.at(node);
    }
    std::optional<std::string> PrimaryName(std::uint32_t key) override {
        calls.push_back("primary:" + std::to_string(key));
        if (key == 1u) return "A";
        if (key == 3u) return "C";
        return std::nullopt;
    }
    std::optional<std::string> SecondaryName(std::uint32_t key) override {
        calls.push_back("secondary:" + std::to_string(key));
        if (key == 1u) return "wrong";
        if (key == 2u) return "B";
        return std::nullopt;
    }
    std::string GameString(std::uint32_t id) override {
        calls.push_back("string:" + std::to_string(id));
        return id == 0x101u ? "Prize" : "%s:%u;";
    }
    std::string FormatEntry(std::string_view format,
                            std::string_view name,
                            std::uint32_t value) override {
        Check(format == "%s:%u;", "entry format ID");
        calls.push_back("format:" + std::string(name));
        return std::string(name) + ":" + std::to_string(value) + ";";
    }
    std::string ToUtf8(std::string_view text) override {
        return std::string(text);
    }
    void SetFormatting(std::uint32_t window, std::uint32_t horizontal,
                       std::uint32_t vertical) override {
        Check(window == 1120u && horizontal == 2u && vertical == 2u,
              "title formatting");
        calls.emplace_back("format-title");
    }
    void SetText(std::uint32_t window, std::string_view text) override {
        texts[window] = text;
        calls.push_back("text:" + std::to_string(window));
    }
    float GetTextHeight(std::uint32_t window) override {
        calls.push_back("text-height:" + std::to_string(window));
        if (window == 960u) return 45.0f;
        if (window == 1000u) return 20.0f;
        return window == 1040u ? left_text_height : right_text_height;
    }
    float GetHeight(std::uint32_t window, std::uint32_t metric) override {
        Check(metric == 1u, "height metric");
        calls.push_back("height:" + std::to_string(window));
        return heights[window];
    }
    void SetHeight(std::uint32_t window, std::uint32_t metric,
                   float height) override {
        Check(metric == 1u, "set height metric");
        heights[window] = height;
        calls.push_back("set-height:" + std::to_string(window));
    }
    int CleanupFormatBuffer() override {
        calls.emplace_back("cleanup");
        return 42;
    }
};

std::size_t Find(const Runtime& runtime, std::string_view call) {
    for (std::size_t i = 0u; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    throw std::runtime_error("missing call");
}

void CountGate() {
    Runtime runtime;
    Check(pal4::CombatEndingPrizeTextUpdater::Update(kDialog, runtime) == 0 &&
              runtime.calls.empty(), "zero count exits before formatting");
}

void AlternateOnlyMatchedRows() {
    Runtime runtime;
    runtime.words[kDialog + 468u] = 4u;
    runtime.words[kHead] = 0x2100u;
    runtime.next = {{0x2100u, 0x2200u}, {0x2200u, 0x2300u},
                    {0x2300u, 0x2400u}, {0x2400u, kHead}};
    runtime.words[0x2100u + 12u] = 1u;
    runtime.words[0x2100u + 16u] = 10u;
    runtime.words[0x2200u + 12u] = 99u;
    runtime.words[0x2300u + 12u] = 2u;
    runtime.words[0x2300u + 16u] = 20u;
    runtime.words[0x2400u + 12u] = 3u;
    runtime.words[0x2400u + 16u] = 30u;
    Check(pal4::CombatEndingPrizeTextUpdater::Update(kDialog, runtime) == 42,
          "format cleanup result");
    Check(runtime.texts[1120u] == "Prize" &&
              runtime.texts[1040u] == "A:10;C:30;" &&
              runtime.texts[1080u] == "B:20;",
          "matched records alternate columns in tree order");
    Check(Find(runtime, "primary:1") < Find(runtime, "format:A") &&
              Find(runtime, "primary:2") < Find(runtime, "secondary:2") &&
              Find(runtime, "secondary:2") < Find(runtime, "format:B") &&
              Find(runtime, "format:B") < Find(runtime, "next:8960") &&
              Find(runtime, "set-height:880") < Find(runtime, "cleanup"),
          "lookup precedence, iteration and cleanup order");
    Check(std::fabs(runtime.heights[1040u] - 30.0f) < 0.001f &&
              std::fabs(runtime.heights[1080u] - 30.0f) < 0.001f &&
              std::fabs(runtime.heights[880u] - 70.0f) < 0.001f,
          "both edits use max text height; panel removes excess");
}

void EmptyTreeAndLeftTallest() {
    Runtime runtime;
    runtime.words[kDialog + 468u] = 1u;
    runtime.left_text_height = 55.0f;
    runtime.right_text_height = 20.0f;
    (void)pal4::CombatEndingPrizeTextUpdater::Update(kDialog, runtime);
    Check(runtime.texts[1040u].empty() && runtime.texts[1080u].empty() &&
              runtime.heights[1040u] == 55.0f &&
              runtime.heights[1080u] == 55.0f &&
              runtime.heights[880u] == 95.0f,
          "nonzero count with empty tree and left maximum");
}

void PropertyColumnsUseOtherTreeAndControls() {
    Runtime runtime;
    const auto head = kHead + 0x100u;
    runtime.words[head] = 0x3100u;
    runtime.next = {{0x3100u, 0x3200u}, {0x3200u, 0x3300u},
                    {0x3300u, head}};
    runtime.words[0x3100u + 12u] = 1u;
    runtime.words[0x3100u + 16u] = 7u;
    runtime.words[0x3200u + 12u] = 99u;
    runtime.words[0x3300u + 12u] = 2u;
    runtime.words[0x3300u + 16u] = 9u;
    Check(pal4::CombatEndingPropertyTextUpdater::Update(kDialog, runtime) ==
              42 && runtime.texts[960u] == "A:7;" &&
              runtime.texts[1000u] == "B:9;" &&
              !runtime.texts.contains(1120u),
          "property tree uses separate head, edit controls and no title");
    Check(runtime.heights[960u] == 45.0f &&
              runtime.heights[1000u] == 45.0f &&
              runtime.heights[760u] == 85.0f &&
              runtime.heights[880u] == 100.0f,
          "property panel alone adjusts for edit content");

    Runtime empty;
    Check(pal4::CombatEndingPropertyTextUpdater::Update(kDialog, empty) ==
              42 && empty.texts[960u].empty() &&
              empty.texts[1000u].empty(),
          "property entry has no count gate");
}

}  // namespace

int main() {
    try {
        CountGate();
        AlternateOnlyMatchedRows();
        EmptyTreeAndLeftTallest();
        PropertyColumnsUseOtherTreeAndControls();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
