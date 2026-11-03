#include "pal4/script_player_garb.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptPlayerGarbRuntime {
    std::vector<std::string> calls;
    bool reload = false;
    bool allocated = true;
    std::uint32_t old_state = 7u;
    std::uint32_t active_index = 1u;
    std::int32_t last_index = -1;
    std::uint32_t last_player = 0u;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x1000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x1000u, "lookup manager");
        last_index = index;
        calls.emplace_back("player:" + std::to_string(index));
        last_player = 0x1000u + 0xb14u * static_cast<std::uint32_t>(index);
        return last_player;
    }
    void AssignModelText(std::uint32_t buffer,
                         std::string_view model) override {
        Check(buffer == last_player + 0xa18u, "assign buffer");
        calls.emplace_back("assign:" + std::string(model));
    }
    bool AllocateModelText(std::uint32_t buffer, std::uint32_t length,
                           std::uint32_t preserve) override {
        Check(buffer == last_player + 0xa18u && length == 3u &&
                  preserve == 1u, "allocate buffer arguments");
        calls.emplace_back("allocate");
        return allocated;
    }
    void WriteModelText(std::uint32_t buffer,
                        std::string_view model) override {
        Check(buffer == last_player + 0xa18u, "write buffer");
        calls.emplace_back("write:" + std::string(model));
    }
    bool NeedsModelReload(std::uint32_t player) override {
        Check(player == last_player, "reload player");
        calls.emplace_back("reload");
        return reload;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        if (address == last_player + 0x1f0u) {
            calls.emplace_back("old state");
            return old_state;
        }
        Check(address == 0x3c50u, "active index address");
        calls.emplace_back("active index");
        return active_index;
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0x9000u;
    }
    void ScriptOnRemove(std::uint32_t arena,
                        std::uint32_t player) override {
        Check(arena == 0x9000u && player == last_player, "remove operands");
        calls.emplace_back("remove");
    }
    void ProcessAndCleanup(std::uint32_t player) override {
        Check(player == last_player, "cleanup player");
        calls.emplace_back("cleanup");
    }
    void SetStateFlag(std::uint32_t player,
                      std::uint32_t flag) override {
        Check(player == last_player && flag == old_state, "restore state");
        calls.emplace_back("state");
    }
    void ScriptOnAdd(std::uint32_t arena,
                     std::uint32_t player) override {
        Check(arena == 0x9000u && player == last_player, "add operands");
        calls.emplace_back("add");
    }
    std::uint32_t ActorControl() override {
        calls.emplace_back("control");
        return 0xa000u;
    }
    void SetActor(std::uint32_t control,
                  std::uint32_t player) override {
        Check(control == 0xa000u && player == last_player,
              "active actor operands");
        calls.emplace_back("set actor");
    }

    void Select(std::int32_t index) {
        last_player = 0x1000u + 0xb14u * static_cast<std::uint32_t>(index);
    }
};

void SecondLookupBranches() {
    Runtime zero;
    zero.Select(0);
    pal4::ScriptPlayerGarb::ApplySecond(0, zero);
    Check(zero.calls == std::vector<std::string>{
              "manager", "player:0", "assign:102", "reload"},
          "Garb2 index zero reuses initial lookup");

    Runtime one;
    one.Select(1);
    one.reload = true;
    pal4::ScriptPlayerGarb::ApplySecond(1, one);
    Check(one.calls == std::vector<std::string>{
              "manager", "player:1", "manager", "player:1", "assign:104",
              "reload", "old state", "arena", "remove", "cleanup",
              "state", "arena", "add", "manager", "active index",
              "control", "set actor"},
          "Garb2 index one reloads current actor in original order");

    Runtime two;
    two.Select(2);
    two.allocated = false;
    two.reload = true;
    two.old_state = 0u;
    pal4::ScriptPlayerGarb::ApplySecond(2, two);
    Check(two.calls == std::vector<std::string>{
              "manager", "player:2", "manager", "player:2", "allocate",
              "reload", "old state", "arena", "remove", "cleanup",
              "manager", "active index"},
          "allocation failure still checks reload; zero state skips add");

    for (auto index : {-4, 3}) {
        Runtime invalid;
        pal4::ScriptPlayerGarb::ApplySecond(index, invalid);
        Check(invalid.calls == std::vector<std::string>{
                  "manager", "player:" + std::to_string(index)},
              "invalid Garb2 index performs first lookup only");
    }
}

void FirstLookupBranches() {
    for (std::int32_t index = 0; index < 3; ++index) {
        Runtime runtime;
        runtime.Select(index);
        pal4::ScriptPlayerGarb::ApplyFirst(index, runtime);
        const auto model = index == 0 ? "101" : index == 1 ? "103" : "106";
        const auto prefix = index == 2 ? "allocate" :
            "assign:" + std::string(model);
        const auto expected = index == 2 ? std::vector<std::string>{
            "manager", "player:2", "manager", "player:2", "allocate",
            "write:106", "reload"} : std::vector<std::string>{
            "manager", "player:" + std::to_string(index), "manager",
            "player:" + std::to_string(index), prefix, "reload"};
        Check(runtime.calls == expected,
              "Garb1 performs two lookups and selects its model");
    }
    Runtime invalid;
    pal4::ScriptPlayerGarb::ApplyFirst(4, invalid);
    Check(invalid.calls == std::vector<std::string>{"manager", "player:4"},
          "invalid Garb1 index performs only discarded lookup");
}

void AllPlayers() {
    Runtime runtime;
    pal4::ScriptPlayerGarb::ApplySecondToAll(runtime);
    Check(runtime.calls == std::vector<std::string>{
              "manager", "player:0", "assign:102", "reload",
              "manager", "player:1", "assign:104", "reload",
              "manager", "player:2", "allocate", "write:107", "reload"},
          "all-player Garb2 uses one lookup per index and correct models");

    Runtime changed;
    changed.reload = true;
    changed.active_index = 7u;
    pal4::ScriptPlayerGarb::ApplySecondToAll(changed);
    Check(changed.calls.size() == 40u,
          "all-player Garb2 reloads all three actors");
    for (const auto& call : changed.calls)
        Check(call != "control" && call != "set actor",
              "non-current players do not reset actor control");
}

}  // namespace

int main() {
    try {
        SecondLookupBranches();
        FirstLookupBranches();
        AllPlayers();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
