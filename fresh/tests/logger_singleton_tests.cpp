#include "pal4/logger_singleton.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class FakeRuntime final : public pal4::ILoggerSingletonRuntime {
public:
    std::uint32_t GetManager() override {
        if (state != nullptr) manager_guard_snapshot = state->guard;
        events.emplace_back("manager");
        return manager;
    }

    std::uint32_t Register(const std::uint32_t actual_manager,
                           const std::string_view type_name,
                           const std::uint32_t singleton_id,
                           const std::uint32_t factory_address,
                           const std::uint32_t type_info_address) override {
        if (state != nullptr) registration_guard_snapshot = state->guard;
        events.emplace_back("register:" + std::to_string(actual_manager) + ":" +
                            std::string(type_name) + ":" +
                            std::to_string(singleton_id) + ":" +
                            std::to_string(factory_address) + ":" +
                            std::to_string(type_info_address));
        return registered_instance;
    }

    std::vector<std::string> events;
    pal4::LoggerSingletonState* state = nullptr;
    std::uint8_t manager_guard_snapshot = 0;
    std::uint8_t registration_guard_snapshot = 0;
    std::uint32_t manager = 0x1234;
    std::uint32_t registered_instance = 0x5678;
};

void FirstCallSetsGuardThenRegistersLogger() {
    pal4::LoggerSingletonState state{0x80, 0};
    FakeRuntime runtime;
    runtime.state = &state;

    Check(pal4::LoggerSingletonAccess::Get(state, runtime) == 0x5678,
          "first access returns the singleton manager result");
    Check(state.guard == 0x81 && state.instance == 0x5678,
          "initialization sets bit zero and caches the returned pointer");
    Check(runtime.manager_guard_snapshot == 0x81 &&
              runtime.registration_guard_snapshot == 0x81,
          "guard is published before manager lookup and registration");
    Check(runtime.events == std::vector<std::string>{
              "manager",
              "register:4660:logger:1:4202896:4202992"},
          "manager lookup precedes registration with observed constants");
}

void RepeatedCallReturnsCachedInstanceWithoutRuntimeCalls() {
    pal4::LoggerSingletonState state{1, 0x8888};
    FakeRuntime runtime;
    Check(pal4::LoggerSingletonAccess::Get(state, runtime) == 0x8888,
          "guarded access returns the cached address");
    Check(runtime.events.empty(), "guarded access does not re-register");
}

void NullRegistrationResultIsStillGuarded() {
    pal4::LoggerSingletonState state{};
    FakeRuntime runtime;
    runtime.registered_instance = 0;
    Check(pal4::LoggerSingletonAccess::Get(state, runtime) == 0,
          "failed registration result is propagated");
    runtime.registered_instance = 0x9999;
    Check(pal4::LoggerSingletonAccess::Get(state, runtime) == 0,
          "guard is set before registration and prevents implicit retry");
    Check(runtime.events.size() == 2,
          "the manager and registration are each called only once");
}

void NullManagerStillFlowsIntoRegistration() {
    pal4::LoggerSingletonState state{};
    FakeRuntime runtime;
    runtime.manager = 0;
    Check(pal4::LoggerSingletonAccess::Get(state, runtime) == 0x5678 &&
              runtime.events == std::vector<std::string>{
                                    "manager", "register:0:logger:1:4202896:4202992"},
          "accessor forwards a null manager without adding an early exit");
}

}  // namespace

int main() {
    FirstCallSetsGuardThenRegistersLogger();
    RepeatedCallReturnsCachedInstanceWithoutRuntimeCalls();
    NullRegistrationResultIsStillGuarded();
    NullManagerStillFlowsIntoRegistration();
    std::cout << "4 logger-singleton test cases passed\n";
}
