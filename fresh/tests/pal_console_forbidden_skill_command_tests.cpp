#include "pal4/pal_console_forbidden_skill_command.h"

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

struct Runtime final : pal4::IPalConsoleForbiddenSkillRuntime {
    std::vector<std::string> calls;
    std::vector<std::int32_t> values{3, -2};
    std::size_t position = 0u;
    std::int32_t id = 2;
    std::uint32_t player = 0x2000u;
    std::int32_t skill = 0;
    bool present = false;
    bool fail_value = false;
    std::string output;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x1000u;
    }
    std::int32_t PlayerId(std::uint32_t manager,
                          std::string_view name) override {
        Check(manager == 0x1000u && name == "hero", "player ID operands");
        calls.emplace_back("id");
        return id;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x1000u && index == id,
              "player lookup operands");
        calls.emplace_back("player");
        return player;
    }
    void SetPresent(std::uint32_t collection, std::int32_t item,
                    bool enabled) override {
        Check(collection == player + 0x938u,
              "forbidden-skill collection offset");
        calls.emplace_back("set present");
        skill = item;
        present = enabled;
    }
    std::string_view PlayerName(std::uint32_t subject) override {
        Check(subject == player, "player name address");
        calls.emplace_back("name");
        return "Yun";
    }
    void InitializeFormatBuffer(std::string_view name) override {
        Check(name == "Yun", "format starts with player name");
        calls.emplace_back("init buffer");
    }
    std::uint32_t CreateIterator(std::uint32_t collection) override {
        Check(collection == player + 0x938u,
              "forbidden-skill iterator collection");
        calls.emplace_back("create iterator");
        return 0x4000u;
    }
    bool IteratorDone(std::uint32_t iterator) override {
        Check(iterator == 0x4000u, "iterator done address");
        calls.emplace_back("done");
        return position >= values.size();
    }
    std::int32_t IteratorValue(std::uint32_t iterator) override {
        Check(iterator == 0x4000u && position < values.size(),
              "iterator value address");
        calls.emplace_back("value");
        if (fail_value) throw std::runtime_error("value failed");
        return values[position];
    }
    void IteratorNext(std::uint32_t iterator) override {
        Check(iterator == 0x4000u, "iterator next address");
        calls.emplace_back("next");
        ++position;
    }
    void DestroyIterator(std::uint32_t iterator,
                         std::uint32_t flags) override {
        Check(iterator == 0x4000u && flags == 1u,
              "iterator deleting destructor operands");
        calls.emplace_back("destroy iterator");
    }
    std::int32_t CleanupFormatBuffer() override {
        calls.emplace_back("cleanup buffer");
        return 57;
    }
    std::uint32_t Console() override {
        calls.emplace_back("console");
        return 0x5000u;
    }
    std::int32_t AddOutputLine(std::uint32_t console,
                               std::string_view format) override {
        Check(console == 0x5000u, "forbidden-skill console");
        calls.emplace_back("output");
        output = format;
        return -7;
    }
};

void EnabledAndListed() {
    Runtime runtime;
    Check(pal4::PalConsoleForbiddenSkillCommand::Execute(
              "hero -9 2", runtime) == 57 &&
              runtime.skill == -9 && runtime.present &&
              runtime.output == "Yun ForbidenSkill:3\t,-2\t," &&
              runtime.calls == std::vector<std::string>{
                  "manager", "id", "manager", "player", "set present",
                  "name", "init buffer", "create iterator", "done",
                  "value", "next", "done", "value", "next", "done",
                  "console", "output", "destroy iterator", "cleanup buffer"},
          "skill toggle, listing format, iterator and buffer lifetime");
    Runtime disabled;
    disabled.values.clear();
    Check(pal4::PalConsoleForbiddenSkillCommand::Execute(
              "hero 8 -1", disabled) == 57 &&
              !disabled.present &&
              disabled.output == "Yun ForbidenSkill:" &&
              disabled.calls == std::vector<std::string>{
                  "manager", "id", "manager", "player", "set present",
                  "name", "init buffer", "create iterator", "done",
                  "console", "output", "destroy iterator", "cleanup buffer"},
          "nonpositive flag removes skill and empty set still prints header");
}

void ErrorsAndNullPlayer() {
    for (const char* input :
         std::array<const char*, 3>{nullptr, "hero", "hero 1"}) {
        Runtime runtime;
        Check(pal4::PalConsoleForbiddenSkillCommand::Execute(
                  input, runtime) == -7 &&
                  runtime.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd" &&
                  runtime.calls == std::vector<std::string>{
                      "console", "output"},
              "missing fields skip player lookup");
    }
    Runtime unknown;
    unknown.id = 4;
    Check(pal4::PalConsoleForbiddenSkillCommand::Execute(
              "hero 1 1", unknown) == -7 &&
              unknown.calls == std::vector<std::string>{
                  "manager", "id", "console", "output"},
          "unknown player ID four reports argument error");
    Runtime absent;
    absent.player = 0u;
    Check(pal4::PalConsoleForbiddenSkillCommand::Execute(
              "hero 1 1", absent) == 0 &&
              absent.calls == std::vector<std::string>{
                  "manager", "id", "manager", "player"},
          "null player returns safely before native name dereference");
}

void IteratorFailureReleasesResources() {
    Runtime runtime;
    runtime.fail_value = true;
    try {
        (void)pal4::PalConsoleForbiddenSkillCommand::Execute(
            "hero 1 1", runtime);
        throw std::runtime_error("expected value failure");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) == "value failed",
              "iterator failure propagates");
    }
    Check(runtime.calls == std::vector<std::string>{
              "manager", "id", "manager", "player", "set present",
              "name", "init buffer", "create iterator", "done", "value",
              "destroy iterator", "cleanup buffer"},
          "iterator exception releases iterator and format buffer");
}

}  // namespace

int main() {
    try {
        EnabledAndListed();
        ErrorsAndNullPlayer();
        IteratorFailureReleasesResources();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
