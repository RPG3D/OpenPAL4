#include "pal4/pal_console_player_garb_command.h"

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

struct Runtime final : pal4::IPalConsolePlayerGarbRuntime {
    static constexpr std::uint32_t kManager = 0x1000u;
    std::vector<std::string> calls;
    std::array<bool, 3> reload{false, false, false};
    std::array<std::uint32_t, 3> old_state{1u, 1u, 1u};
    std::uint32_t active_index = 2u;
    bool allow_alloc = true;

    static std::uint32_t Player(std::uint32_t index) {
        return kManager + 2836u * index;
    }
    static std::uint32_t Index(std::uint32_t player) {
        Check(player >= kManager && (player - kManager) % 2836u == 0u,
              "player address stride");
        const auto index = (player - kManager) / 2836u;
        Check(index < 3u, "player index bound");
        return index;
    }
    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return kManager;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::uint32_t index) override {
        Check(manager == kManager && index < 3u, "player lookup operands");
        calls.emplace_back("player " + std::to_string(index));
        return Player(index);
    }
    void AssignModelText(std::uint32_t buffer,
                         std::string_view model) override {
        const auto index = Index(buffer - 0xa18u);
        Check(index < 2u && model == (index == 0u ? "101" : "103"),
              "first two model text operands");
        calls.emplace_back("assign " + std::string(model));
    }
    bool AllocateModelText(std::uint32_t buffer, std::uint32_t length,
                           std::uint32_t preserve) override {
        Check(buffer == Player(2u) + 0xa18u && length == 3u &&
                  preserve == 1u,
              "third model buffer allocation operands");
        calls.emplace_back("allocate 106");
        return allow_alloc;
    }
    void WriteModelText(std::uint32_t buffer,
                        std::string_view model) override {
        Check(buffer == Player(2u) + 0xa18u && model == "106",
              "third model text write operands");
        calls.emplace_back("write 106");
    }
    bool NeedsModelReload(std::uint32_t player) override {
        const auto index = Index(player);
        calls.emplace_back("compare " + std::to_string(index));
        return reload[index];
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        if (address == kManager + 0x2c50u) {
            calls.emplace_back("active index");
            return active_index;
        }
        const auto index = Index(address - 0x1f0u);
        calls.emplace_back("old state " + std::to_string(index));
        return old_state[index];
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0x3000u;
    }
    void ScriptOnRemove(std::uint32_t arena,
                        std::uint32_t player) override {
        Check(arena == 0x3000u, "arena remove address");
        calls.emplace_back("remove " + std::to_string(Index(player)));
    }
    void ProcessAndCleanup(std::uint32_t player) override {
        calls.emplace_back("cleanup " + std::to_string(Index(player)));
    }
    void SetStateFlag(std::uint32_t player,
                      std::uint32_t flag) override {
        Check(flag == old_state[Index(player)], "restore old state flag");
        calls.emplace_back("restore " + std::to_string(Index(player)));
    }
    void ScriptOnAdd(std::uint32_t arena,
                     std::uint32_t player) override {
        Check(arena == 0x3000u, "arena add address");
        calls.emplace_back("add " + std::to_string(Index(player)));
    }
    std::uint32_t ActorControl() override {
        calls.emplace_back("actor control");
        return 0x4000u;
    }
    void SetActor(std::uint32_t control,
                  std::uint32_t player) override {
        Check(control == 0x4000u && Index(player) == active_index,
              "active actor assignment");
        calls.emplace_back("set actor " + std::to_string(Index(player)));
    }
};

void UnchangedModels() {
    Runtime runtime;
    pal4::PalConsolePlayerGarbCommand::ApplyToAll(runtime);
    Check(runtime.calls == std::vector<std::string>{
              "manager", "player 0", "assign 101", "compare 0",
              "manager", "player 1", "assign 103", "compare 1",
              "manager", "player 2", "allocate 106", "write 106",
              "compare 2"},
          "three ordered model writes with no reload");
}

void ReloadAndActiveActor() {
    Runtime runtime;
    runtime.reload = {true, true, true};
    runtime.old_state = {0u, 5u, 7u};
    runtime.active_index = 2u;
    pal4::PalConsolePlayerGarbCommand::ApplyToAll(runtime);
    Check(runtime.calls == std::vector<std::string>{
              "manager", "player 0", "assign 101", "compare 0",
              "old state 0", "arena", "remove 0", "cleanup 0",
              "manager", "active index",
              "manager", "player 1", "assign 103", "compare 1",
              "old state 1", "arena", "remove 1", "cleanup 1",
              "restore 1", "arena", "add 1", "manager", "active index",
              "manager", "player 2", "allocate 106", "write 106",
              "compare 2", "old state 2", "arena", "remove 2",
              "cleanup 2", "restore 2", "arena", "add 2", "manager",
              "active index", "actor control", "set actor 2"},
          "reload, zero-state skip and current actor synchronization");
}

void FailedThirdAllocationStillCompares() {
    Runtime runtime;
    runtime.allow_alloc = false;
    runtime.reload[2] = true;
    runtime.active_index = 0u;
    pal4::PalConsolePlayerGarbCommand::ApplyToAll(runtime);
    Check(runtime.calls == std::vector<std::string>{
              "manager", "player 0", "assign 101", "compare 0",
              "manager", "player 1", "assign 103", "compare 1",
              "manager", "player 2", "allocate 106", "compare 2",
              "old state 2", "arena", "remove 2", "cleanup 2",
              "restore 2", "arena", "add 2", "manager", "active index"},
          "failed explicit allocation skips write but runs compare path");
}

}  // namespace

int main() {
    try {
        UnchangedModels();
        ReloadAndActiveActor();
        FailedThirdAllocationStillCompares();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
