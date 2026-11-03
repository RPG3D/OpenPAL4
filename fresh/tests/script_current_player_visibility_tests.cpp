#include "pal4/script_current_player_visibility.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptCurrentPlayerVisibilityRuntime {
    std::uint32_t manager = 0x1000u;
    std::uint32_t index = 2u;
    std::uint32_t remove_result = 0x34u;
    std::uint32_t add_result = 0x56u;
    std::vector<std::string> calls;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager"); return manager;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == manager + 0x2c50u, "current index offset");
        calls.emplace_back("index"); return index;
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0x8000u + 0x100u * static_cast<std::uint32_t>(
            std::count(calls.begin(), calls.end(), "arena") - 1);
    }
    std::uint32_t ScriptOnRemove(std::uint32_t arena,
                                 std::uint32_t player) override {
        Check(arena == 0x8000u && player == manager + index * 0xb14u,
              "remove receives first arena and computed player");
        calls.emplace_back("remove"); return remove_result;
    }
    void SetStateFlag(std::uint32_t player, std::uint32_t state) override {
        Check(player == manager + index * 0xb14u && state == 1u,
              "visible state flag");
        calls.emplace_back("state");
    }
    std::uint32_t ScriptOnAdd(std::uint32_t arena,
                              std::uint32_t player) override {
        Check(arena == 0x8100u && player == manager + index * 0xb14u,
              "add receives second arena and computed player");
        calls.emplace_back("add"); return add_result;
    }
};

void VisibilityBranches() {
    Runtime hidden;
    Check(pal4::ScriptCurrentPlayerVisibility::Set(0u, hidden) ==
              hidden.remove_result &&
          hidden.calls == std::vector<std::string>{
              "manager", "index", "arena", "remove"},
          "hidden branch removes only and forwards its return");

    for (const auto visible : {1u, 2u, 255u}) {
        Runtime shown;
        Check(pal4::ScriptCurrentPlayerVisibility::Set(
                  static_cast<std::uint8_t>(visible), shown) ==
                  shown.add_result &&
              shown.calls == std::vector<std::string>{
                  "manager", "index", "arena", "remove", "state",
                  "arena", "add"},
              "nonzero visibility removes, sets flag, then adds");
    }
}

void ZeroComputedPlayer() {
    Runtime runtime;
    runtime.manager = 0u;
    runtime.index = 0u;
    Check(pal4::ScriptCurrentPlayerVisibility::Set(1u, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"manager", "index"},
          "zero computed player returns without arena calls");
}

}  // namespace

int main() {
    try {
        VisibilityBranches();
        ZeroComputedPlayer();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
