#include "pal4/script_monster_effects.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptMonsterEffectsRuntime {
    std::vector<std::string> calls;
    std::uint32_t name = 0x9000u;
    std::uint32_t effect = 0xa000u;
    std::uint32_t monster = 0x5000u;
    std::int32_t argument = -7;

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x1004u) {
            calls.emplace_back("name");
            return name;
        }
        Check(address == 0x2004u, "monster effect record +4");
        calls.emplace_back("effect");
        return effect;
    }
    std::uint32_t EmptyString() override {
        calls.emplace_back("empty");
        return 0xe000u;
    }
    std::uint32_t MonsterManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t FindMonster(std::uint32_t manager,
                              std::uint32_t text) override {
        Check(manager == 0x4000u &&
                  text == (name == 0u ? 0xe000u : name),
              "monster manager and name");
        calls.emplace_back("find");
        return monster;
    }
    std::uint32_t AttachEffect(std::uint32_t found,
                               std::uint32_t text,
                               std::int32_t value) override {
        Check(found == 0x5000u &&
                  text == (effect == 0u ? 0xe000u : effect) &&
                  value == argument,
              "monster virtual +0x40 operands");
        calls.emplace_back("attach");
        return 0x87654321u;
    }
    std::uint32_t DetachEffect(std::uint32_t found) override {
        Check(found == 0x5000u, "monster virtual +0x44 receiver");
        calls.emplace_back("detach");
        return 0x12345678u;
    }
};

void Attach() {
    Runtime named;
    Check(pal4::ScriptMonsterEffects::Attach(
              0x1000u, 0x2000u, named.argument, named) == 0x87654321u &&
              named.calls == std::vector<std::string>{
                  "name", "manager", "find", "effect", "attach"},
          "attach resolves monster before effect text");
    Runtime empty;
    empty.name = 0u;
    empty.effect = 0u;
    Check(pal4::ScriptMonsterEffects::Attach(
              0x1000u, 0x2000u, empty.argument, empty) == 0x87654321u &&
              empty.calls == std::vector<std::string>{
                  "name", "empty", "manager", "find", "effect", "empty",
                  "attach"},
          "both missing text handles fall back to empty");
    Runtime missing;
    missing.monster = 0u;
    Check(pal4::ScriptMonsterEffects::Attach(
              0x1000u, 0x2000u, missing.argument, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "name", "manager", "find"},
          "missing monster avoids effect record access");
}

void Detach() {
    Runtime found;
    Check(pal4::ScriptMonsterEffects::Detach(0x1000u, found) ==
                  0x12345678u &&
              found.calls == std::vector<std::string>{
                  "name", "manager", "find", "detach"},
          "monster detach return forwarding");
    Runtime missing;
    missing.name = 0u;
    missing.monster = 0u;
    Check(pal4::ScriptMonsterEffects::Detach(0x1000u, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "name", "empty", "manager", "find"},
          "monster detach empty name and lookup miss");
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
