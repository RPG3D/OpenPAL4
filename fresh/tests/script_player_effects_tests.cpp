#include "pal4/script_player_effects.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptPlayerEffectsRuntime {
    std::vector<std::string> calls;
    std::int32_t expected_index = -3;
    std::int32_t expected_argument = -29;
    std::uint32_t player = 0x5000u;
    std::uint32_t effect = 0xa000u;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x4000u && index == expected_index,
              "player lookup preserves signed index");
        calls.emplace_back("player");
        return player;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x2004u, "effect record word one");
        calls.emplace_back("effect");
        return effect;
    }
    std::uint32_t EmptyString() override {
        calls.emplace_back("empty");
        return 0xe000u;
    }
    std::uint32_t AttachEffect(std::uint32_t receiver,
                               std::uint32_t text,
                               std::int32_t argument) override {
        Check(receiver == 0x5000u &&
                  text == (effect == 0u ? 0xe000u : effect) &&
                  argument == expected_argument,
              "player virtual +0x40 operands");
        calls.emplace_back("attach");
        return 0x87654321u;
    }
    std::uint32_t DetachEffect(std::uint32_t receiver) override {
        Check(receiver == 0x5000u, "player virtual +0x44 receiver");
        calls.emplace_back("detach");
        return 0x12345678u;
    }
};

void Hook() {
    Runtime named;
    Check(pal4::ScriptPlayerEffects::Hook(
              -3, 0x2000u, -29, named) == 0x87654321u &&
              named.calls == std::vector<std::string>{
                  "manager", "player", "effect", "attach"},
          "effect read follows player lookup");
    Runtime empty;
    empty.effect = 0u;
    Check(pal4::ScriptPlayerEffects::Hook(
              -3, 0x2000u, -29, empty) == 0x87654321u &&
              empty.calls == std::vector<std::string>{
                  "manager", "player", "effect", "empty", "attach"},
          "null effect handle uses global empty text");
    Runtime missing;
    missing.player = 0u;
    Check(pal4::ScriptPlayerEffects::Hook(
              -3, 0x2000u, -29, missing) == 0u &&
              missing.calls == std::vector<std::string>{"manager", "player"},
          "missing player avoids effect string access");
}

void Detach() {
    Runtime found;
    Check(pal4::ScriptPlayerEffects::Detach(-3, found) == 0x12345678u &&
              found.calls == std::vector<std::string>{
                  "manager", "player", "detach"},
          "player detach forwards virtual result");
    Runtime missing;
    missing.player = 0u;
    Check(pal4::ScriptPlayerEffects::Detach(-3, missing) == 0u &&
              missing.calls == std::vector<std::string>{"manager", "player"},
          "missing player skips detach");
}

}  // namespace

int main() {
    try {
        Hook();
        Detach();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
