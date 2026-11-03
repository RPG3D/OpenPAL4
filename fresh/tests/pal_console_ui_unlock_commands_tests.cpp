#include "pal4/pal_console_ui_unlock_commands.h"

#include <array>
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

struct Runtime final : pal4::IPalConsoleUiUnlockRuntime {
    std::vector<std::string> calls;
    std::string output;
    std::uint32_t entry = 99u;
    std::int32_t value = 99;

    std::uint32_t GameInstance() override {
        calls.emplace_back("game");
        return 0x1000u;
    }
    void SetGameDataEntry(std::uint32_t game, std::uint32_t key,
                          std::int32_t data) override {
        Check(game == 0x1000u, "game entry instance");
        calls.emplace_back("set entry");
        entry = key;
        value = data;
    }
    std::uint32_t Console() override {
        calls.emplace_back("console");
        return 0x2000u;
    }
    std::int32_t AddOutputLine(std::uint32_t console,
                               std::string_view format) override {
        Check(console == 0x2000u, "UI error console");
        calls.emplace_back("output");
        output = format;
        return -7;
    }
};

using Command = void (*)(const char*, pal4::IPalConsoleUiUnlockRuntime&);

void SystemEntries() {
    for (int index = 0; index <= 6; ++index) {
        Runtime runtime;
        const auto input = std::to_string(index) + " -5";
        pal4::PalConsoleUiUnlockCommands::OpenSystemUi(input.c_str(), runtime);
        Check(runtime.entry == static_cast<std::uint32_t>(index + 8) &&
                  runtime.value == -5 &&
                  runtime.calls ==
                      std::vector<std::string>{"game", "set entry"},
              "system index maps to game data entries eight through 14");
    }
    for (const char* input :
         std::array<const char*, 5>{nullptr, "0", "-1 1", "7 1",
                                    "2147483647 1"}) {
        Runtime runtime;
        pal4::PalConsoleUiUnlockCommands::OpenSystemUi(input, runtime);
        Check(runtime.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd" &&
                  runtime.calls ==
                      std::vector<std::string>{"console", "output"},
              "system index rejects missing and signed out-of-range fields");
    }
    Runtime zero;
    pal4::PalConsoleUiUnlockCommands::OpenSystemUi("text words", zero);
    Check(zero.entry == 8u && zero.value == 0,
          "system nonnumeric fields map to zero");
}

void FixedEntries() {
    for (const auto [command, entry] :
         {std::pair<Command, std::uint32_t>{
              &pal4::PalConsoleUiUnlockCommands::OpenMagicUi, 20u},
          std::pair<Command, std::uint32_t>{
              &pal4::PalConsoleUiUnlockCommands::OpenStuntUi, 19u}}) {
        Runtime success;
        command("-3", success);
        Check(success.entry == entry && success.value == -3 &&
                  success.calls ==
                      std::vector<std::string>{"game", "set entry"},
              "fixed UI entry accepts signed value");
        Runtime missing;
        command(nullptr, missing);
        Check(missing.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd" &&
                  missing.calls ==
                      std::vector<std::string>{"console", "output"},
              "fixed UI entry reports missing argument");
    }
}

}  // namespace

int main() {
    try {
        SystemEntries();
        FixedEntries();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
