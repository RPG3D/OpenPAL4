#include "pal4/pal_console_player_stats_commands.h"

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

struct Runtime final : pal4::IPalConsolePlayerStatsRuntime {
    std::vector<std::string> calls;
    std::string output;
    std::int32_t player_id = 2;
    std::uint32_t category_seen = 0u;
    std::int32_t value_seen = -1;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x1000u;
    }
    std::int32_t PlayerId(std::uint32_t manager,
                          std::string_view name) override {
        Check(manager == 0x1000u && name == "hero", "player ID operands");
        calls.emplace_back("id");
        return player_id;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t id) override {
        Check(manager == 0x1000u && id == player_id,
              "player index operands");
        calls.emplace_back("player");
        return 0x2000u;
    }
    std::int32_t SetComponentValue(std::uint32_t component,
                                   std::uint32_t category,
                                   std::int32_t value) override {
        Check(component == 0x2654u, "player component +0x654");
        calls.emplace_back("set");
        category_seen = category;
        value_seen = value;
        return 41;
    }
    std::uint32_t Console() override {
        calls.emplace_back("console");
        return 0x3000u;
    }
    std::int32_t AddOutputLine(std::uint32_t console,
                               std::string_view format) override {
        Check(console == 0x3000u, "error output console");
        calls.emplace_back("output");
        output = format;
        return -7;
    }
};

using Command = std::int32_t (*)(
    const char*, pal4::IPalConsolePlayerStatsRuntime&);

struct Case {
    Command command;
    std::uint32_t category;
    std::string_view range_error;
};

const std::array<Case, 3> kCases{{
    {&pal4::PalConsolePlayerStatsCommands::SetHp, 7u,
     "\xd6\xf7\xbd\xc7\x48\x70\xb3\xac\xb3\xf6\xb7\xb6\xce\xa7"},
    {&pal4::PalConsolePlayerStatsCommands::SetMp, 9u,
     "\xd6\xf7\xbd\xc7\x4d\x70\xb3\xac\xb3\xf6\xb7\xb6\xce\xa7"},
    {&pal4::PalConsolePlayerStatsCommands::SetRage, 8u,
     "\xd6\xf7\xbd\xc7\x52\x61\x67\x65\xb3\xac\xb3\xf6\xb7\xb6\xce\xa7"},
}};

void SuccessfulValues() {
    for (const auto& item : kCases) {
        Runtime runtime;
        Check(item.command("hero 25 extra", runtime) == 41 &&
                  runtime.category_seen == item.category &&
                  runtime.value_seen == 25 &&
                  runtime.calls == std::vector<std::string>{
                      "manager", "id", "manager", "player", "set"},
              "stat setter category, value and call order");

        Runtime zero;
        Check(item.command("hero nonnumeric", zero) == 41 &&
                  zero.value_seen == 0,
              "atoi zero is accepted");
    }
}

void NegativeValues() {
    for (const auto& item : kCases) {
        Runtime runtime;
        Check(item.command("hero -1", runtime) == -7 &&
                  runtime.output == item.range_error &&
                  runtime.calls == std::vector<std::string>{
                      "manager", "id", "manager", "player",
                      "console", "output"},
              "negative stat uses its distinct original GBK error");
    }
}

void MissingOrUnknownPlayer() {
    for (const auto& item : kCases) {
        for (const char* input :
             std::vector<const char*>{nullptr, "hero"}) {
            Runtime runtime;
            Check(item.command(input, runtime) == -7 &&
                      runtime.output ==
                          "\xb4\xed\xce\xf3\xb2\xce\xca\xfd" &&
                      runtime.calls == std::vector<std::string>{
                          "console", "output"},
                  "missing fields skip player manager");
        }
        Runtime unknown;
        unknown.player_id = 4;
        Check(item.command("hero 1", unknown) == -7 &&
                  unknown.calls == std::vector<std::string>{
                      "manager", "id", "console", "output"},
              "ID four reports parameter error");
    }
}

}  // namespace

int main() {
    try {
        SuccessfulValues();
        NegativeValues();
        MissingOrUnknownPlayer();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
