#include "pal4/ef_effect_system_lifecycle.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

struct Runtime final : pal4::EfEffectSystemLifecycleRuntimeStub {
    std::vector<std::string> calls;
    std::uint32_t allocated = 0x2000u;
    std::uint32_t constructed = 0x2000u;
    std::uint32_t registered = 0x3000u;
    std::uint32_t freed = 0x4000u;
    int cleanup_result = -7;

    std::uint32_t SingletonManager() override {
        calls.emplace_back("manager");
        return 0x1000u;
    }
    std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destroy) override {
        Check(manager == 0x1000u &&
              name == pal4::EfEffectSystemLifecycle::kSingletonName &&
              id == 1037u && factory == 0x635d50u &&
              destroy == 0x635db0u,
              "singleton registration uses IDA name, ID and callbacks");
        calls.emplace_back("register");
        return registered;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0xa8u, "instance factory allocates 168 bytes");
        calls.emplace_back("allocate");
        return allocated;
    }
    std::uint32_t Construct(std::uint32_t object) override {
        Check(object == allocated, "constructor receives allocated object");
        calls.emplace_back("construct");
        return constructed;
    }
    void Destruct(std::uint32_t object) override {
        Check(object == 0x2000u, "destructor receives instance");
        calls.emplace_back("destruct");
    }
    std::uint32_t Free(std::uint32_t object) override {
        Check(object == 0x2000u, "allocator frees the same instance");
        calls.emplace_back("free");
        return freed;
    }
    void ProcessScript() override { calls.emplace_back("process script"); }
    int CleanupScripts() override {
        calls.emplace_back("cleanup scripts");
        return cleanup_result;
    }
};

void TestRegistration() {
    Runtime runtime;
    std::uint8_t guard = 0x80u;
    std::uint32_t registered = 0u;
    Check(pal4::EfEffectSystemLifecycle::Register(
              guard, registered, runtime) == 0x3000u &&
          guard == 0x81u && registered == 0x3000u &&
          runtime.calls == (std::vector<std::string>{"manager", "register"}),
          "first registration sets guard and caches result");
    runtime.calls.clear();
    Check(pal4::EfEffectSystemLifecycle::Register(
              guard, registered, runtime) == 0x3000u &&
          runtime.calls.empty(),
          "guarded registration returns cached value without native calls");

    guard = 0u;
    registered = 0x7777u;
    runtime.registered = 0u;
    Check(pal4::EfEffectSystemLifecycle::Register(
              guard, registered, runtime) == 0u &&
          guard == 1u && registered == 0u,
          "failed registration still sets guard and stores native result");
    runtime.calls.clear();
    Check(pal4::EfEffectSystemLifecycle::Register(
              guard, registered, runtime) == 0u &&
          runtime.calls.empty(),
          "registration failure is not retried after guard is set");
}

void TestInstanceFactoryAndDestroy() {
    Runtime runtime;
    runtime.allocated = 0u;
    Check(pal4::EfEffectSystemLifecycle::Create(runtime) == 0u &&
          runtime.calls == (std::vector<std::string>{"allocate"}),
          "allocation failure skips construction");
    runtime.calls.clear();
    runtime.allocated = 0x2000u;
    Check(pal4::EfEffectSystemLifecycle::Create(runtime) == 0x2000u &&
          runtime.calls == (std::vector<std::string>{
              "allocate", "construct"}),
          "successful factory constructs allocated instance");
    runtime.calls.clear();
    Check(pal4::EfEffectSystemLifecycle::Destroy(0u, runtime) == 0u &&
          runtime.calls.empty(),
          "null destruction performs no native calls");
    Check(pal4::EfEffectSystemLifecycle::Destroy(0x2000u, runtime) ==
              0x4000u &&
          runtime.calls == (std::vector<std::string>{
              "destruct", "free"}),
          "instance destruction precedes allocator release");
}

void TestScriptWrappers() {
    Runtime runtime;
    Check(pal4::EfEffectSystemLifecycle::ProcessScript(
              0x5000u, runtime) == 0x5000u &&
          pal4::EfEffectSystemLifecycle::CleanupScripts(runtime) == -7 &&
          runtime.calls == (std::vector<std::string>{
              "process script", "cleanup scripts"}),
          "script wrappers preserve owner and native cleanup result");
}

}  // namespace

int main() {
    TestRegistration();
    TestInstanceFactoryAndDestroy();
    TestScriptWrappers();
}
