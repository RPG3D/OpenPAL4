#include "pal4/player_manager_access.h"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PlayerManagerAccessRuntimeStub {
    pal4::PlayerManagerAccessState* state{};
    std::uint32_t manager_calls{};
    std::uint32_t register_calls{};
    std::uint32_t allocate_calls{};
    std::uint32_t construct_calls{};
    std::uint32_t allocation{0x5000u};

    std::uint32_t SingletonManager() override {
        Check(state != nullptr && (state->guard & 1u) != 0u,
              "guard is set before singleton-manager lookup");
        ++manager_calls;
        return 0x3000u;
    }
    std::uint32_t Register(std::uint32_t manager, std::string_view name,
        std::uint32_t id, std::uint32_t factory,
        std::uint32_t argument) override {
        Check(manager == 0x3000u && name == "palPlayerManager" &&
                  id == 0x40fu && factory == 0x440ec0u &&
                  argument == 0x440f20u,
              "singleton registration uses observed arguments");
        ++register_calls;
        return 0x4000u;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x2c64u, "factory allocates exact object size");
        ++allocate_calls;
        return allocation;
    }
    std::uint32_t Construct(std::uint32_t address) override {
        Check(address == 0x5000u, "factory constructs allocated address");
        ++construct_calls;
        return 0x6000u;
    }
};

void TestRegistration() {
    pal4::PlayerManagerAccessState state{0x80u, 0x1111u};
    Runtime runtime{};
    runtime.state = &state;
    Check(pal4::PlayerManagerAccess::Get(state, runtime) == 0x4000u &&
              state.guard == 0x81u && state.instance == 0x4000u &&
              runtime.manager_calls == 1u && runtime.register_calls == 1u,
          "first registration sets guard and caches result");
    Check(pal4::PlayerManagerAccess::Get(state, runtime) == 0x4000u &&
              runtime.manager_calls == 1u && runtime.register_calls == 1u,
          "later registration reads cache without another runtime call");
    state = {0u, 0u};
    Check(pal4::PlayerManagerAccess::Get(state, runtime) == 0x4000u &&
              runtime.register_calls == 2u,
          "zeroed state registers again");
}

void TestFactory() {
    Runtime runtime{};
    Check(pal4::PlayerManagerAccess::CreateInstance(runtime) == 0x6000u &&
              runtime.allocate_calls == 1u && runtime.construct_calls == 1u,
          "factory forwards constructed object result");
    runtime.allocation = 0u;
    Check(pal4::PlayerManagerAccess::CreateInstance(runtime) == 0u &&
              runtime.allocate_calls == 2u && runtime.construct_calls == 1u,
          "allocation failure skips constructor");
}

}  // namespace

int main() {
    TestRegistration();
    TestFactory();
}
