#include "pal4/pal_console_master_skill_command.h"

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

struct Runtime final : pal4::IPalConsoleMasterSkillRuntime {
    enum class Mode { Magic, Stunt, Missing };
    Mode mode = Mode::Magic;
    std::int32_t player_id = 2;
    std::uint32_t current = 3u;
    std::uint32_t delta = 0u;
    std::vector<std::string> calls;
    std::vector<std::string> output;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x1000u;
    }
    std::int32_t PlayerId(std::uint32_t manager,
                          std::string_view name) override {
        Check(manager == 0x1000u && name == "hero", "player lookup");
        calls.emplace_back("id");
        return player_id;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t id) override {
        Check(manager == 0x1000u && id == player_id, "player index");
        calls.emplace_back("player");
        return 0x2000u;
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("database");
        return 0x3000u;
    }
    bool HasMagic(std::uint32_t lookup,
                  std::int32_t id) override {
        Check(lookup == 0x300cu && id == 17, "magic lookup args");
        calls.emplace_back("has-magic");
        return mode == Mode::Magic;
    }
    pal4::PalConsoleMagicRecord FindMagic(
        std::uint32_t lookup, std::int32_t id) override {
        Check(lookup == 0x300cu && id == 17, "magic find args");
        calls.emplace_back("find-magic");
        return {5u, 9u, "magic-name", "magic-description"};
    }
    bool HasStunt(std::uint32_t lookup,
                  std::int32_t id) override {
        Check(lookup == 0x300cu && id == 17, "stunt lookup args");
        calls.emplace_back("has-stunt");
        return mode == Mode::Stunt;
    }
    pal4::PalConsoleStuntRecord FindStunt(
        std::uint32_t lookup, std::int32_t id) override {
        Check(lookup == 0x300cu && id == 17, "stunt find args");
        calls.emplace_back("find-stunt");
        return {"stunt-name", "stunt-description"};
    }
    std::uint32_t CurrentValue(std::uint32_t collection,
                               std::uint32_t category,
                               std::int32_t id) override {
        Check(collection == 0x2938u && category == 5u && id == 17,
              "magic value operands");
        calls.emplace_back("current");
        return current;
    }
    void AddDelta(std::uint32_t collection,
                  std::uint32_t category, std::int32_t id,
                  std::uint32_t value) override {
        Check(collection == 0x2938u && category == 5u && id == 17,
              "delta operands");
        calls.emplace_back("delta");
        delta = value;
    }
    void InsertMagic(std::uint32_t collection,
                     std::uint32_t category,
                     std::int32_t id) override {
        Check(collection == 0x2938u && category == 5u && id == 17,
              "magic insertion operands");
        calls.emplace_back("insert-magic");
    }
    void InsertStunt(std::uint32_t collection,
                     std::int32_t id) override {
        Check(collection == 0x2938u && id == 17,
              "stunt insertion operands");
        calls.emplace_back("insert-stunt");
    }
    std::uint32_t Console() override {
        calls.emplace_back("console");
        return 0x4000u;
    }
    std::int32_t AddOutputLine(std::uint32_t console,
                               std::string_view format) override {
        Check(console == 0x4000u, "output console");
        calls.emplace_back("output");
        output.emplace_back(format);
        return -23;
    }
};

void MagicFillsRemainingValue() {
    Runtime runtime;
    Check(pal4::PalConsoleMasterSkillCommand::Execute(
              "hero 17 trailing", runtime) == -23 &&
              runtime.delta == 6u &&
              runtime.output == std::vector<std::string>{
                  "magic-name", "magic-description"} &&
              runtime.calls == std::vector<std::string>{
                  "manager", "id", "manager", "player", "database",
                  "has-magic", "database", "find-magic", "current",
                  "delta", "insert-magic", "console", "output",
                  "console", "output"},
          "magic fills max and prints both record strings");
    Runtime overfull;
    overfull.current = 12u;
    (void)pal4::PalConsoleMasterSkillCommand::Execute("hero 17", overfull);
    Check(overfull.delta == static_cast<std::uint32_t>(-3),
          "native unsigned subtraction wraps for an overfull value");
}

void StuntAndMissingSkill() {
    Runtime stunt;
    stunt.mode = Runtime::Mode::Stunt;
    Check(pal4::PalConsoleMasterSkillCommand::Execute(
              "hero 17", stunt) == -23 &&
              stunt.output == std::vector<std::string>{
                  "stunt-name", "stunt-description"} &&
              stunt.calls == std::vector<std::string>{
                  "manager", "id", "manager", "player", "database",
                  "has-magic", "database", "has-stunt", "database",
                  "find-stunt", "insert-stunt", "console", "output",
                  "console", "output"},
          "stunt path inserts once and prints both strings");

    Runtime missing;
    missing.mode = Runtime::Mode::Missing;
    Check(pal4::PalConsoleMasterSkillCommand::Execute(
              "hero 17", missing) == -23 &&
              missing.output == std::vector<std::string>{
                  "\xd5\xd2\xb2\xbb\xb5\xbd\xb6\xd4\xd3\xa6\xb5\xc4\xcf\xc9\xca\xf5\xbb\xf2\xcc\xd8\xbc\xbc"} &&
              missing.calls.back() == "output",
          "missing magic and stunt prints distinct GBK error");
}

void BadInputAndPlayer() {
    for (const char* input :
         std::vector<const char*>{nullptr, "hero"}) {
        Runtime runtime;
        Check(pal4::PalConsoleMasterSkillCommand::Execute(
                  input, runtime) == -23 &&
                  runtime.calls == std::vector<std::string>{
                      "console", "output"} &&
                  runtime.output[0] ==
                      "\xb4\xed\xce\xf3\xb2\xce\xca\xfd",
              "missing arguments print parameter error");
    }
    Runtime unknown;
    unknown.player_id = 4;
    Check(pal4::PalConsoleMasterSkillCommand::Execute(
              "hero 17", unknown) == -23 &&
              unknown.calls == std::vector<std::string>{
                  "manager", "id", "console", "output"},
          "unknown player skips database");
}

}  // namespace

int main() {
    try {
        MagicFillsRemainingValue();
        StuntAndMissingSkill();
        BadInputAndPlayer();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
