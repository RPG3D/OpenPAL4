#include "pal4/skill_system_lifecycle.h"
#include "pal4/skill_system_event_registry.h"
#include "pal4/crt_noop_atexit_registration.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISkillSystemLifecycleRuntime,
                       pal4::ICrtNoopAtExitRuntime {
    std::map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x7000u;
    std::uint32_t registration = 0x9000u;
    std::uint8_t header_byte = 0x5au;
    std::uint32_t allocations = 0u;
    std::uint32_t next_vector_allocation = 0xb000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return memory[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("word " + std::to_string(address) + " " +
                        std::to_string(value));
        memory[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("byte " + std::to_string(address) + " " +
                        std::to_string(value));
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back("allocate " + std::to_string(size));
        if (allocations++ == 0u) return allocation;
        const auto result = next_vector_allocation;
        next_vector_allocation += size + 0x100u;
        return result;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
        return 99u;
    }
    std::uint8_t UninitializedHeaderByte() override {
        calls.push_back("header-byte");
        return header_byte;
    }
    void ConstructEventLogger(std::uint32_t object) override {
        calls.push_back("construct-logger " + std::to_string(object));
    }
    void CleanupEventLogger(std::uint32_t object) override {
        calls.push_back("cleanup-logger " + std::to_string(object));
    }
    void SetLogFile(std::uint32_t name, std::uint32_t mode) override {
        calls.push_back("logfile " + std::to_string(name) + " " +
                        std::to_string(mode));
    }
    void InitializeEventObjects() override {
        calls.push_back("init-objects");
    }
    std::uint32_t TimeGetTime() override {
        calls.push_back("time");
        return 0x1234u;
    }
    void SeedRandomNumberGenerator(std::uint32_t value) override {
        calls.push_back("opaque " + std::to_string(value));
    }
    std::uint32_t SingletonManagerInstance() override {
        calls.push_back("manager");
        return 0x6000u;
    }
    std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t id, std::uint32_t factory,
        std::uint32_t cleanup) override {
        calls.push_back("register " + std::to_string(manager) + " " +
                        std::to_string(name) + " " +
                        std::to_string(id) + " " +
                        std::to_string(factory) + " " +
                        std::to_string(cleanup));
        return registration;
    }
    int RegisterAtExit(std::uint32_t callback) override {
        calls.push_back("atexit " + std::to_string(callback));
        return 7;
    }
};

