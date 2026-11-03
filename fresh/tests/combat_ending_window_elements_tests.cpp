#include "pal4/combat_ending_window_elements.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr std::uint32_t kDialog = 0x1000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEndingWindowElementsRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::string, std::uint32_t> windows;
    std::vector<std::string> names;
    std::vector<std::string> calls;
    std::uint32_t callback_allocation = 0xa000u;
    bool throw_subscription = false;
    std::uint32_t released = 0u;
    std::uint32_t position_calls = 0u;

    Runtime() { words[kDialog + 36u] = 0x5000u; }
    std::string LayoutPrefix(std::uint32_t address) override {
        Check(address == kDialog + 20u, "layout prefix offset");
        calls.emplace_back("prefix");
        return "Ending/";
    }
    std::uint32_t FindWindow(std::uint32_t dialog,
                             std::string_view name) override {
        Check(dialog == kDialog && name.starts_with("Ending/"),
              "layout window lookup");
        const auto address = 0x6000u +
                             4u * static_cast<std::uint32_t>(names.size());
        names.emplace_back(name);
        windows.emplace(names.back(), address);
        if (name.find("StaticNeedExp") != name.npos)
            words[address + 68u] = address + 0x1000u;
        calls.emplace_back("find:" + std::string(name));
        return address;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    void SetAlwaysOnTop(std::uint32_t window, bool enabled) override {
        Check(enabled, "always-on-top enabled");
        calls.emplace_back("top:" + std::to_string(window));
    }
    void SetFormatting(std::uint32_t window, std::uint32_t horizontal,
                       std::uint32_t vertical) override {
        Check(window == windows.at("Ending/StaticScoreMark") &&
                  horizontal == 2u && vertical == 2u,
              "score formatting");
        calls.emplace_back("format");
    }
    void SetHorizontalFormatting(std::uint32_t window,
                                 std::uint32_t mode) override {
        Check(window == windows.at("Ending/StaticMoney") && mode == 2u,
              "money horizontal formatting");
        calls.emplace_back("horizontal");
    }
    void SetClippedByParent(std::uint32_t window,
                            bool clipped) override {
        Check(!clipped &&
                  (window == windows.at("Ending/EditPropLeft") ||
                   window == windows.at("Ending/EditPropRight")),
              "property edits unclipped");
        calls.emplace_back("clip");
    }
    float GetYPosition(std::uint32_t window) override {
        Check(window >= 0x6000u, "row panel Y source");
        calls.emplace_back("y");
        return 12.5f;
    }
    float GetWidth(std::uint32_t window) override {
        Check(window >= 0x6000u, "image width source");
        calls.emplace_back("width");
        return 25.25f;
    }
    pal4::CombatUiPosition GetPosition(std::uint32_t window,
                                       std::uint32_t metrics) override {
        Check(metrics == 0u && window >= 0x6000u,
              "need-exp position source and metric");
        ++position_calls;
        calls.emplace_back("position");
        return {3.0f, 4.0f};
    }
    pal4::CombatUiPosition WindowToScreen(
        std::uint32_t parent, pal4::CombatUiPosition value) override {
        Check(parent >= 0x7000u && value.x == 3.0f && value.y == 4.0f,
              "parent window screen conversion");
        calls.emplace_back("screen");
        return {value.x + 10.0f, value.y + 20.0f};
    }
    std::uint32_t GuiRootWindow() override {
        calls.emplace_back("root");
        return 0x9000u;
    }
    pal4::CombatUiPosition AbsoluteToRelative(
        std::uint32_t root, pal4::CombatUiPosition value) override {
        Check(root == 0x9000u && value.x == 13.0f && value.y == 24.0f,
              "relative coordinate input");
        calls.emplace_back("relative");
        return {value.x - 1.0f, value.y - 2.0f};
    }
    std::uint32_t AllocateCallback(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "mouse callback size");
        calls.emplace_back("allocate");
        return callback_allocation;
    }
    std::uint32_t SubscribeMouseDown(std::uint32_t root,
                                     std::uint32_t callback) override {
        Check(root == 0x5000u && callback == callback_allocation,
              "mouse subscription arguments");
        calls.emplace_back("subscribe");
        if (throw_subscription) throw std::runtime_error("subscribe failed");
        return callback;
    }
    void ReleaseCallback(std::uint32_t callback) override {
        released = callback;
        calls.emplace_back("release");
    }
};

