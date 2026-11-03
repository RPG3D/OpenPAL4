#include "pal4/global_data_subsystem_registration.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGlobalDataSubsystemRuntime {
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x3000;
    std::uint32_t registered = 0x2000;
    bool throw_in_constructor = false;

    std::uint32_t RegisterSingleton(
        std::uint32_t name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t cleanup) override {
        Check(name == 0x8bdee0 && id == 1047 &&
                  factory == 0x5ccdf0 && cleanup == 0x5cce50,
              "palGlobalData registration operands");
        calls.emplace_back("register");
        return registered;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        Check(bytes == 0xc4, "palGlobalData object size");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t Construct(std::uint32_t object) override {
        Check(object == allocation, "constructor receives allocation");
        calls.emplace_back("construct");
        if (throw_in_constructor) throw std::runtime_error("construct failed");
        return 0x3100;
    }
    void Cleanup(std::uint32_t object) override {
        Check(object == 0x3000, "cleanup receives registered object");
        calls.emplace_back("cleanup");
    }
    void FreeMemory(std::uint32_t object) override {
        Check(object == 0x3000, "free receives registered object");
        calls.emplace_back("free");
    }
};

void TestGuardAndRegistration() {
    Runtime runtime;
    std::uint8_t guard = 0x80;
    std::uint32_t cached = 0x1111;
    Check(pal4::GlobalDataSubsystemRegistration::Initialize(
              guard, cached, runtime) == 0x2000 &&
              guard == 0x81 && cached == 0x2000 &&
              runtime.calls == std::vector<std::string>{"register"},
          "first initialization sets guard and caches registration result");
    runtime.registered = 0x2222;
    Check(pal4::GlobalDataSubsystemRegistration::Initialize(
              guard, cached, runtime) == 0x2000 &&
              runtime.calls == std::vector<std::string>{"register"},
          "guarded initialization returns cached pointer");
    guard = 1;
    cached = 0;
    runtime.calls.clear();
    Check(pal4::GlobalDataSubsystemRegistration::Initialize(
              guard, cached, runtime) == 0 && runtime.calls.empty(),
          "set guard returns even a null cached pointer");
}

void TestFactoryAndCleanup() {
    Runtime runtime;
    Check(pal4::GlobalDataSubsystemRegistration::Create(runtime) == 0x3100 &&
              runtime.calls ==
                  std::vector<std::string>{"allocate", "construct"},
          "factory returns constructor result");
    runtime.calls.clear();
    pal4::GlobalDataSubsystemRegistration::Destroy(0x3000, runtime);
    Check(runtime.calls == std::vector<std::string>{"cleanup", "free"},
          "cleanup runs before free");
    runtime.calls.clear();
    pal4::GlobalDataSubsystemRegistration::Destroy(0, runtime);
    Check(runtime.calls.empty(), "null cleanup has no side effects");

    runtime = Runtime{};
    runtime.allocation = 0;
    Check(pal4::GlobalDataSubsystemRegistration::Create(runtime) == 0 &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "allocation failure avoids constructor");
    runtime = Runtime{};
    runtime.throw_in_constructor = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::GlobalDataSubsystemRegistration::Create(
            runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.calls == std::vector<std::string>{
                                       "allocate", "construct", "free"},
          "constructor failure frees allocation through SEH cleanup path");
}

}  // namespace

int main() {
    try {
        TestGuardAndRegistration();
        TestFactoryAndCleanup();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
