#include "pal4/shop_ui_update.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IShopUiUpdateRuntime {
    static constexpr std::uint32_t kFrame = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::array<bool, 3> categories{false, false, false};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> shown;
    std::vector<std::uint32_t> hidden;
    std::vector<std::uint32_t> positioned;
    std::vector<std::uint32_t> mouse_windows;
    std::uint32_t empty_length = 0u;
    int compare_result = 0;
    std::uint32_t game_entry = 0u;
    std::uint32_t effect = 0u;
    std::uint32_t game_database_calls = 0u;
    std::uint32_t effect_calls = 0u;
    float bottom_height = 2.0f;
    float image_y = 7.0f;

    Runtime() {
        for (std::uint32_t offset = 0x40u; offset < 0x60u; offset += 4u) {
            words[kFrame + offset] = 0x1000u + offset;
        }
        words[kFrame + 0x30u] = 0x9000u;
        words[kFrame + 0x34u] = 0x9030u;
        words[kFrame + 0x3cu] = 0x55u;
        words[kFrame + 0x74u] = 0xa000u;
        words[kFrame + 0x78u] = 0u;
        words[kFrame + 0xc0u] = 0x2000u;
        words[kFrame + 0xc4u] = 0x2004u;
        words[0xb000u + 0x34u] = 0u;
    }

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write " + std::to_string(address));
    }
    void SetVisible(const std::uint32_t window,
                    const bool visible) override {
        calls.emplace_back(visible ? "show" : "hide");
        (visible ? shown : hidden).push_back(window);
    }
    void SetPosition(const std::uint32_t window,
                     const std::uint32_t vector) override {
        calls.emplace_back("position");
        positioned.push_back(window);
        Check(vector == kFrame + 0x80u +
                  static_cast<std::uint32_t>(positioned.size() - 1u) * 8u,
              "buttons use consecutive saved position vectors");
    }
    void SetupMouseEventHandlers(const std::uint32_t list,
                                 const std::uint32_t window) override {
        Check(list == kFrame + 0x28u, "mouse list offset");
        calls.emplace_back("mouse");
        mouse_windows.push_back(window);
        words[kFrame + 0x34u] += 4u;
    }
    std::uint32_t TradeSystem() override {
        calls.emplace_back("trade system");
        return 0x3000u;
    }
    bool HasTradeItems(const std::uint32_t system,
                       const std::uint32_t text,
                       const std::uint32_t category) override {
        Check(system == 0x3000u && text == kFrame + 0x60u &&
                  category < categories.size(),
              "trade query arguments");
        calls.emplace_back("trade " + std::to_string(category));
        return categories[category];
    }
    std::uint32_t CStringLength(const std::uint32_t text) override {
        Check(text == pal4::ShopUiUpdate::kEmptyErrorString,
              "global empty/error string address");
        calls.emplace_back("length");
        return empty_length;
    }
    int CompareBytes(const std::uint32_t left, const std::uint32_t right,
                     const std::uint32_t length) override {
        Check(left == 0xa000u &&
                  right == pal4::ShopUiUpdate::kEmptyErrorString &&
                  length == (words.at(kFrame + 0x78u) < empty_length
                                 ? words.at(kFrame + 0x78u) : empty_length),
              "task text compares shortest byte span");
        calls.emplace_back("compare");
        return compare_result;
    }
    std::uint32_t Game() override {
        calls.emplace_back("game");
        return 0x4000u;
    }
    std::uint32_t GameEntry(const std::uint32_t game,
                            const std::uint32_t id) override {
        Check(game == 0x4000u && id == 15u, "game entry 15");
        calls.emplace_back("entry");
        return game_entry;
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("database");
        ++game_database_calls;
        return 0x4100u;
    }
    std::uint32_t FindShopRecord(const std::uint32_t player,
                             const std::uint32_t shop_text) override {
        Check(player == 0x416cu && shop_text == kFrame + 0x60u,
              "effect player subobject and shop text");
        calls.emplace_back("effect");
        ++effect_calls;
        return effect;
    }
    float GetHeight(const std::uint32_t window) override {
        calls.emplace_back("get height");
        if (window == 0x1040u) return 3.0f;
        Check(window == 0x2000u, "bottom window height receiver");
        return bottom_height;
    }
    void SetHeight(const std::uint32_t window,
                   const float height) override {
        Check(window == 0x2000u, "bottom window height target");
        calls.emplace_back("set height");
        bottom_height = height;
    }
    float GetYPosition(const std::uint32_t window) override {
        Check(window == 0x2004u, "bottom image y receiver");
        calls.emplace_back("get y");
        return image_y;
    }
    void SetYPosition(const std::uint32_t window, const float y) override {
        Check(window == 0x2004u, "bottom image y target");
        calls.emplace_back("set y");
        image_y = y;
    }
};