void CheckWindowBindings(const Runtime& runtime) {
    constexpr std::array fixed{
        std::pair{80u, "PanelScoreLv"},
        std::pair{84u, "PanelScoreMark"},
        std::pair{64u, "ImgScoreLv"},
        std::pair{88u, "PanelPrize"},
        std::pair{104u, "EditPrizeLeft"},
        std::pair{108u, "EditPrizeRight"},
        std::pair{112u, "StaticPrizeTitle"},
        std::pair{68u, "StaticScoreMark"},
        std::pair{72u, "PanelMoney"},
        std::pair{76u, "PanelProp"},
        std::pair{92u, "StaticMoney"},
        std::pair{96u, "EditPropLeft"},
        std::pair{100u, "EditPropRight"}};
    for (std::uint32_t i = 0; i < fixed.size(); ++i) {
        const auto full_name = "Ending/" + std::string(fixed[i].second);
        Check(runtime.names[i] == full_name &&
                  runtime.words.at(kDialog + fixed[i].first) ==
                      runtime.windows.at(full_name),
              "fixed binding order and slot");
    }
    constexpr std::array row{
        std::pair{116u, "PanelRole"},
        std::pair{120u, "StaticName"},
        std::pair{124u, "StaticHp"},
        std::pair{128u, "StaticMp"},
        std::pair{132u, "StaticRage"},
        std::pair{136u, "StaticExp"},
        std::pair{140u, "StaticNeedExp"},
        std::pair{144u, "ImgHp"},
        std::pair{148u, "ImgMp"},
        std::pair{152u, "ImgRage"},
        std::pair{156u, "ImgRole"},
        std::pair{160u, "ImgName"}};
    Check(runtime.names.size() == 61u, "13 fixed plus 48 row windows");
    for (std::uint32_t i = 0; i < 4u; ++i) {
        for (std::uint32_t j = 0; j < row.size(); ++j) {
            const auto full_name = "Ending/" + std::string(row[j].second) +
                                   std::to_string(i);
            Check(runtime.names[13u + 12u * i + j] == full_name &&
                      runtime.words.at(kDialog + 76u * i + row[j].first) ==
                          runtime.windows.at(full_name),
                  "row binding order and slot");
        }
        const auto base = kDialog + 76u * i;
        Check(runtime.words.at(base + 176u) ==
                  std::bit_cast<std::uint32_t>(12.5f) &&
                  runtime.words.at(base + 164u) ==
                      std::bit_cast<std::uint32_t>(25.25f) &&
                  runtime.words.at(base + 168u) ==
                      std::bit_cast<std::uint32_t>(25.25f) &&
                  runtime.words.at(base + 172u) ==
                      std::bit_cast<std::uint32_t>(25.25f) &&
                  runtime.words.at(base + 180u) ==
                      std::bit_cast<std::uint32_t>(12.0f) &&
                  runtime.words.at(base + 184u) ==
                      std::bit_cast<std::uint32_t>(22.0f),
              "row Y, image widths and converted coordinates");
    }
    Check(runtime.position_calls == 4u, "four coordinate conversions");
}

void SuccessfulSetup() {
    Runtime runtime;
    pal4::CombatEndingWindowElements::Setup(kDialog, runtime);
    CheckWindowBindings(runtime);
    Check(runtime.words[0xa000u] == 0x845388u &&
              runtime.words[0xa008u] == 0x544e40u &&
              runtime.words[0xa00cu] == 0u &&
              runtime.words[0xa010u] == kDialog,
          "mouse callback fields");
    const auto allocated = std::find(runtime.calls.begin(),
                                     runtime.calls.end(), "allocate");
    Check(runtime.released == 0xa000u &&
              allocated != runtime.calls.end() &&
              allocated < runtime.calls.end() - 2 &&
              runtime.calls[runtime.calls.size() - 2] == "subscribe" &&
              runtime.calls.back() == "release",
          "temporary callback released after subscription");
}

void UnavailableCallback() {
    Runtime runtime;
    runtime.callback_allocation = 0u;
    pal4::CombatEndingWindowElements::Setup(kDialog, runtime);
    CheckWindowBindings(runtime);
    Check(runtime.released == 0u &&
              runtime.calls.back() == "subscribe",
          "null callback still subscribed without release");
}

void SubscriptionFailure() {
    Runtime runtime;
    runtime.throw_subscription = true;
    bool threw = false;
    try {
        pal4::CombatEndingWindowElements::Setup(kDialog, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.released == 0xa000u &&
              runtime.calls.back() == "release",
          "subscription failure releases temporary callback");
}

}  // namespace

int main() {
    try {
        SuccessfulSetup();
        UnavailableCallback();
        SubscriptionFailure();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
