#include "pal4/pal_actor_control_access.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PalActorControlAccessRuntimeStub {
    std::uint32_t SingletonManager() override {
        events.emplace_back("manager");
        return 0x8000u;
    }
    std::uint32_t Register(std::uint32_t manager,
                           std::string_view name,
                           std::uint32_t id,
                           std::uint32_t factory,
                           std::uint32_t destroy) override {
        events.emplace_back("register");
        Check(manager == 0x8000u && name == "palActorController" &&
                  id == 1014u && factory == 0x4235e0u &&
                  destroy == 0x423640u,
              "singleton registration preserves ID, name and callbacks");
        return registration_result;
    }
    void* Allocate(std::uint32_t bytes) override {
        events.emplace_back("allocate");
        Check(bytes == 0x464u, "factory allocates 1124 bytes");
        return allocation;
    }
    void* Construct(void* storage) override {
        events.emplace_back("construct");
        Check(storage == allocation,
              "factory forwards allocated storage to constructor");
        return constructed;
    }
    void Destruct(void* object) override {
        events.emplace_back("destruct");
        Check(object == allocation,
              "destroy wrapper forwards the original object");
    }
    std::uint32_t Free(void* object) override {
        events.emplace_back("free");
        Check(object == allocation,
              "destroy wrapper frees the original object");
        return 0x7777u;
    }

    std::uint32_t registration_result{0x1234u};
    int object{};
    int result{};
    void* allocation{&object};
    void* constructed{&result};
    std::vector<std::string> events;
};

void TestRegistration() {
    Runtime runtime;
    pal4::PalActorControlAccessState state{};
    state.guard = 0x80u;
    Check(pal4::PalActorControlAccess::Get(state, runtime) ==
              0x1234u && state.instance == 0x1234u &&
              state.guard == 0x81u &&
              runtime.events ==
                  std::vector<std::string>{"manager", "register"},
          "cold singleton registration caches returned pointer");
    runtime.events.clear();
    Check(pal4::PalActorControlAccess::Get(state, runtime) ==
              0x1234u && runtime.events.empty(),
          "warm singleton access returns cache without manager call");

    pal4::PalActorControlAccessState failed{};
    runtime.registration_result = 0u;
    Check(pal4::PalActorControlAccess::Get(failed, runtime) == 0u &&
              failed.guard == 1u && failed.instance == 0u,
          "registration failure still sets one-shot guard");
    runtime.events.clear();
    Check(pal4::PalActorControlAccess::Get(failed, runtime) == 0u &&
              runtime.events.empty(),
          "guard prevents registration retry after failure");
}

void TestFactory() {
    Runtime runtime;
    Check(pal4::PalActorControlAccess::Create(runtime) ==
              runtime.constructed &&
              runtime.events ==
                  std::vector<std::string>{"allocate", "construct"},
          "factory constructs an allocated control object");
    runtime.allocation = nullptr;
    runtime.events.clear();
    Check(pal4::PalActorControlAccess::Create(runtime) == nullptr &&
              runtime.events == std::vector<std::string>{"allocate"},
          "allocation failure skips constructor");
}

void TestDestroy() {
    Runtime runtime;
    Check(pal4::PalActorControlAccess::Destroy(
              runtime.allocation, 0xabceu, runtime) == 0x7777u &&
              runtime.events ==
                  std::vector<std::string>{"destruct", "free"},
          "destroy wrapper destructs before freeing");
    runtime.events.clear();
    Check(pal4::PalActorControlAccess::Destroy(
              nullptr, 0xabceu, runtime) == 0xabceu &&
              runtime.events.empty(),
          "null destroy preserves the original unspecified register value");
}

}  // namespace

int main() {
    TestRegistration();
    TestFactory();
    TestDestroy();
}
