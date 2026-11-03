#include "pal4/pet_manager.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PetManagerRuntimeStub {
    std::uint32_t allocated{0x5000u};
    std::uint32_t registration{0x7000u};
    std::uint32_t reset_result{0x1234u};
    std::uint32_t cleanup_result{0x5678u};
    std::uint32_t update_result{0x9abcu};
    bool throw_on_base_construct{};
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    pal4::PetManagerAccessState* access{};

    std::uint32_t SingletonManager() override {
        events.emplace_back("singleton");
        if (access != nullptr)
            Check(pal4::PetManager::GetInstance(*access, *this) == 0x8888u,
                  "guard is set before singleton lookup");
        return 0x6000u;
    }
    std::uint32_t Register(std::uint32_t manager, std::string_view name,
                           std::uint32_t id, std::uint32_t factory,
                           std::uint32_t type_info) override {
        Check(manager == 0x6000u && name == "palPet" && id == 0x412u &&
                  factory == 0x43dea0u && type_info == 0x7de5d0u,
              "singleton registration operands");
        events.emplace_back("register");
        return registration;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x674u, "pet object allocation size");
        events.emplace_back("allocate");
        return allocated;
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x5000u, "factory frees allocated object");
        events.emplace_back("free");
    }
    void BaseConstruct(std::uint32_t address) override {
        Check(address == 0x5000u, "base constructor receives object");
        events.emplace_back("base-construct");
        if (throw_on_base_construct) throw std::runtime_error("base failure");
    }
    void BaseDestroy(std::uint32_t address) override {
        Check(address == 0x5000u, "base destructor receives object");
        events.emplace_back("base-destroy");
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        events.push_back("write:" + std::to_string(address));
    }
    std::uint32_t ResetGameObject(std::uint32_t address) override {
        Check(address == 0x5000u, "reset receives object");
        events.emplace_back("reset");
        return reset_result;
    }
    void ProcessScript(std::uint32_t script) override {
        Check(script == 0xaaaau, "script pointer is read from +0x654");
        events.emplace_back("script");
    }
    void InitializeGameObject(std::uint32_t object, std::uint32_t first,
                              std::uint32_t second,
                              std::uint32_t third) override {
        Check(object == 0x5000u && first == 1u && second == 2u && third == 3u,
              "game object initializer forwards all three arguments");
        events.emplace_back("initialize");
    }
    std::uint32_t CleanupScripts() override {
        events.emplace_back("cleanup-scripts");
        return cleanup_result;
    }
    std::uint32_t UpdateGameObject(std::uint32_t object,
                                   std::uint32_t argument) override {
        Check(object == 0x5000u && argument == 0x99u,
              "conditional update forwards object and stack argument");
        events.emplace_back("update");
        return update_result;
    }
};

void CheckSingleton() {
    Runtime runtime;
    pal4::PetManagerAccessState state{0x80u, 0x8888u};
    runtime.access = &state;
    Check(pal4::PetManager::GetInstance(state, runtime) == 0x7000u &&
              state.guard == 0x81u && state.instance == 0x7000u,
          "first lookup registers and caches the instance");
    Check(runtime.events == std::vector<std::string>{"singleton", "register"},
          "singleton manager lookup precedes registration");
    Check(pal4::PetManager::GetInstance(state, runtime) == 0x7000u &&
              runtime.events.size() == 2u,
          "later lookup uses cached instance");
}

