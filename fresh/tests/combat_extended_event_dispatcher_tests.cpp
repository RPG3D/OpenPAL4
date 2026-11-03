#include "pal4/combat_extended_event_dispatcher.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatExtendedEventDispatchRuntime {
    std::uint32_t kind = 0u;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x1000u + 12u, "event kind address");
        calls.emplace_back("kind");
        return kind;
    }
    void SelectMagic(std::uint32_t event) override {
        Check(event == 0x1000u, "music event");
        calls.emplace_back("magic");
    }
    void SelectStunt(std::uint32_t event) override {
        Check(event == 0x1000u, "sound event");
        calls.emplace_back("stunt");
    }
    void UseProperty(std::uint32_t event) override {
        Check(event == 0x1000u, "property event");
        calls.emplace_back("property");
    }
    void SelectAttack(std::uint32_t event) override {
        Check(event == 0x1000u, "flee event");
        calls.emplace_back("attack");
    }
    std::uint32_t ScriptEventRouter() override {
        calls.emplace_back("router");
        return 0x3000u;
    }
    void ProcessByteEvent(std::uint32_t router,
                          std::uint32_t descriptor,
                          std::uint8_t value,
                          std::uint32_t mode) override {
        Check(router == 0x3000u && descriptor == 0x8b62d8u &&
                  value == 0xa5u && mode == 1u,
              "close UI event arguments");
        calls.emplace_back("close-ui");
    }
    void ProcessRecordEvent(std::uint32_t router,
                            std::uint32_t descriptor,
                            std::uint32_t event,
                            std::uint32_t mode) override {
        Check(router == 0x3000u && descriptor == 0x8b62ccu &&
                  event == 0x1000u && mode == 1u,
              "select target event arguments");
        calls.emplace_back("select-target");
    }
};

void AllCases() {
    for (const auto [kind, command] : {
             std::pair{0u, "magic"}, std::pair{1u, "stunt"},
             std::pair{2u, "property"}, std::pair{4u, "attack"}}) {
        Runtime runtime;
        runtime.kind = kind;
        pal4::CombatExtendedEventDispatcher::Dispatch(
            0xa5002000u, 0x1000u, runtime);
        Check(runtime.calls ==
                  std::vector<std::string>{"kind", command},
              "direct command dispatch uses one event argument");
    }

    for (const auto kind : {3u, 5u}) {
        Runtime runtime;
        runtime.kind = kind;
        pal4::CombatExtendedEventDispatcher::Dispatch(
            0xa5002000u, 0x1000u, runtime);
        Check(runtime.calls == std::vector<std::string>{
                  "kind", "router", "close-ui", "router",
                  "select-target"},
              "cases three and five dispatch both tail events in order");
    }

    Runtime unknown;
    unknown.kind = 6u;
    pal4::CombatExtendedEventDispatcher::Dispatch(
        0xa5002000u, 0x1000u, unknown);
    Check(unknown.calls == std::vector<std::string>{"kind"},
          "unknown command has no side effect");
}

}  // namespace

int main() {
    try {
        AllCases();
        std::cout << "combat extended event dispatcher tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
