#include "pal4/script_npc_material.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptNpcMaterialRuntime {
    std::uint32_t npc_name = 0x4000u;
    std::uint32_t material_name = 0x5000u;
    std::uint32_t npc = 0x6000u;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x1004u) {
            calls.emplace_back("npc name");
            return npc_name;
        }
        Check(address == 0x2004u, "NPC material name offset");
        calls.emplace_back("material name");
        return material_name;
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty");
        return 0x3000u;
    }
    std::uint32_t NpcManager() override {
        calls.emplace_back("manager");
        return 0x7000u;
    }
    std::uint32_t FindNpc(std::uint32_t manager,
                           std::uint32_t name) override {
        Check(manager == 0x7000u &&
                  name == (npc_name == 0u ? 0x3000u : npc_name),
              "NPC material lookup operands");
        calls.emplace_back("find");
        return npc;
    }
    std::uint32_t SetMaterial(std::uint32_t actor,
                              std::uint32_t name) override {
        Check(actor == npc &&
                  name == (material_name == 0u ? 0x3000u : material_name),
              "NPC material setter operands");
        calls.emplace_back("set");
        return 0x81u;
    }
    std::uint32_t ApplyMaterial(std::uint32_t actor) override {
        Check(actor == npc, "NPC material apply operand");
        calls.emplace_back("apply");
        return 0x82u;
    }
};

void MaterialBranches() {
    Runtime set;
    set.npc_name = 0u;
    set.material_name = 0u;
    Check(pal4::ScriptNpcMaterial::Set(0x1000u, 0x2000u, set) == 0x81u &&
              set.calls == std::vector<std::string>{
                  "npc name", "empty", "manager", "find",
                  "material name", "empty", "set"},
          "NPC and material names each use empty fallback");
    Runtime apply;
    Check(pal4::ScriptNpcMaterial::Apply(0x1000u, apply) == 0x82u &&
              apply.calls == std::vector<std::string>{
                  "npc name", "manager", "find", "apply"},
          "NPC material apply forwards result");
    Runtime missing;
    missing.npc = 0u;
    Check(pal4::ScriptNpcMaterial::Set(
              0x1000u, 0x2000u, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "npc name", "manager", "find"},
          "missing NPC skips material string");
    Runtime missing_apply;
    missing_apply.npc = 0u;
    Check(pal4::ScriptNpcMaterial::Apply(
              0x1000u, missing_apply) == 0u &&
              missing_apply.calls == std::vector<std::string>{
                  "npc name", "manager", "find"},
          "missing NPC skips material apply");
}

}  // namespace

int main() {
    try {
        MaterialBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
