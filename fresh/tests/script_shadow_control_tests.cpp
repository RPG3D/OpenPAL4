#include "pal4/script_shadow_control.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptShadowRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> callback_returns;
    std::uint32_t context = 0xffffffffu;
    std::uint32_t traversal_result = 0xabcdu;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("players");
        return 0x1000u;
    }
    std::uint32_t ForAllPlayers(
        std::uint32_t manager, const PlayerCallback& callback,
        std::uint32_t enabled) override {
        Check(manager == 0x1000u, "player manager handle");
        context = enabled;
        calls.emplace_back("iterate");
        for (const auto player : {0u, 0x5000u, 0x6000u})
            callback_returns.push_back(callback(player, enabled));
        return traversal_result;
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0x2000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x2010u, "arena state offset");
        calls.emplace_back("arena-state");
        return 0x7000u;
    }
    void SetPlayerShadow(std::uint32_t player,
                         std::uint32_t enabled,
                         std::uint32_t arena_state) override {
        Check((player == 0x5000u || player == 0x6000u) &&
                  enabled == context && arena_state == 0x7000u,
              "player shadow operands");
        calls.emplace_back(player == 0x5000u ? "first-shadow" :
                                                "second-shadow");
    }
};

void Traversal() {
    for (const auto input : {std::uint8_t{0u}, std::uint8_t{1u},
                             std::uint8_t{0xffu}}) {
        Runtime runtime;
        Check(pal4::ScriptShadowControl::Enable(input, runtime) ==
                  runtime.traversal_result,
              "traversal result forwarded");
        Check(runtime.context == (input == 0u ? 0u : 1u),
              "script bool normalized to zero or one");
        Check(runtime.callback_returns ==
                  std::vector<std::uint32_t>{0u, 0x5000u, 0x6000u},
              "callback returns original player including null");
        Check(runtime.calls == std::vector<std::string>{
                  "players", "iterate", "arena", "arena-state",
                  "first-shadow", "arena", "arena-state", "second-shadow"},
              "arena is reacquired for each non-null player");
    }
}

void Callback() {
    Runtime runtime;
    Check(pal4::ScriptShadowControl::ApplyToPlayer(0u, 7u, runtime) == 0u &&
              runtime.calls.empty(), "null player avoids arena access");
    runtime.context = 7u;
    Check(pal4::ScriptShadowControl::ApplyToPlayer(
              0x5000u, 7u, runtime) == 0x5000u,
          "callback forwards full context and player");
    Check(runtime.calls == std::vector<std::string>{
              "arena", "arena-state", "first-shadow"},
          "standalone callback operation order");
}

}  // namespace

int main() {
    try {
        Traversal();
        Callback();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
