#include "pal4/role_selector_registration.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IRoleSelectorRegistrationRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x8b1a5cu, 0x8b1a60u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x7000u;
    std::uint32_t bytes = 0u;
    std::uint32_t freed = 0u;
    std::uint32_t constructed_object = 0u;
    std::uint32_t constructed_name = 0u;
    std::uint32_t registered_manager = 0u;
    std::uint32_t registered_name = 0u;
    std::uint32_t registered_factory = 0u;
    bool throw_on_construct = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.emplace_back("allocate");
        bytes = size;
        return allocation;
    }
    void Free(std::uint32_t object) override {
        calls.emplace_back("free");
        freed = object;
    }
    std::uint32_t Construct(std::uint32_t object,
                            std::uint32_t name) override {
        calls.emplace_back("construct");
        constructed_object = object;
        constructed_name = name;
        if (throw_on_construct) throw std::runtime_error("construct failed");
        return object + 1u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    std::uint32_t RegisterFactory(std::uint32_t manager,
                                  std::uint32_t name,
                                  std::uint32_t factory) override {
        calls.emplace_back("register");
        registered_manager = manager;
        registered_name = name;
        registered_factory = factory;
        return 0xabcdu;
    }
};

void CheckName(bool thunk) {
    Runtime runtime;
    const auto result = thunk
        ? pal4::RoleSelectorRegistration::InitializeNameThunk(runtime)
        : pal4::RoleSelectorRegistration::InitializeName(runtime);
    Check(result == 0x8b1a60u &&
          runtime.words.at(0x8edfa0u) == 0x8b1a60u &&
          runtime.calls == std::vector<std::string>{"read", "write"},
          "name pointer initializer and thunk");
}

void CheckCreate(bool allocate, bool throws) {
    Runtime runtime;
    if (!allocate) runtime.allocation = 0u;
    runtime.throw_on_construct = throws;
    bool caught = false;
    try {
        const auto result = pal4::RoleSelectorRegistration::Create(
            0x8b1a60u, runtime);
        Check(result == (allocate ? 0x7001u : 0u),
              "factory return");
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught == throws && runtime.bytes == 0x160u,
          "factory allocation and exception");
    Check(runtime.calls ==
              (throws
                   ? std::vector<std::string>{
                         "allocate", "construct", "free"}
                   : allocate
                       ? std::vector<std::string>{"allocate", "construct"}
                       : std::vector<std::string>{"allocate"}) &&
          runtime.freed == (throws ? 0x7000u : 0u),
          "construction gate and exception cleanup");
    if (allocate)
        Check(runtime.constructed_object == 0x7000u &&
              runtime.constructed_name == 0x8b1a60u,
              "constructor arguments");
}

void CheckRegister(bool thunk) {
    Runtime runtime;
    runtime.words[0x8edfa0u] = 0x8b1a60u;
    const auto result = thunk
        ? pal4::RoleSelectorRegistration::RegisterFactoryThunk(runtime)
        : pal4::RoleSelectorRegistration::RegisterFactory(runtime);
    Check(result == 0xabcdu &&
          runtime.registered_manager == 0x8000u &&
          runtime.registered_name == 0x8b1a60u &&
          runtime.registered_factory == 0x4b62d0u &&
          runtime.calls == std::vector<std::string>{
              "manager", "read", "register"},
          "manager registration and thunk");
}

}  // namespace

int main() {
    try {
        CheckName(false);
        CheckName(true);
        CheckCreate(false, false);
        CheckCreate(true, false);
        CheckCreate(true, true);
        CheckRegister(false);
        CheckRegister(true);
        std::cout << "role_selector_registration_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