void TestAllButtons() {
    Runtime runtime;
    runtime.categories = {true, true, true};
    runtime.words[Runtime::kFrame + 0x78u] = 4u;
    runtime.game_entry = 1u;
    runtime.effect = 0xb000u;
    Check(pal4::ShopUiUpdate::Update(Runtime::kFrame, runtime) ==
              0x9000u,
          "update returns original mouse list begin");
    Check(runtime.hidden == std::vector<std::uint32_t>{
              0x1040u, 0x1044u, 0x1048u, 0x104cu,
              0x1050u, 0x1054u, 0x1058u, 0x105cu} &&
              runtime.shown == std::vector<std::uint32_t>{
              0x1040u, 0x1044u, 0x1048u, 0x104cu,
              0x1050u, 0x1058u, 0x1054u, 0x105cu} &&
              runtime.positioned == runtime.shown &&
              runtime.mouse_windows == runtime.shown,
          "all eight buttons are hidden then positioned in selection order");
    Check(runtime.bottom_height == 24.0f && runtime.image_y == 29.0f &&
              runtime.words.at(Runtime::kFrame + 0x34u) == 0x9020u &&
              runtime.words.at(Runtime::kFrame + 0x3cu) == 0u &&
              runtime.game_database_calls == 1u &&
              runtime.effect_calls == 1u,
          "eight rows resize the bottom and clear active mouse index");
    Check(runtime.calls[0] == "hide" && runtime.calls[7] == "hide" &&
              runtime.calls[8] == "write 20532" &&
              runtime.calls[9] == "trade system" &&
              runtime.calls[10] == "trade 0" &&
              runtime.calls[11] == "show" &&
              runtime.calls[12] == "position" &&
              runtime.calls[13] == "show" &&
              runtime.calls[14] == "position" &&
              runtime.calls[15] == "mouse" &&
              runtime.calls[16] == "mouse",
          "paired buttons are positioned before either mouse binding");
}

void TestOnlyExitAndNullMouseBegin() {
    Runtime runtime;
    runtime.words[Runtime::kFrame + 0x30u] = 0u;
    Check(pal4::ShopUiUpdate::Update(Runtime::kFrame, runtime) == 0u &&
              runtime.shown == std::vector<std::uint32_t>{0x105cu} &&
              runtime.bottom_height == 3.0f && runtime.image_y == 8.0f &&
              runtime.words.at(Runtime::kFrame + 0x3cu) == 0x55u &&
              runtime.game_database_calls == 0u &&
              runtime.effect_calls == 0u,
          "empty shop retains only exit and skips mouse index reset for null begin");
}

void TestConditionalBranches() {
    Runtime runtime;
    runtime.categories = {false, true, false};
    runtime.empty_length = 3u;
    runtime.words[Runtime::kFrame + 0x78u] = 3u;
    runtime.game_entry = 1u;
    runtime.effect = 0xb000u;
    runtime.words[0xb034u] = 1u;
    static_cast<void>(pal4::ShopUiUpdate::Update(Runtime::kFrame, runtime));
    Check(runtime.shown == std::vector<std::uint32_t>{
              0x1048u, 0x104cu, 0x105cu} &&
              runtime.bottom_height == 9.0f &&
              runtime.effect_calls == 1u,
          "matching task text and nonzero effect field hide task and smith");

    Runtime different;
    different.empty_length = 5u;
    different.words[Runtime::kFrame + 0x78u] = 3u;
    different.compare_result = 0;
    different.game_entry = 1u;
    different.effect = 0u;
    static_cast<void>(pal4::ShopUiUpdate::Update(Runtime::kFrame, different));
    Check(different.shown == std::vector<std::uint32_t>{
              0x1058u, 0x105cu} && different.bottom_height == 6.0f &&
              different.game_database_calls == 1u,
          "different task length shows task while null effect hides smith");

    Runtime changed_bytes;
    changed_bytes.empty_length = 2u;
    changed_bytes.words[Runtime::kFrame + 0x78u] = 2u;
    changed_bytes.compare_result = -1;
    static_cast<void>(pal4::ShopUiUpdate::Update(Runtime::kFrame,
                                                  changed_bytes));
    Check(changed_bytes.shown == std::vector<std::uint32_t>{
              0x1058u, 0x105cu},
          "same-length nonmatching task bytes show task button");
}

}  // namespace

int main() {
    TestAllButtons();
    TestOnlyExitAndNullMouseBegin();
    TestConditionalBranches();
    return EXIT_SUCCESS;
}
