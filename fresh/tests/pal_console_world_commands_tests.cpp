#include "pal4/pal_console_world_commands.h"

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

struct Runtime final : pal4::IPalConsoleWorldRuntime {
    std::vector<std::string> calls;
    std::string output;
    std::uint32_t address = 0u;
    std::uint32_t value = 0u;
    std::int32_t weather = 0;

    void WriteDword(std::uint32_t target,
                    std::uint32_t data) override {
        calls.emplace_back("write");
        address = target;
        value = data;
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0x1000u;
    }
    std::int32_t SelectWeatherEffect(std::uint32_t arena,
                                     std::int32_t effect) override {
        Check(arena == 0x1000u, "weather arena address");
        calls.emplace_back("select weather");
        weather = effect;
        return 57;
    }
    std::uint8_t UpdateWeatherEffects(std::uint32_t arena) override {
        Check(arena == 0x1000u, "close weather arena address");
        calls.emplace_back("update weather");
        return 3u;
    }
    std::uint32_t Console() override {
        calls.emplace_back("console");
        return 0x2000u;
    }
    std::int32_t AddOutputLine(std::uint32_t console,
                               std::string_view format) override {
        Check(console == 0x2000u, "error console address");
        calls.emplace_back("output");
        output = format;
        return -7;
    }
};

void MazeVariables() {
    for (int index = 0; index < 5; ++index) {
        Runtime runtime;
        const auto input = std::to_string(index) + " -12";
        Check(pal4::PalConsoleWorldCommands::SetMazeVariable(
                  input.c_str(), runtime) == -12 &&
                  runtime.address == 0x90dec4u + 4u * index &&
                  runtime.value == static_cast<std::uint32_t>(-12) &&
                  runtime.calls == std::vector<std::string>{"write"},
              "five direct global words and signed return");
    }
    Runtime nonnumeric;
    Check(pal4::PalConsoleWorldCommands::SetMazeVariable(
              "text words", nonnumeric) == 0 &&
              nonnumeric.address == 0x90dec4u &&
              nonnumeric.value == 0u,
          "atoi zero selects first word");
    for (const char* input :
         std::array<const char*, 5>{nullptr, "0", "-1 7", "5 7",
                                    "2147483647 7"}) {
        Runtime runtime;
        Check(pal4::PalConsoleWorldCommands::SetMazeVariable(
                  input, runtime) == -7 &&
                  runtime.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd" &&
                  runtime.calls ==
                      std::vector<std::string>{"console", "output"},
              "missing or invalid maze index");
    }
}

void Weather() {
    for (const auto [input, expected] :
         {std::pair{"-3", -3}, std::pair{"text", 0},
          std::pair{"", 0}}) {
        Runtime runtime;
        Check(pal4::PalConsoleWorldCommands::OpenWeather(
                  input, runtime) == 57 &&
                  runtime.weather == expected &&
                  runtime.calls ==
                      std::vector<std::string>{"arena", "select weather"},
              "open weather passes atoi result through arena");
    }
    Runtime missing;
    Check(pal4::PalConsoleWorldCommands::OpenWeather(
              nullptr, missing) == -7 &&
              missing.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd" &&
              missing.calls == std::vector<std::string>{"console", "output"},
          "open weather missing argument");
    Runtime close;
    Check(pal4::PalConsoleWorldCommands::CloseWeather(close) == 3u &&
              close.calls ==
                  std::vector<std::string>{"arena", "update weather"},
          "close weather returns update result");
}

}  // namespace

int main() {
    try {
        MazeVariables();
        Weather();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
