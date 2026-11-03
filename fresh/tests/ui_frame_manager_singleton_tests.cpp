#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameManagerSingletonRuntime {
    std::vector<std::string> events;
    bool fail_registration = false;
    bool fail_construction = false;
    std::uint32_t allocation = 0x5000u;

    std::uint32_t SingletonManager() override {
        events.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t Register(std::uint32_t manager,
                           const char* name,
                           std::uint32_t singleton_id,
                           std::uint32_t factory) override {
        Check(manager == 0x4000u &&
                  std::string(name) == "uiFrameManager" &&
                  singleton_id == 0x3ffu &&
                  factory == pal4::UiFrameManagerSingleton::
                                 kFactoryAddress,
              "singleton registration fields");
        events.emplace_back("register");
        if (fail_registration)
            throw std::runtime_error("register");
        return 0x6000u;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 0x224u, "instance allocation size");
        events.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t Construct(std::uint32_t address) override {
        Check(address == 0x5000u,
              "constructor receives allocation");
        events.emplace_back("construct");
        if (fail_construction)
            throw std::runtime_error("construct");
        return 0x5100u;
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x5000u,
              "failed allocation freed");
        events.emplace_back("free");
    }
};

void CheckRegistration() {
    Runtime runtime;
    std::uint8_t guard = 0x20u;
    std::uint32_t cached = 0u;
    Check(pal4::UiFrameManagerSingleton::GetInstance(
              guard, cached, runtime) == 0x6000u &&
              guard == 0x21u && cached == 0x6000u &&
              runtime.events ==
                  std::vector<std::string>{
                      "manager", "register"},
          "first use registers and caches instance");
    runtime.events.clear();
    Check(pal4::UiFrameManagerSingleton::GetInstance(
              guard, cached, runtime) == 0x6000u &&
              runtime.events.empty(),
          "guarded second use returns cache");

    Runtime failed;
    failed.fail_registration = true;
    guard = 0x80u;
    cached = 0x1234u;
    bool threw = false;
    try {
        (void)pal4::UiFrameManagerSingleton::GetInstance(
            guard, cached, failed);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && guard == 0x81u && cached == 0x1234u,
          "registration failure leaves guard set and cache untouched");
}

void CheckFactory() {
    Runtime runtime;
    Check(pal4::UiFrameManagerSingleton::CreateInstance(
              runtime) == 0x5100u &&
              runtime.events ==
                  std::vector<std::string>{
                      "allocate", "construct"},
          "successful instance factory returns constructor result");

    Runtime null_allocation;
    null_allocation.allocation = 0u;
    Check(pal4::UiFrameManagerSingleton::CreateInstance(
              null_allocation) == 0u &&
              null_allocation.events ==
                  std::vector<std::string>{"allocate"},
          "null allocation skips constructor");

    Runtime failure;
    failure.fail_construction = true;
    bool threw = false;
    try {
        (void)pal4::UiFrameManagerSingleton::CreateInstance(
            failure);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failure.events ==
                       std::vector<std::string>{
                           "allocate", "construct", "free"},
          "constructor failure frees allocated instance");
}

}  // namespace

int main() {
    try {
        CheckRegistration();
        CheckFactory();
        std::cout << "ui_frame_manager_singleton_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
