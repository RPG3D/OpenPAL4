#include "pal4/script_npc_effects.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptNpcEffectsRuntime {
    std::vector<std::string> calls;
    std::uint32_t name = 0x9000u;
    std::uint32_t effect = 0xa000u;
    std::uint32_t npc = 0x5000u;
    std::int32_t argument = -17;

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x1004u) {
            calls.emplace_back("name");
            return name;
        }
        Check(address == 0x2004u, "effect record +4");
        calls.emplace_back("effect");
        return effect;
    }
    std::uint32_t EmptyString() override {
        calls.emplace_back("empty");
        return 0xe000u;
    }
    std::uint32_t NpcManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t FindNpc(std::uint32_t manager,
                          std::uint32_t text) override {
        Check(manager == 0x4000u &&
                  text == (name == 0u ? 0xe000u : name),
              "NPC manager and name");
        calls.emplace_back("find");
        return npc;
    }
    std::uint32_t AttachEffect(std::uint32_t found,
                               std::uint32_t text,
                               std::int32_t value) override {
        Check(found == 0x5000u &&
                  text == (effect == 0u ? 0xe000u : effect) &&
                  value == argument,
              "NPC virtual +0x40 operands");
        calls.emplace_back("attach");
        return 0xdeadbeefu;
    }
    std::uint32_t DetachEffect(std::uint32_t found) override {
        Check(found == 0x5000u, "NPC virtual +0x44 receiver");
        calls.emplace_back("detach");
        return 0xcafebabeu;
    }
};

void Attach() {
    Runtime named;
    Check(pal4::ScriptNpcEffects::Attach(
              0x1000u, 0x2000u, named.argument, named) == 0xdeadbeefu &&
              named.calls == std::vector<std::string>{
                  "name", "manager", "find", "effect", "attach"},
          "attach resolves name before effect and forwards full result");

    Runtime empty;
    empty.name = 0u;
    empty.effect = 0u;
    Check(pal4::ScriptNpcEffects::Attach(
              0x1000u, 0x2000u, empty.argument, empty) == 0xdeadbeefu &&
              empty.calls == std::vector<std::string>{
                  "name", "empty", "manager", "find", "effect", "empty",
                  "attach"},
          "both null text handles use global empty string");

    Runtime missing;
    missing.npc = 0u;
    Check(pal4::ScriptNpcEffects::Attach(
              0x1000u, 0x2000u, missing.argument, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "name", "manager", "find"},
          "missing NPC avoids second record access");
}

void Detach() {
    Runtime found;
    Check(pal4::ScriptNpcEffects::Detach(0x1000u, found) == 0xcafebabeu &&
              found.calls == std::vector<std::string>{
                  "name", "manager", "find", "detach"},
          "detach tail call result forwarded");
    Runtime missing;
    missing.name = 0u;
    missing.npc = 0u;
    Check(pal4::ScriptNpcEffects::Detach(0x1000u, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "name", "empty", "manager", "find"},
          "detach null name fallback and lookup miss");
}

}  // namespace

int main() {
    try {
        Attach();
        Detach();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
