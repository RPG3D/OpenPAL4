#include "pal4/trade_system_lifecycle.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    std::string_view operation;
    std::uint32_t first = 0u;
    std::uint32_t second = 0u;
    bool operator==(const Call&) const = default;
};

struct SingletonRuntime final : pal4::ITradeSystemSingletonRuntime {
    pal4::TradeSystemSingletonState* state = nullptr;
    std::vector<Call> calls;
    std::uint32_t registration_result = 0x9000u;

    std::uint32_t GetSingletonManager() override {
        Check(state != nullptr && (state->initialization_bits & 1u) != 0u,
              "TradeSystem guard is set before manager lookup");
        calls.push_back({"manager"});
        return 0x8000u;
    }
    std::uint32_t Register(const std::uint32_t manager, const char* name,
                           const std::uint32_t id,
                           const std::uint32_t factory,
                           const std::uint32_t destructor) override {
        Check(manager == 0x8000u && name == std::string_view("palTradeSystem") &&
                  id == 1053u && factory == 0x50f970u &&
                  destructor == 0x428c00u,
              "TradeSystem registration identity matches IDA");
        calls.push_back({"register", factory, destructor});
        return registration_result;
    }
};

struct LifecycleRuntime final : pal4::ITradeSystemLifecycleRuntime {
    std::vector<Call> calls;
    std::uint32_t allocation = 0x4000u;

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        calls.push_back({"allocate", bytes});
        return allocation;
    }
    void Clear(const std::uint32_t address,
               const std::uint32_t bytes) override {
        calls.push_back({"clear", address, bytes});
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.push_back({"write", address, value});
    }
};

void SingletonRegistration() {
    pal4::TradeSystemSingletonState state{0xa0u, 0u};
    pal4::TradeSystemSingleton singleton(state);
    SingletonRuntime runtime;
    runtime.state = &state;
    Check(singleton.GetInstance(runtime) == 0x9000u &&
              state.initialization_bits == 0xa1u &&
              state.instance == 0x9000u,
          "TradeSystem singleton caches registration result");
    Check(singleton.GetInstance(runtime) == 0x9000u &&
              runtime.calls == std::vector<Call>{
                  {"manager"}, {"register", 0x50f970u, 0x428c00u}},
          "TradeSystem singleton registers only once");

    pal4::TradeSystemSingletonState failed_state{};
    pal4::TradeSystemSingleton failed(failed_state);
    SingletonRuntime failed_runtime;
    failed_runtime.state = &failed_state;
    failed_runtime.registration_result = 0u;
    Check(failed.GetInstance(failed_runtime) == 0u &&
              failed_state.initialization_bits == 1u &&
              failed.GetInstance(failed_runtime) == 0u &&
              failed_runtime.calls.size() == 2u,
          "TradeSystem failed registration is still guarded");
}

void FactoryAndConstructor() {
    LifecycleRuntime runtime;
    Check(pal4::TradeSystemLifecycle::CreateInstance(runtime) == 0x4000u &&
              runtime.calls == std::vector<Call>{
                  {"allocate", 0x54u}, {"clear", 0x4004u, 0x50u},
                  {"write", 0x4004u, 1u}},
          "TradeSystem factory clears payload then sets its leading word");

    LifecycleRuntime direct;
    Check(pal4::TradeSystemLifecycle::Construct(0x5000u, direct) == 0x5000u &&
              direct.calls == std::vector<Call>{
                  {"clear", 0x5004u, 0x50u}, {"write", 0x5004u, 1u}},
          "TradeSystem constructor preserves the first object word");

    LifecycleRuntime failed;
    failed.allocation = 0u;
    Check(pal4::TradeSystemLifecycle::CreateInstance(failed) == 0u &&
              failed.calls == std::vector<Call>{{"allocate", 0x54u}},
          "TradeSystem allocation failure skips construction");
}

}  // namespace

int main() {
    try {
        SingletonRegistration();
        FactoryAndConstructor();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
