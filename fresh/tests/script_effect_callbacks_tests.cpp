#include "pal4/script_effect_callbacks.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptEffectRuntime {
    std::uint32_t effect_name = 0x8000u;
    std::uint32_t npc_name = 0x9000u;
    std::uint32_t player = 0x6000u;
    std::uint32_t npc = 0x7000u;
    std::uint32_t current_index = 2u;
    std::uint32_t player_manager = 0x3000u;
    std::uint32_t expected_position = 0u;
    std::int32_t expected_key = 0;
    std::vector<std::string> calls;

    std::uint32_t EffectManager() override {
        calls.emplace_back("effect manager"); return 0x4000u;
    }
    std::uint32_t HandleScriptCallback(std::uint32_t manager,
                                       std::int32_t key) override {
        Check(manager == 0x4000u && key == expected_key,
              "effect callback key and manager");
        calls.emplace_back("callback"); return 0x81u;
    }
    std::uint32_t CleanupScriptCallback(std::uint32_t manager) override {
        Check(manager == 0x4000u, "effect cleanup manager");
        calls.emplace_back("cleanup"); return 0x82u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x1004u) {
            calls.emplace_back("effect name"); return effect_name;
        }
        if (address == 0x2004u) {
            calls.emplace_back("npc name"); return npc_name;
        }
        Check(address == player_manager + 0x2c50u,
              "current player index offset");
        calls.emplace_back("index"); return current_index;
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty"); return 0xa000u;
    }
    std::uint32_t PlayerManager() override {
        calls.emplace_back("player manager"); return player_manager;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == player_manager && index == -3,
              "indexed player lookup forwards signed index");
        calls.emplace_back("player"); return player;
    }
    std::uint32_t NpcManager() override {
        calls.emplace_back("npc manager"); return 0x5000u;
    }
    std::uint32_t FindNpc(std::uint32_t manager,
                          std::uint32_t name) override {
        Check(manager == 0x5000u && name ==
                  (npc_name == 0u ? 0xa000u : npc_name),
              "NPC lookup name fallback");
        calls.emplace_back("npc"); return npc;
    }
    pal4::ScriptEffectHandle CreateAtCoordinates(
        std::uint32_t manager, std::uint32_t name,
        const std::array<float, 3>& position,
        std::uint32_t argument, std::uint32_t flag) override {
        Check(manager == 0x4000u &&
                  name == (effect_name == 0u ? 0xa000u : effect_name) &&
                  position == std::array<float, 3>{1.25f, -2.5f, 3.75f} &&
                  argument == 0xb000u && flag == 0u,
              "world effect XYZ, name, argument and fixed flag");
        calls.emplace_back("create XYZ"); return {0x11u, 0x22u};
    }
    pal4::ScriptEffectHandle CreateAtAddress(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t position, std::uint32_t argument,
        std::uint32_t flag) override {
        Check(manager == 0x4000u &&
                  name == (effect_name == 0u ? 0xa000u : effect_name) &&
                  position == expected_position && argument == 0xb000u &&
                  flag == 0u,
              "actor effect name, position +0xac and fixed flag");
        calls.emplace_back("create actor"); return {0x11u, 0x22u};
    }
    std::uint32_t ResetHandle(pal4::ScriptEffectHandle& handle) override {
        Check(handle.first == 0x11u && handle.second == 0x22u,
              "temporary two-word effect handle");
        calls.emplace_back("reset");
        handle = {};
        return 0x83u;
    }
};

void CallbackKeys() {
    for (const auto [input, adjusted] : {
             std::pair{0, 1}, std::pair{15, 16}, std::pair{16, 15},
             std::pair{-1, 0},
             std::pair{std::numeric_limits<std::int32_t>::max(),
                       std::numeric_limits<std::int32_t>::min()}}) {
        Runtime runtime;
        runtime.expected_key = adjusted;
        Check(pal4::ScriptEffectCallbacks::PlayCallback(input, runtime) ==
                  0x81u &&
              runtime.calls == std::vector<std::string>{
                  "effect manager", "callback"},
              "effect callback increment, signed clamp and wrap");
    }
    Runtime stopped;
    Check(pal4::ScriptEffectCallbacks::StopCallback(stopped) == 0x82u &&
              stopped.calls == std::vector<std::string>{
                  "effect manager", "cleanup"},
          "effect stop callback forwards manager and result");
}

void CoordinateAndPlayerEffects() {
    Runtime world;
    world.effect_name = 0u;
    Check(pal4::ScriptEffectCallbacks::Create(
                  0x1000u, 0xb000u, 1.25f, -2.5f, 3.75f, world) == 0x83u &&
              world.calls == std::vector<std::string>{
                  "effect name", "empty", "effect manager",
                  "create XYZ", "reset"},
          "explicit XYZ effect releases temporary handle");
    Runtime indexed;
    indexed.expected_position = indexed.player + 0xacu;
    Check(pal4::ScriptEffectCallbacks::CreateOnPlayer(
                  0x1000u, 0xb000u, -3, indexed) == 0x83u &&
              indexed.calls == std::vector<std::string>{
                  "player manager", "player", "effect name",
                  "effect manager", "create actor", "reset"},
          "indexed player effect uses position subobject");
    Runtime missing;
    missing.player = 0u;
    Check(pal4::ScriptEffectCallbacks::CreateOnPlayer(
                  0x1000u, 0xb000u, -3, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "player manager", "player"},
          "missing indexed player does not read effect name");
    Runtime current;
    current.expected_position = 0x3000u + 2u * 0xb14u + 0xacu;
    Check(pal4::ScriptEffectCallbacks::CreateOnCurrentPlayer(
                  0x1000u, 0xb000u, current) == 0x83u &&
              current.calls == std::vector<std::string>{
                  "player manager", "index", "effect name",
                  "effect manager", "create actor", "reset"},
          "current player uses inline manager stride and position");
    Runtime zero_current;
    zero_current.player_manager = 0u;
    zero_current.current_index = 0u;
    Check(pal4::ScriptEffectCallbacks::CreateOnCurrentPlayer(
                  0x1000u, 0xb000u, zero_current) == 0u &&
              zero_current.calls == std::vector<std::string>{
                  "player manager", "index"},
          "zero inline current player skips effect lookup");
}

void NpcEffects() {
    Runtime found;
    found.npc_name = 0u;
    found.expected_position = found.npc + 0xacu;
    Check(pal4::ScriptEffectCallbacks::CreateOnNpc(
                  0x1000u, 0xb000u, 0x2000u, found) == 0x83u &&
              found.calls == std::vector<std::string>{
                  "npc name", "empty", "npc manager", "npc",
                  "effect name", "effect manager", "create actor", "reset"},
          "NPC lookup precedes effect name and uses fallback");
    Runtime missing;
    missing.npc = 0u;
    Check(pal4::ScriptEffectCallbacks::CreateOnNpc(
                  0x1000u, 0xb000u, 0x2000u, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "npc name", "npc manager", "npc"},
          "missing NPC skips effect creation");
}

}  // namespace

int main() {
    try {
        CallbackKeys();
        CoordinateAndPlayerEffects();
        NpcEffects();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
