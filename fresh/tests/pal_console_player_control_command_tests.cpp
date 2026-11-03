#include "pal4/pal_console_player_control_command.h"

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

struct Runtime final : pal4::IPalConsolePlayerControlRuntime {
    static constexpr std::uint32_t kManager = 0x1000u;
    static constexpr std::uint32_t kPlayer = kManager + 2836u * 2u;
    std::vector<std::string> calls;
    std::int32_t id = 3;
    std::string output;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("player manager");
        return kManager;
    }
    std::int32_t PlayerId(std::uint32_t manager,
                          std::string_view name) override {
        Check(manager == kManager && name == "hero", "player ID operands");
        calls.emplace_back("player ID");
        return id;
    }
    void UpdatePlayerStates(std::uint32_t manager,
                            std::int32_t selected_id) override {
        Check(manager == kManager && selected_id == id,
              "player state operands");
        calls.emplace_back("update player states");
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        Check(address == kManager + 0x2c50u, "active player index offset");
        calls.emplace_back("active index");
        return 2u;
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena manager");
        return 0x3000u;
    }
    void ScriptOnRemove(std::uint32_t arena,
                        std::uint32_t player) override {
        Check(arena == 0x3000u && player == kPlayer,
              "arena remove operands");
        calls.emplace_back("remove");
    }
    void SetStateFlag(std::uint32_t player,
                      std::uint32_t flag) override {
        Check(player == kPlayer && flag == 1u, "state flag operands");
        calls.emplace_back("state flag");
    }
    void ScriptOnAdd(std::uint32_t arena,
                     std::uint32_t player) override {
        Check(arena == 0x3000u && player == kPlayer, "arena add operands");
        calls.emplace_back("add");
    }
    std::uint32_t ActorControl() override {
        calls.emplace_back("actor control");
        return 0x4000u;
    }
    void SetActor(std::uint32_t control,
                  std::uint32_t player) override {
        Check(control == 0x4000u && player == kPlayer,
              "set actor operands");
        calls.emplace_back("set actor");
    }
    std::int32_t ResetAndSync(std::uint32_t control,
                              std::uint32_t enabled) override {
        Check(control == 0x4000u && enabled == 1u,
              "reset actor operands");
        calls.emplace_back("reset and sync");
        return 77;
    }
    std::uint32_t Console() override {
        calls.emplace_back("console");
        return 0x5000u;
    }
    std::int32_t AddOutputLine(std::uint32_t console,
                               std::string_view format) override {
        Check(console == 0x5000u, "console operand");
        calls.emplace_back("output");
        output = format;
        return -7;
    }
};

void SuccessfulControlChange() {
    Runtime runtime;
    Check(pal4::PalConsolePlayerControlCommand::Execute("hero", runtime) ==
              77 &&
              runtime.calls == std::vector<std::string>{
                  "player manager", "player ID", "player manager",
                  "update player states", "player manager", "active index",
                  "arena manager", "remove", "state flag", "arena manager",
                  "add", "actor control", "set actor", "actor control",
                  "reset and sync"},
          "control change native order and return value");
}

void Errors() {
    Runtime absent;
    Check(pal4::PalConsolePlayerControlCommand::Execute(nullptr, absent) ==
              -7 &&
              absent.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd" &&
              absent.calls == std::vector<std::string>{"console", "output"},
          "missing player argument");
    Runtime invalid;
    invalid.id = 4;
    Check(pal4::PalConsolePlayerControlCommand::Execute("hero", invalid) ==
              -7 &&
              invalid.output == "\xb4\xed\xce\xf3\xb2\xce\xca\xfd" &&
              invalid.calls == std::vector<std::string>{
                  "player manager", "player ID", "console", "output"},
          "invalid ID aborts before state update");
}

}  // namespace

int main() {
    try {
        SuccessfulControlChange();
        Errors();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
