#include "pal4/combat_action_console_window_registration.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final
    : pal4::ICombatActionConsoleWindowFactoryRegistrationRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint8_t guard = 0u;
    std::uint32_t singleton = 0u;
    std::uint32_t insertion_result = 0x8000u;
    std::uint32_t insertion_calls = 0u;

    Runtime() {
        words[0x8f94d4u] = 0x8b695cu;
        words[0x5008u] = 0x6000u;
        words[0x6004u] = 0x7000u;
        bytes[0x500cu] = 1u;
    }
    std::uint32_t UiFrameManagerInstance() override {
        calls.emplace_back("ui-manager");
        return 0x9000u;
    }
    std::uint8_t& FactoryGuard() override { return guard; }
    std::uint32_t& FactorySingleton() override { return singleton; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x8f94d4u) calls.emplace_back("name-slot");
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override { words[address] = value; }
    std::string_view FrameName(std::uint32_t) override { return {}; }
    std::uint32_t SingletonManager() override {
        calls.emplace_back("singleton-manager");
        return 0x4000u;
    }
    std::uint32_t RegisterFactorySingleton(
        std::uint32_t manager, const char* name, std::uint32_t id,
        std::uint32_t constructor, std::uint32_t destructor) override {
        Check(manager == 0x4000u &&
                  std::string_view(name) ==
                      "fsObjectFactory < template-arg4 >" &&
                  id == 1019u && constructor == 0x4c1c10u &&
                  destructor == 0x4c2210u,
              "shared factory singleton registration");
        calls.emplace_back("register-singleton");
        return 0x5000u;
    }
    std::uint32_t FindFactoryNode(std::uint32_t,
                                  const char*) override { return 0u; }
    std::uint32_t FactoryEndNode(std::uint32_t) override { return 0u; }
    std::uint32_t CreateFrame(std::uint32_t,
                              const char*) override { return 0u; }
    std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    void Free(std::uint32_t) override {}
    std::uint32_t SharedNil() override { return 0x7000u; }
    std::uint32_t Predecessor(std::uint32_t) override {
        throw std::runtime_error("unexpected predecessor");
    }
    std::uint32_t InsertFactoryNode(
        std::uint32_t tree, std::uint32_t nil,
        std::uint32_t parent, std::uint32_t key,
        std::uint32_t creator) override {
        Check(tree == 0x5004u && nil == 0x7000u &&
                  parent == 0x6000u && key == 0x8b695cu &&
                  creator == 0x540f70u,
              "factory insertion args");
        calls.emplace_back("insert");
        ++insertion_calls;
        return insertion_result;
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        Check(pal4::CombatActionConsoleWindowFactoryRegistration::Register(
                  runtime) == 0x8000u &&
                  runtime.guard == 1u && runtime.singleton == 0x5000u &&
                  runtime.insertion_calls == 1u &&
                  runtime.calls == std::vector<std::string>{
                      "ui-manager", "name-slot", "singleton-manager",
                      "register-singleton", "insert"},
              "first registration resolves name, singleton and factory");

        runtime.calls.clear();
        runtime.insertion_result = 0u;
        Check(pal4::CombatActionConsoleWindowFactoryRegistration::RegisterThunk(
                  runtime) == 0u &&
                  runtime.insertion_calls == 2u &&
                  runtime.calls == std::vector<std::string>{
                      "ui-manager", "name-slot", "insert"},
              "thunk forwards insertion result and reuses singleton");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