void CheckFactoryAndLifecycle() {
    Runtime runtime;
    runtime.words[0x5000u + 0x664u] = 0xbeefu;
    Check(pal4::PetManager::Create(runtime) == 0x5000u,
          "factory returns constructed object");
    Check(runtime.events[0] == "allocate" &&
              runtime.events[1] == "base-construct",
          "factory allocation and base construction order");
    Check(runtime.words[0x5000u] == 0x842e98u &&
              runtime.words[0x5170u] == 0x842e8cu &&
              runtime.words[0x51ecu] == 0x842e80u,
          "constructor writes three observed vtable fields");
    for (const auto offset : {0x654u, 0x658u, 0x65cu, 0x660u})
        Check(runtime.words[0x5000u + offset] == 0u,
              "constructor clears each observed state field");
    Check(runtime.words[0x5668u] == 0x41a00000u &&
              runtime.words[0x5670u] == 0x43790000u &&
              runtime.words[0x5664u] == 0xbeefu,
          "constructor writes float bit patterns and preserves neighboring field");

    runtime.words[0x5658u] = 1u;
    runtime.words[0x565cu] = 2u;
    runtime.words[0x5660u] = 3u;
    runtime.events.clear();
    Check(pal4::PetManager::Reset(0x5000u, runtime) == 0x1234u,
          "reset forwards game object result");
    Check(runtime.words[0x5658u] == 0u && runtime.words[0x565cu] == 0u &&
              runtime.words[0x5660u] == 0u &&
              runtime.words[0x5668u] == 0x41a00000u &&
              runtime.events.back() == "reset",
          "reset clears three fields before base reset");
    runtime.events.clear();
    pal4::PetManager::Destroy(0x5000u, runtime);
    Check(runtime.events.size() == 4u &&
              runtime.events.back() == "base-destroy" &&
              runtime.words[0x5000u] == 0x842e98u,
          "destructor rewrites all vtables before base destructor");
}

void CheckAllocationAndException() {
    Runtime runtime;
    runtime.allocated = 0u;
    Check(pal4::PetManager::Create(runtime) == 0u &&
              runtime.events == std::vector<std::string>{"allocate"},
          "failed allocation skips construction");
    runtime.allocated = 0x5000u;
    runtime.throw_on_base_construct = true;
    runtime.events.clear();
    try {
        static_cast<void>(pal4::PetManager::Create(runtime));
        Check(false, "factory must propagate construction exception");
    } catch (const std::runtime_error&) {
        Check(runtime.events == std::vector<std::string>{
                  "allocate", "base-construct", "free"},
              "factory frees storage on constructor exception");
    }
}

void CheckScriptAndUpdate() {
    Runtime runtime;
    Check(pal4::PetManager::ExecuteScriptAndInitialize(
              0x5000u, 1u, 2u, 3u, runtime) == 0u &&
              runtime.events.size() == 1u,
          "zero script skips processing and initialization");
    runtime.words[0x5654u] = 0xaaaau;
    runtime.events.clear();
    Check(pal4::PetManager::ExecuteScriptAndInitialize(
              0x5000u, 1u, 2u, 3u, runtime) == 0x5678u &&
              runtime.events == std::vector<std::string>{
                  "read:22100", "script", "initialize", "cleanup-scripts"},
          "active script processing, initialization and cleanup order");

    runtime.words[0x5654u] = 0u;
    runtime.events.clear();
    Check(pal4::PetManager::ConditionalUpdate(
              0x5000u, 0x99u, runtime) == 0u &&
              runtime.events.size() == 1u,
          "inactive pet skips secondary test and update");
    runtime.words[0x5654u] = 1u;
    runtime.events.clear();
    Check(pal4::PetManager::ConditionalUpdate(
              0x5000u, 0x99u, runtime) == 0u &&
              runtime.events.size() == 2u,
          "missing +0x1f0 field skips update");
    runtime.words[0x51f0u] = 1u;
    runtime.events.clear();
    Check(pal4::PetManager::ConditionalUpdate(
              0x5000u, 0x99u, runtime) == 0x9abcu &&
              runtime.events.back() == "update",
          "both conditions forward to game object update");
}

}  // namespace

int main() {
    CheckSingleton();
    CheckFactoryAndLifecycle();
    CheckAllocationAndException();
    CheckScriptAndUpdate();
    std::cout << "4 pet manager test groups passed\n";
}