void SingletonAndFactory() {
    Runtime runtime;
    pal4::SkillSystemGlobals globals{0u, 0xa0u};
    Check(pal4::SkillSystemLifecycle::GetInstance(
              globals, runtime) == 0x9000u &&
              globals.instance == 0x9000u && globals.guard == 0xa1u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "register 24576 9147292 1033 5808208 5808304"},
          "first singleton access sets guard and registers exact callbacks");
    runtime.calls.clear();
    Check(pal4::SkillSystemLifecycle::GetInstance(
              globals, runtime) == 0x9000u && runtime.calls.empty(),
          "cached singleton access does not register again");
    globals.instance = 0u;
    Check(pal4::SkillSystemLifecycle::GetInstance(
              globals, runtime) == 0u && runtime.calls.empty(),
          "guarded null instance is not retried");

    runtime.calls.clear();
    Check(pal4::SkillSystemLifecycle::Construct(runtime) == 0x7000u &&
              runtime.calls.size() > 12u &&
              std::vector<std::string>(runtime.calls.begin(),
                                       runtime.calls.begin() + 12u) ==
                  std::vector<std::string>{
                      "allocate 2360", "construct-logger 28672", "header-byte",
                      "byte 31016 90", "word 31020 0", "word 31024 0",
                      "word 31028 0", "logfile 9147308 0", "word 28672 2",
                      "init-objects", "time", "opaque 4660"},
          "factory allocates full skill system and initializes event state");
    const auto vector_begin = runtime.memory[0x7000u + 2348u];
    Check(runtime.memory[0x7000u + 2352u] - vector_begin == 39u * 8u &&
              runtime.memory[0x7000u + 2356u] - vector_begin == 64u * 8u,
          "core event processor registers 39 pairs in a 64-slot vector");
    constexpr std::array<std::uint32_t, 39> expected_callbacks{
        0x57fd80u, 0x57fdf0u, 0x57fea0u, 0x7e1840u,
        0x7e1840u, 0x7e1840u, 0x7e1840u, 0x5800b0u,
        0x5800f0u, 0x5801f0u, 0x5802d0u, 0x580390u,
        0x5803b0u, 0x5803d0u, 0x580510u, 0x580530u,
        0x580550u, 0x580590u, 0x5806d0u, 0x5808e0u,
        0x580a70u, 0x580ba0u, 0x7e1840u, 0x7e1840u,
        0x580ca0u, 0x580d40u, 0x580d70u, 0x580dc0u,
        0x580f20u, 0x5810b0u, 0x581140u, 0x581180u,
        0x5811c0u, 0x581210u, 0x581250u, 0x581280u,
        0x581380u, 0x5813f0u, 0x581430u,
    };
    for (std::size_t i = 0u; i < expected_callbacks.size(); ++i) {
        const auto id = i < 2u ? 9000u + static_cast<std::uint32_t>(i) :
                        9100u + static_cast<std::uint32_t>(i - 2u);
        const auto entry = vector_begin + static_cast<std::uint32_t>(i) * 8u;
        Check(runtime.memory[entry] == id &&
                  runtime.memory[entry + 4u] == expected_callbacks[i],
              "every event ID is paired with its exact IDA callback address");
    }
    const auto prior_allocations = runtime.allocations;
    pal4::SkillSystemEventRegistry::Initialize(0x7000u, runtime);
    Check(runtime.allocations == prior_allocations &&
              runtime.memory[0x7000u + 2348u] == vector_begin &&
              runtime.memory[0x7000u + 2352u] == vector_begin + 39u * 8u,
          "repeat registry initialization clears logical length and reuses capacity");

    Runtime failed;
    failed.allocation = 0u;
    Check(pal4::SkillSystemLifecycle::Construct(failed) == 0u &&
              failed.calls == std::vector<std::string>{"allocate 2360"},
          "allocation failure skips all constructor work");
}

void Cleanup() {
    Runtime runtime;
    runtime.memory[0x7000u + 2348u] = 0x8000u;
    Check(pal4::SkillSystemLifecycle::CleanupResource(
              0x7000u, runtime) == 99u &&
              runtime.calls == std::vector<std::string>{
                  "read 31020", "free 32768", "word 31020 0",
                  "word 31024 0", "word 31028 0",
                  "cleanup-logger 28672", "free 28672"},
          "resource cleanup frees event vector before logger and object");
    runtime.calls.clear();
    Check(pal4::SkillSystemLifecycle::CleanupResource(
              0u, runtime) == 0u && runtime.calls.empty(),
          "null resource cleanup is defined and inert");
    runtime.calls.clear();
    pal4::SkillSystemLifecycle::CleanupEventData(0x7000u, runtime);
    Check(runtime.calls.front() == "read 31020" &&
              runtime.calls.back() == "cleanup-logger 28672",
          "direct event cleanup still calls logger cleanup");
}

void SharedCrtRegistration() {
    Runtime runtime;
    std::uint8_t guard = 0xe0u;
    Check(pal4::CrtNoopAtExitRegistration::Run(guard, runtime) == 7 &&
              guard == 0xe1u &&
              runtime.calls == std::vector<std::string>{"atexit 6161760"},
          "nearby CRT wrapper reuses the existing no-op registration");
}

}  // namespace

int main() {
    try {
        SingletonAndFactory();
        Cleanup();
        SharedCrtRegistration();
        std::cout << "skill system lifecycle tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
