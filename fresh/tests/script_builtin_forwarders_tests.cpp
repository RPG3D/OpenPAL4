#include "pal4/script_builtin_forwarders.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptPlayerLevelRuntime,
                       pal4::IScriptLogoNavigationRuntime,
                       pal4::IScriptCameraDistanceRuntime {
    std::uint32_t player = 0x4000u;
    std::uint32_t flag = 0xffffffffu;
    std::vector<std::string> calls;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager"); return 0x1000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x1000u && index == -2,
              "player index forwarded without clamping");
        calls.emplace_back("player"); return player;
    }
    std::int32_t ReadStat(std::uint32_t component,
                          std::uint32_t stat) override {
        Check(component == player + 0x654u && stat == 4u,
              "level reads stat four from player component");
        calls.emplace_back("stat"); return 63;
    }
    std::uint32_t GameInstance() override {
        calls.emplace_back("game"); return 0x9000u;
    }
    std::uint32_t EnterState(std::uint32_t game, std::uint32_t state,
                             std::uint32_t argument) override {
        Check(game == 0x9000u && state == 0u && argument == 0u,
              "logo state and both zero arguments");
        calls.emplace_back("enter"); return 0xabcdefu;
    }
    std::uint32_t SetDistanceOptimization(
        std::uint32_t enabled) override {
        flag = enabled;
        calls.emplace_back("distance"); return enabled + 7u;
    }
};

void Level() {
    Runtime found;
    Check(pal4::ScriptPlayerLevelQuery::Get(-2, found) == 63 &&
              found.calls == std::vector<std::string>{
                  "manager", "player", "stat"},
          "level found path");
    Runtime missing;
    missing.player = 0u;
    Check(pal4::ScriptPlayerLevelQuery::Get(-2, missing) == -1 &&
              missing.calls == std::vector<std::string>{"manager", "player"},
          "missing player returns minus one without stat read");
}

void LogoAndDistance() {
    Runtime runtime;
    Check(pal4::ScriptLogoNavigation::Go(runtime) == 0xabcdefu &&
              runtime.calls == std::vector<std::string>{"game", "enter"},
          "logo forwards state transition result");
    for (const auto input : {0u, 1u, 0x1234u}) {
        runtime.calls.clear();
        Check(pal4::ScriptCameraDistanceOptimization::Set(
                  input, runtime) == (input & 0xffu) + 7u &&
                  runtime.flag == (input & 0xffu) &&
                  runtime.calls == std::vector<std::string>{"distance"},
              "distance wrapper masks to byte and forwards result");
    }
}

}  // namespace

int main() {
    try {
        Level();
        LogoAndDistance();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
