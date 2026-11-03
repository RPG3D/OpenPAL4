#include "pal4/combat_action_console_window_elements.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::array<std::string_view, 6> kNames{
    "BtnAttack", "BtnDefend", "BtnStunt",
    "BtnMagic", "BtnArticle", "BtnFlee"};

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatActionConsoleWindowElementRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t button_index{};
    std::uint32_t format_index{};
    std::uint32_t text_cleanups{};
    std::uint32_t destroyed{};
    std::uint32_t top_window{};
    std::string fail_at;

    void Call(const std::string& name) {
        calls.push_back(name);
        if (name == fail_at) throw std::runtime_error(name);
    }
    void InitializeText() override { Call("initialize-text"); }
    std::uint32_t CleanupText() override {
        ++text_cleanups;
        Call("cleanup-text");
        return 0x55u;
    }
    std::uint32_t EmptyText() override {
        Call("empty-prefix");
        return 0xdeadu;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t FormatName(std::uint32_t prefix,
                             std::string_view suffix) override {
        Check(prefix == 0xdeadu && format_index < 7u,
              "empty layout prefix fallback");
        Check(suffix == (format_index < 6u ? kNames[format_index]
                                            : "StaticControlPanel"),
              "seven formatted names follow IDA order");
        Call("format:" + std::string(suffix));
        return 0x6000u + format_index++;
    }
    std::uint32_t ToUtf8(std::uint32_t name) override {
        Check(name == 0x6000u + button_index, "button name conversion");
        Call("utf8");
        return name + 0x100u;
    }
    std::uint32_t FindWindowByName(std::uint32_t owner,
                                   std::uint32_t name) override {
        Check(owner == 0x1000u && name == 0x6100u + button_index,
              "UTF-8 button lookup");
        Call("find:" + std::string(kNames[button_index]));
        return 0x7000u + 0x200u * button_index;
    }
    std::uint32_t FindWindow(std::uint32_t owner,
                             std::uint32_t name) override {
        Check(owner == 0x1000u && name == 0x6006u,
              "panel lookup uses formatted name without UTF-8 conversion");
        Call("find-panel");
        return 0xa000u;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "click callback size");
        Call("allocate");
        return button_index == 1u ? 0u : 0x8000u + 0x40u * button_index;
    }
    std::uint32_t SubscribeClicked(std::uint32_t button,
                                   std::uint32_t callback) override {
        Check(button == 0x7000u + 0x200u * button_index &&
                  callback == (button_index == 1u ? 0u
                                                   : 0x8000u + 0x40u * button_index),
              "button and callback forwarded to CEGUI subscription");
        Call("subscribe");
        const auto index = button_index++;
        if (index == 2u) return 0u;
        const auto connection = 0x9000u + 0x20u * index;
        words[connection + 4u] = index == 3u ? 2u : 1u;
        return connection;
    }
    void DestroyConnection(std::uint32_t connection,
                           std::uint32_t flags) override {
        Check(flags == 1u, "temporary connection deleting flag");
        ++destroyed;
        calls.emplace_back("destroy:" + std::to_string(connection));
    }
    void SetAlwaysOnTop(std::uint32_t window, bool enabled) override {
        Check(enabled, "panel placed on top");
        top_window = window;
        Call("top");
    }
};

void CheckSuccess() {
    Runtime runtime;
    Check(pal4::CombatActionConsoleWindowElements::Setup(
              0x1000u, runtime) == 0x55u &&
              runtime.button_index == 6u && runtime.format_index == 7u &&
              runtime.text_cleanups == 1u && runtime.destroyed == 4u,
          "six buttons and panel, connection ownership, text cleanup");
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        const auto button = 0x7000u + 0x200u * index;
        Check(runtime.words[0x1150u + 4u * index] == button &&
                  runtime.words[button + 0x118u] == index,
              "button pointer and action index");
        if (index == 1u) continue;
        const auto callback = 0x8000u + 0x40u * index;
        Check(runtime.words[callback] == 0x845350u &&
                  runtime.words[callback + 8u] == 0x541480u &&
                  runtime.words[callback + 0xcu] == 0u &&
                  runtime.words[callback + 0x10u] == 0x1000u,
              "click callback layout");
    }
    Check(runtime.words[0x11e0u] == 0xa000u &&
              runtime.top_window == 0xa000u &&
              runtime.words[0x9064u] == 1u &&
              runtime.calls.back() == "cleanup-text",
          "panel is stored and raised; surviving connection decremented");
}

void CheckFailureCleanup() {
    Runtime runtime;
    runtime.fail_at = "find:BtnStunt";
    bool caught = false;
    try {
        (void)pal4::CombatActionConsoleWindowElements::Setup(
            0x1000u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.text_cleanups == 1u &&
              runtime.calls.back() == "cleanup-text" &&
              runtime.button_index == 2u,
          "lookup exception releases temporary text after completed buttons");
}

}  // namespace

int main() {
    try {
        CheckSuccess();
        CheckFailureCleanup();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
