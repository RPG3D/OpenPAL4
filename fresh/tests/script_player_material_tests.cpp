#include "pal4/script_player_material.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptPlayerMaterialRuntime {
    std::uint32_t manager = 0x1000u;
    std::uint32_t indexed_player = 0x5000u;
    std::uint32_t expected_player = 0x5000u;
    std::uint32_t name = 0x6000u;
    std::uint32_t active_index = 1u;
    std::int32_t expected_index = -3;
    std::vector<std::string> calls;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return manager;
    }
    std::uint32_t PlayerByIndex(std::uint32_t found_manager,
                                std::int32_t index) override {
        Check(found_manager == manager && index == expected_index,
              "material indexed player lookup");
        calls.emplace_back("player");
        return indexed_player;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == manager + 0x2c50u) {
            calls.emplace_back("active index");
            return active_index;
        }
        Check(address == 0x2004u, "material name payload offset");
        calls.emplace_back("name");
        return name;
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty");
        return 0x4000u;
    }
    std::uint32_t SetMaterial(std::uint32_t player,
                              std::uint32_t resolved_name) override {
        Check(player == expected_player &&
                  resolved_name == (name == 0u ? 0x4000u : name),
              "set material operands");
        calls.emplace_back("set");
        return 0x81u;
    }
    std::uint32_t ApplyMaterial(std::uint32_t player) override {
        Check(player == expected_player, "apply material player");
        calls.emplace_back("apply");
        return 0x82u;
    }
    std::uint32_t SetLegMaterial(std::uint32_t player,
                                 std::uint32_t resolved_name) override {
        Check(player == expected_player &&
                  resolved_name == (name == 0u ? 0x4000u : name),
              "set LingSha leg material operands");
        calls.emplace_back("set legs");
        return 0x83u;
    }
    std::uint32_t ApplyLegMaterial(std::uint32_t player) override {
        Check(player == expected_player, "apply LingSha leg material player");
        calls.emplace_back("apply legs");
        return 0x84u;
    }
};

void IndexedBranches() {
    Runtime set;
    set.name = 0u;
    Check(pal4::ScriptPlayerMaterial::Set(-3, 0x2000u, set) == 0x81u &&
              set.calls == std::vector<std::string>{
                  "manager", "player", "name", "empty", "set"},
          "indexed set resolves null material after player lookup");
    Runtime apply;
    Check(pal4::ScriptPlayerMaterial::Apply(-3, apply) == 0x82u &&
              apply.calls == std::vector<std::string>{
                  "manager", "player", "apply"},
          "indexed apply forwards result");
    Runtime missing;
    missing.indexed_player = 0u;
    Check(pal4::ScriptPlayerMaterial::Set(-3, 0x2000u, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "manager", "player"},
          "missing indexed player skips material string");
    Runtime missing_apply;
    missing_apply.indexed_player = 0u;
    Check(pal4::ScriptPlayerMaterial::Apply(-3, missing_apply) == 0u &&
              missing_apply.calls == std::vector<std::string>{
                  "manager", "player"},
          "missing indexed player skips apply");
}

void CurrentBranches() {
    Runtime set;
    set.expected_player = set.manager + 0xb14u;
    Check(pal4::ScriptPlayerMaterial::SetCurrent(0x2000u, set) == 0x81u &&
              set.calls == std::vector<std::string>{
                  "manager", "active index", "name", "set"},
          "current set uses manager stride without indexed lookup");
    Runtime apply;
    apply.expected_player = apply.manager + 0xb14u;
    Check(pal4::ScriptPlayerMaterial::ApplyCurrent(apply) == 0x82u &&
              apply.calls == std::vector<std::string>{
                  "manager", "active index", "apply"},
          "current apply uses inline player");
    Runtime zero;
    zero.manager = 0u;
    zero.active_index = 0u;
    Check(pal4::ScriptPlayerMaterial::SetCurrent(0x2000u, zero) == 0u &&
              zero.calls == std::vector<std::string>{
                  "manager", "active index"},
          "zero current player skips material name");
}

void LingshaBranches() {
    Runtime set;
    set.expected_index = 1;
    set.name = 0u;
    Check(pal4::ScriptPlayerMaterial::SetLingshaLegs(
              0x2000u, set) == 0x83u &&
              set.calls == std::vector<std::string>{
                  "manager", "player", "name", "empty", "set legs"},
          "LingSha legs use fixed index one and special setter");
    Runtime apply;
    apply.expected_index = 1;
    Check(pal4::ScriptPlayerMaterial::ApplyLingshaLegs(apply) == 0x84u &&
              apply.calls == std::vector<std::string>{
                  "manager", "player", "apply legs"},
          "LingSha legs use special apply call");
    Runtime missing;
    missing.expected_index = 1;
    missing.indexed_player = 0u;
    Check(pal4::ScriptPlayerMaterial::SetLingshaLegs(
              0x2000u, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "manager", "player"},
          "missing LingSha skips material name");
    Runtime missing_apply;
    missing_apply.expected_index = 1;
    missing_apply.indexed_player = 0u;
    Check(pal4::ScriptPlayerMaterial::ApplyLingshaLegs(
              missing_apply) == 0u &&
              missing_apply.calls == std::vector<std::string>{
                  "manager", "player"},
          "missing LingSha skips special apply");
}

}  // namespace

int main() {
    try {
        IndexedBranches();
        CurrentBranches();
        LingshaBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
