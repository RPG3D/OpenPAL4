#include "pal4/combat_system_lifecycle.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct SingletonRuntime final : pal4::ICombatSystemSingletonRuntime {
    std::vector<std::string> calls;
    std::uint32_t result = 0x3000u;
    std::uint8_t* guard = nullptr;

    std::uint32_t GetSingletonManager() override {
        Check(guard != nullptr && (*guard & 1u) != 0u,
              "registration guard precedes manager lookup");
        calls.emplace_back("manager");
        return 0x2000u;
    }
    std::uint32_t Register(std::uint32_t manager, const char* name,
                           std::uint32_t id, std::uint32_t factory,
                           std::uint32_t destructor) override {
        Check(manager == 0x2000u && std::string(name) == "cbCombatSystem" &&
                  id == 1028u && factory == 0x574f50u &&
                  destructor == 0x574fb0u,
              "singleton registration identity matches IDA");
        calls.emplace_back("register");
        return result;
    }
};

struct LifecycleRuntime final : pal4::ICombatSystemLifecycleRuntime {
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x4000u;
    std::uint32_t constructed = 0x4100u;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x208u, "factory allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t Construct(std::uint32_t object) override {
        Check(object == 0x4000u, "constructor receives allocated object");
        calls.emplace_back("construct");
        return constructed;
    }
    void Destruct(std::uint32_t object) override {
        Check(object == 0x4000u, "destructor receives object");
        calls.emplace_back("destruct");
    }
    std::uint32_t Free(std::uint32_t object) override {
        Check(object == 0x4000u, "free receives object");
        calls.emplace_back("free");
        return 0x5000u;
    }
};

void Singleton() {
    pal4::CombatSystemSingletonState state{0xaau, 0u};
    pal4::CombatSystemSingleton singleton(state);
    SingletonRuntime runtime;
    runtime.guard = &state.initialization_bits;
    Check(singleton.GetInstance(runtime) == 0x3000u &&
              state.initialization_bits == 0xabu &&
              state.instance == 0x3000u &&
              runtime.calls == std::vector<std::string>{"manager", "register"},
          "first lookup registers and caches instance");
    runtime.calls.clear();
    Check(singleton.GetInstance(runtime) == 0x3000u && runtime.calls.empty(),
          "guarded lookup returns cached instance");

    pal4::CombatSystemSingletonState failed;
    pal4::CombatSystemSingleton retry(failed);
    runtime.guard = &failed.initialization_bits;
    runtime.result = 0u;
    Check(retry.GetInstance(runtime) == 0u &&
              failed.initialization_bits == 1u,
          "failed registration still sets guard");
    runtime.calls.clear();
    runtime.result = 0x3000u;
    Check(retry.GetInstance(runtime) == 0u && runtime.calls.empty(),
          "guard prevents registration retry");
}

void FactoryAndCleanup() {
    LifecycleRuntime runtime;
    Check(pal4::CombatSystemLifecycle::Create(runtime) == 0x4100u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "construct"},
          "factory returns constructor result");
    runtime.calls.clear();
    runtime.allocation = 0u;
    Check(pal4::CombatSystemLifecycle::Create(runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "allocation failure skips constructor");
    runtime.calls.clear();
    Check(pal4::CombatSystemLifecycle::Destroy(0u, runtime) == 0u &&
              runtime.calls.empty(), "null cleanup is empty");
    Check(pal4::CombatSystemLifecycle::Destroy(0x4000u, runtime) ==
                  0x5000u &&
              runtime.calls == std::vector<std::string>{"destruct", "free"},
          "cleanup destroys before freeing and returns free result");
}

}  // namespace

int main() {
    try {
        Singleton();
        FactoryAndCleanup();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
