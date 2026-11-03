#include "pal4/combat_ending_window_registration.h"

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

struct FactoryRuntime final : pal4::ICombatEndingWindowFactoryRuntime {
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x3000u;
    bool throw_construct{};

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 0x2d0u, "ending dialog object size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t object) override {
        Check(object == 0x3000u, "ending dialog free address");
        calls.emplace_back("free");
    }
    std::uint32_t Construct(std::uint32_t object,
                            std::uint32_t name) override {
        Check(object == 0x3000u && name == 0x8b6aa0u,
              "ending dialog constructor arguments");
        calls.emplace_back("construct");
        if (throw_construct) throw std::runtime_error("construct");
        return 0x3100u;
    }
};

void CheckFactory() {
    FactoryRuntime success;
    Check(pal4::CombatEndingWindowFactory::Create(0x8b6aa0u, success) ==
              0x3100u &&
              success.calls ==
                  std::vector<std::string>{"allocate", "construct"},
          "ending dialog factory forwards constructor result");
    FactoryRuntime unavailable;
    unavailable.allocation = 0u;
    Check(pal4::CombatEndingWindowFactory::Create(0x8b6aa0u, unavailable) ==
              0u &&
              unavailable.calls == std::vector<std::string>{"allocate"},
          "ending dialog factory handles allocation failure");
    FactoryRuntime throwing;
    throwing.throw_construct = true;
    bool caught = false;
    try {
        (void)pal4::CombatEndingWindowFactory::Create(0x8b6aa0u, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.calls ==
              (std::vector<std::string>{"allocate", "construct", "free"}),
          "ending dialog factory releases memory on constructor exception");
}

struct RegistrationRuntime final : pal4::ICombatEndingWindowFactoryRegistrationRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x8f9590u, 0x8b6aa0u}, {0x5008u, 0x6000u},
        {0x6004u, 0x7000u}};
    std::vector<std::string> calls;
    std::uint8_t guard{};
    std::uint32_t singleton{};
    std::uint32_t inserted{};

    std::uint32_t UiFrameManagerInstance() override {
        calls.emplace_back("ui-manager");
        return 0x9000u;
    }
    std::uint8_t& FactoryGuard() override { return guard; }
    std::uint32_t& FactorySingleton() override { return singleton; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x8f9590u) calls.emplace_back("name-slot");
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return address == 0x500cu ? 1u : 0u;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
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
              "ending dialog shared factory singleton");
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
                  parent == 0x6000u && key == 0x8b6aa0u &&
                  creator == 0x542d00u,
              "ending dialog factory insertion arguments");
        calls.emplace_back("insert");
        ++inserted;
        return 0x8000u;
    }
};

void CheckFactoryRegistration() {
    RegistrationRuntime runtime;
    Check(pal4::CombatEndingWindowFactoryRegistration::Register(runtime) ==
              0x8000u && runtime.guard == 1u &&
              runtime.singleton == 0x5000u && runtime.inserted == 1u &&
              runtime.calls ==
                  (std::vector<std::string>{
                      "ui-manager", "name-slot", "singleton-manager",
                      "register-singleton", "insert"}),
          "ending dialog factory registers name and constructor");
    runtime.calls.clear();
    Check(pal4::CombatEndingWindowFactoryRegistration::RegisterThunk(
              runtime) == 0x8000u && runtime.inserted == 2u &&
              runtime.calls ==
                  (std::vector<std::string>{
                      "ui-manager", "name-slot", "insert"}),
          "ending dialog factory thunk reuses singleton");
}

struct EventRuntime final : pal4::IWindowRegistrationRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x8b6a98u, 0x8b6aa0u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x3000u;
    std::uint32_t inserted_handler{};
    std::uint32_t destroyed_handler{};
    bool throw_insert{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x8b6a98u, "ending event name source");
        calls.emplace_back("read");
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 8u, "ending event callback size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t) override {}
    std::uint32_t Construct(std::uint32_t, std::uint32_t) override {
        return 0u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x5000u;
    }
    std::uint32_t RegisterFactory(std::uint32_t, std::uint32_t,
                                  std::uint32_t) override { return 0u; }
    void InsertOrReplace(std::uint32_t target, std::uint32_t key,
                         std::uint32_t handler) override {
        Check(target == 0x5104u && key == 0x8b690cu,
              "ending event tree and key");
        calls.emplace_back("insert");
        inserted_handler = handler;
        if (throw_insert) throw std::runtime_error("insert");
    }
    void DestroyHandler(std::uint32_t handler,
                        std::uint32_t flags) override {
        Check(flags == 1u, "ending event destructor flags");
        calls.emplace_back("destroy");
        destroyed_handler = handler;
    }
};

void CheckEventRegistration() {
    EventRuntime runtime;
    Check(pal4::CombatEndingWindowRegistration::RegisterEvent(runtime) ==
              0x8f958cu &&
              runtime.words.at(0x3000u) == 0x843494u &&
              runtime.words.at(0x3004u) == 0x8b6aa0u &&
              runtime.inserted_handler == 0x3000u &&
              runtime.destroyed_handler == 0x3000u &&
              runtime.calls ==
                  (std::vector<std::string>{
                      "read", "allocate", "write", "write", "manager",
                      "insert", "destroy"}),
          "ending event callback is registered and temporary released");
    EventRuntime unavailable;
    unavailable.allocation = 0u;
    Check(pal4::CombatEndingWindowRegistration::RegisterEventThunk(
              unavailable) == 0x8f958cu &&
              unavailable.inserted_handler == 0u &&
              unavailable.destroyed_handler == 0u,
          "ending event thunk forwards null callback");
    EventRuntime throwing;
    throwing.throw_insert = true;
    bool caught = false;
    try {
        (void)pal4::CombatEndingWindowRegistration::RegisterEvent(throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.destroyed_handler == 0x3000u,
          "ending event callback releases temporary on insertion exception");
}

}  // namespace

int main() {
    try {
        CheckFactory();
        CheckFactoryRegistration();
        CheckEventRegistration();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
