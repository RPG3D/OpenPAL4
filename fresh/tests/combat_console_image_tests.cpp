#include "pal4/combat_console.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kConsole = 0x5000u;

struct Runtime final : pal4::ICombatConsoleImageRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kConsole + 224u + 2u * 4u, 0x8200u},
        {kConsole + 220u, 0x8100u},
        {kConsole + 236u + 2u * 4u, 0x9200u},
        {kConsole + 236u + 5u * 4u, 0x9500u}};
    std::vector<std::string> calls;
    std::uint32_t state = 0u;
    std::uint32_t visible = 0u;
    std::uint32_t image = 0u;
    std::u32string image_set;
    std::u32string image_name;
    std::string set_name = "zhangdouxiaotouxiang0";

    std::uint32_t GameState() override {
        calls.emplace_back("state");
        return state;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(words.count(address) != 0u, "image slot address");
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == image + 280u && value <= 1u,
              "image enable flag");
        words[address] = value;
        calls.emplace_back(value == 0u ? "disable" : "enable");
    }
    void SetChildVisible(std::uint32_t child, bool show) override {
        visible = child;
        calls.emplace_back(show ? "visible" : "hidden");
    }
    std::string ImageSetName() override {
        calls.emplace_back("set-name");
        return set_name;
    }
    void SetImage(std::uint32_t target, std::u32string_view set,
                  std::u32string_view name) override {
        image = target;
        image_set.assign(set);
        image_name.assign(name);
        calls.emplace_back("image");
    }
    std::uint32_t SetPosition(std::uint32_t target, std::uint32_t mode,
                              std::uint32_t rectangle) override {
        Check(target == image && mode == 1u &&
                  rectangle == kConsole + 44u,
              "image position and pixel mode");
        calls.emplace_back("position");
        return 0xabcdef01u;
    }
};

void EarlyGates() {
    Runtime blocked;
    blocked.state = 1u;
    Check(pal4::CombatConsoleWindow::SetImageAndPosition(
              kConsole, 2, "name", blocked) == 1u &&
              blocked.calls == std::vector<std::string>{"state"},
          "game state one skips image work");

    Runtime out_of_range;
    Check(pal4::CombatConsoleWindow::SetImageAndPosition(
              kConsole, 6, "name", out_of_range) == 6u &&
              out_of_range.calls == std::vector<std::string>{"state"},
          "visibility index six returns without image work");
}

void SharedIndexAndByteStrings() {
    Runtime runtime;
    runtime.state = 2u;
    runtime.set_name = std::string("atlas\xE9\0tail", 11u);
    const std::string source("A\xE9\0tail", 7u);
    Check(pal4::CombatConsoleWindow::SetImageAndPosition(
              kConsole, 2, source, runtime) == 0xabcdef01u &&
              runtime.visible == 0x8200u && runtime.image == 0x8200u &&
              runtime.image_set == U"atlas\u00e9" &&
              runtime.image_name == U"A\u00e9" &&
              runtime.words.at(0x8200u + 280u) == 1u,
          "one index selects the visible and configured image; bytes widen before NUL");
    Check(runtime.calls == std::vector<std::string>{
              "state", "read", "visible", "set-name",
              "image", "position", "enable"},
          "visibility, image, position and enable order");

    Runtime negative;
    Check(pal4::CombatConsoleWindow::SetImageAndPosition(
              kConsole, -1, "x", negative) == 0xabcdef01u &&
              negative.visible == 0x8100u && negative.image == 0x8100u,
          "signed upper-bound check admits negative index");
}

void HideElementAndSecondImageVariant() {
    Runtime hidden;
    hidden.image = 0x8200u;
    Check(pal4::CombatConsoleWindow::HideElement(kConsole, 2, hidden) ==
              0x8200u && hidden.visible == 0x8200u &&
              hidden.words.at(0x8200u + 280u) == 0u &&
              hidden.calls == std::vector<std::string>{
                  "read", "hidden", "disable"},
          "hide clears image flag and returns image");
    hidden.calls.clear();
    Check(pal4::CombatConsoleWindow::HideElement(kConsole, 3, hidden) == 0u &&
              hidden.calls.empty(),
          "index three skips hiding with defined zero return");
    hidden.image = 0x8100u;
    Check(pal4::CombatConsoleWindow::HideElement(kConsole, -1, hidden) ==
              0x8100u,
          "signed upper-bound check admits negative hide index");

    Runtime inactive;
    Check(pal4::CombatConsoleWindow::SetImageAndPositionSecond(
              kConsole, 2, "icon", inactive) == 0u &&
              inactive.calls == std::vector<std::string>{"state"},
          "second variant requires nonzero game state");
    Runtime too_large;
    too_large.state = 1u;
    Check(pal4::CombatConsoleWindow::SetImageAndPositionSecond(
              kConsole, 6, "icon", too_large) == 6u &&
              too_large.calls == std::vector<std::string>{"state"},
          "second variant's upper index gate");

    Runtime active;
    active.state = 1u;
    Check(pal4::CombatConsoleWindow::SetImageAndPositionSecond(
              kConsole, 2, "icon", active) == 0xabcdef01u &&
              active.visible == 0x9200u && active.image == 0x9200u &&
              active.image_set == U"zhangdouxiaotouxiang0" &&
              active.image_name == U"icon" &&
              active.words.at(0x9200u + 280u) == 1u,
          "second variant uses +236 image slots and same positioning path");

    Runtime second_hidden;
    second_hidden.image = 0x9500u;
    Check(pal4::CombatConsoleWindow::HideElementSecond(
              kConsole, 5, second_hidden) == 0x9500u &&
              second_hidden.visible == 0x9500u &&
              second_hidden.words.at(0x9500u + 280u) == 0u,
          "second hide uses +236 slot five");
    second_hidden.calls.clear();
    Check(pal4::CombatConsoleWindow::HideElementSecond(
              kConsole, 6, second_hidden) == 0u &&
              second_hidden.calls.empty(),
          "second hide stops at six with defined zero return");
}

}  // namespace

int main() {
    try {
        EarlyGates();
        SharedIndexAndByteStrings();
        HideElementAndSecondImageVariant();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
