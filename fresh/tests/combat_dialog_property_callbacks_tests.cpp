#include "pal4/combat_dialog_property_callbacks.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatDialogPropertyRuntime {
    std::array<std::uint32_t, 6> properties{};
    std::vector<std::string> calls;
    std::int32_t current = 0;

    std::uint32_t ActorFromFacet(std::uint32_t event) override {
        Check(event == 0x1000u, "facet event address");
        calls.emplace_back("actor");
        return 0x2000u;
    }
    std::int32_t ActorSlot(std::uint32_t actor) override {
        Check(actor == 0x2000u, "actor slot address");
        calls.emplace_back("slot");
        return current;
    }
    void GetProperties(std::uint32_t actor,
                       std::array<std::uint32_t, 6>& output) override {
        Check(actor == 0x2000u, "actor property address");
        calls.emplace_back("properties");
        output = properties;
    }
    std::uint8_t InitializeEffects(std::uint32_t actor,
                                   std::uint32_t slot) override {
        Check(actor == 0x2000u, "effect actor address");
        calls.emplace_back("initialize:" + std::to_string(slot));
        return 0xabu;
    }
};

void Primary() {
    Runtime runtime;
    runtime.current = 2;
    Check(pal4::CombatDialogPropertyCallbacks::HandlePrimary(
              0x1000u, runtime) == 0xabu &&
              runtime.calls == std::vector<std::string>{
                  "actor", "slot", "properties", "initialize:2"},
          "available current slot is used");

    runtime.calls.clear();
    runtime.properties = {5u, 0u, 8u, 0u, 0u, 0u};
    Check(pal4::CombatDialogPropertyCallbacks::HandlePrimary(
              0x1000u, runtime) == 0xabu &&
              runtime.calls.back() == "initialize:1",
          "occupied current slot selects first empty primary slot");

    runtime.calls.clear();
    runtime.current = 5;
    runtime.properties = {5u, 6u, 7u, 0u, 0u, 0u};
    Check(pal4::CombatDialogPropertyCallbacks::HandlePrimary(
              0x1000u, runtime) == 3u &&
              runtime.calls == std::vector<std::string>{
                  "actor", "slot", "properties"},
          "full primary range returns three without initialization");
}

void Extended() {
    Runtime runtime;
    runtime.current = 8;
    runtime.properties = {1u, 2u, 3u, 4u, 0u, 6u};
    Check(pal4::CombatDialogPropertyCallbacks::HandleExtended(
              0x1000u, runtime) == 0xabu &&
              runtime.calls.back() == "initialize:4",
          "out of range slot searches all six properties");

    runtime.calls.clear();
    runtime.current = 5;
    runtime.properties = {1u, 2u, 3u, 4u, 5u, 6u};
    Check(pal4::CombatDialogPropertyCallbacks::HandleExtended(
              0x1000u, runtime) == 6u &&
              runtime.calls == std::vector<std::string>{
                  "actor", "slot", "properties"},
          "full extended range returns six");

    runtime.calls.clear();
    runtime.current = -1;
    bool rejected = false;
    try {
        (void)pal4::CombatDialogPropertyCallbacks::HandleExtended(
            0x1000u, runtime);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    Check(rejected && runtime.calls == std::vector<std::string>{
              "actor", "slot", "properties"},
          "negative slot is rejected before indexing properties");
}

}  // namespace

int main() {
    try {
        Primary();
        Extended();
        std::cout << "combat dialog property callbacks tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
