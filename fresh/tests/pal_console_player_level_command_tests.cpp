#include "pal4/pal_console_player_level_command.h"

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

struct Runtime final : pal4::IPalConsolePlayerLevelRuntime {
    std::vector<std::string> calls;
    std::vector<std::string> output;
    std::vector<std::string> new_skills;
    std::int32_t id = 2;
    std::int32_t level_set = -1;
    bool fail_skill_output = false;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x1000u;
    }
    std::int32_t PlayerId(std::uint32_t manager,
                          std::string_view name) override {
        Check(manager == 0x1000u && name == "hero", "player ID lookup");
        calls.emplace_back("id");
        return id;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t value) override {
        Check(manager == 0x1000u && value == id, "player index lookup");
        calls.emplace_back("player");
        return 0x2000u;
    }
    void SetComponentValue(std::uint32_t component,
                           std::uint32_t category,
                           std::int32_t level) override {
        Check(component == 0x2654u && category == 4u,
              "component virtual slot arguments");
        calls.emplace_back("set-level");
        level_set = level;
    }
    std::string PlayerName(std::uint32_t player) override {
        Check(player == 0x2000u, "player name address");
        calls.emplace_back("name");
        return "hero-name";
    }
    void InitializeFormatBuffer() override {
        calls.emplace_back("init-format");
    }
    std::int32_t CleanupFormatBuffer() override {
        calls.emplace_back("cleanup-format");
        return 51;
    }
    std::uint32_t Console() override {
        calls.emplace_back("console");
        return 0x3000u;
    }
    std::int32_t AddOutputLine(std::uint32_t console,
                               std::string_view text) override {
        Check(console == 0x3000u, "output console");
        calls.emplace_back("output");
        output.emplace_back(text);
        if (fail_skill_output && output.size() == 2u)
            throw std::runtime_error("output failed");
        return -7;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        Check(address == 0x2a48u, "player game-data offset");
        calls.emplace_back("read-data");
        return 0x4000u;
    }
    void UpdateLevelEvent(std::uint32_t component,
                          std::int32_t level,
                          std::uint32_t game_data) override {
        Check(component == 0x2654u && level == level_set &&
                  game_data == 0x4000u,
              "level event operands");
        calls.emplace_back("event");
    }
    std::vector<std::string> BuildNewMatchingRecords(
        std::uint32_t collection, std::int32_t level,
        std::uint32_t game_data) override {
        Check(collection == 0x2938u && level == level_set &&
                  game_data == 0x4000u,
              "matching-record operands");
        calls.emplace_back("records");
        return new_skills;
    }
    void ReleaseMatchingRecords() override {
        calls.emplace_back("release-records");
    }
};

void MissingAndUnknownPlayer() {
    for (const char* input :
         std::vector<const char*>{nullptr, "hero"}) {
        Runtime runtime;
        Check(pal4::PalConsolePlayerLevelCommand::Execute(input, runtime) ==
                  -7 &&
                  runtime.calls == std::vector<std::string>{
                      "console", "output"} &&
                  runtime.output[0] ==
                      "\xb4\xed\xce\xf3\xb2\xce\xca\xfd",
              "missing fields print parameter error");
    }
    Runtime unknown;
    unknown.id = 4;
    Check(pal4::PalConsolePlayerLevelCommand::Execute(
              "hero 5", unknown) == -7 &&
              unknown.calls == std::vector<std::string>{
                  "manager", "id", "console", "output"},
          "unknown player stops before player fetch");
}

void LevelRange() {
    for (const char* input : {"hero -1", "hero 100"}) {
        Runtime runtime;
        Check(pal4::PalConsolePlayerLevelCommand::Execute(
                  input, runtime) == -7 &&
                  runtime.calls == std::vector<std::string>{
                      "manager", "id", "manager", "player",
                      "console", "output"} &&
                  runtime.output[0] ==
                      "\xd6\xf7\xbd\xc7\xb5\xc8\xbc\xb6\xb3\xac\xb3\xf6\xb7\xb6\xce\xa7",
              "signed out-of-range level uses separate GBK error");
    }
}

void SuccessfulLevelAndSkills() {
    Runtime runtime;
    runtime.new_skills = {"skill-a", "skill-b"};
    Check(pal4::PalConsolePlayerLevelCommand::Execute(
              "hero 99 extra", runtime) == 51 &&
              runtime.level_set == 99 &&
              runtime.output == std::vector<std::string>{
                  "hero-name \xc9\xfd\xbc\xb6\xd6\xc1 99 \xbc\xb6",
                  "hero-name \xd1\xa7\xbb\xe1\xcc\xd8\xbc\xbc skill-a",
                  "hero-name \xd1\xa7\xbb\xe1\xcc\xd8\xbc\xbc skill-b"},
          "level and every new skill use native GBK format");
    Check(runtime.calls == std::vector<std::string>{
              "manager", "id", "manager", "player", "set-level",
              "init-format", "name", "console", "output", "read-data",
              "event", "read-data", "records", "name", "console",
              "output", "name", "console", "output", "release-records",
              "cleanup-format"},
          "component, event, record and cleanup order");

    Runtime zero;
    Check(pal4::PalConsolePlayerLevelCommand::Execute("hero 0", zero) ==
              51 && zero.level_set == 0 &&
              zero.calls.back() == "cleanup-format",
          "zero level accepted and empty record array released");
}

void OutputFailureCleansUp() {
    Runtime runtime;
    runtime.new_skills = {"skill-a"};
    runtime.fail_skill_output = true;
    try {
        (void)pal4::PalConsolePlayerLevelCommand::Execute("hero 1", runtime);
        throw std::runtime_error("expected output failure");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) == "output failed",
              "original output exception propagates");
    }
    Check(runtime.calls.size() >= 2u &&
              runtime.calls[runtime.calls.size() - 2u] ==
                  "release-records" &&
              runtime.calls.back() == "cleanup-format",
          "record and formatted-string cleanup during unwind");
}

}  // namespace

int main() {
    try {
        MissingAndUnknownPlayer();
        LevelRange();
        SuccessfulLevelAndSkills();
        OutputFailureCleansUp();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
