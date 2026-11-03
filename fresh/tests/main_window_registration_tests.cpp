#include "pal4/main_window_registration.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MainWindowFactoryRuntimeStub {
    std::uint32_t allocation = 0x6000u;
    std::uint32_t result = 0x6010u;
    bool throw_in_constructor{};
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x6cu, "MainWindow allocation is 0x6c bytes");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t Construct(std::uint32_t storage,
                            std::uint32_t name) override {
        Check(storage == 0x6000u && name == 0x2000u,
              "factory passes name to MainWindow constructor");
        calls.emplace_back("construct");
        if (throw_in_constructor) throw std::runtime_error("construct");
        return result;
    }
    void Free(std::uint32_t storage) override {
        Check(storage == 0x6000u,
              "constructor unwind frees allocated storage");
        calls.emplace_back("free");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x3000u;
    }
    std::uint32_t RegisteredName() override {
        calls.emplace_back("registered-name");
        return 0x2000u;
    }
    std::uint32_t RegisterFactory(
        std::uint32_t manager, std::uint32_t name,
        Creator creator) override {
        Check(manager == 0x3000u && name == 0x2000u &&
                  creator == &pal4::MainWindowFactory::CreateObject,
              "manager registers MainWindow creator and name");
        calls.emplace_back("register-factory");
        return 0x4444u;
    }
};

}  // namespace

int main() {
    const char* name = nullptr;
    Check(pal4::MainWindowRegistration::RegisterName(name) == name &&
              std::string{name} == "MainWindow",
          "registration stores IDA MainWindow literal");
    name = nullptr;
    Check(pal4::MainWindowRegistration::RegisterNameThunk(name) == name &&
              std::string{name} == "MainWindow",
          "name thunk forwards pointer identity");
    Runtime runtime;
    Check(pal4::MainWindowFactory::CreateObject(0x2000u, runtime) ==
              0x6010u &&
              runtime.calls ==
                  (std::vector<std::string>{"allocate", "construct"}),
          "factory returns MainWindow constructor result");
    runtime.calls.clear();
    runtime.allocation = 0u;
    Check(pal4::MainWindowFactory::CreateObject(0x2000u, runtime) == 0u &&
              runtime.calls ==
                  (std::vector<std::string>{"allocate"}),
          "null allocation skips constructor");
    runtime.allocation = 0x6000u;
    runtime.calls.clear();
    runtime.throw_in_constructor = true;
    try {
        (void)pal4::MainWindowFactory::CreateObject(0x2000u, runtime);
        Check(false, "constructor failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls ==
                  (std::vector<std::string>{
                      "allocate", "construct", "free"}),
              "constructor failure releases storage");
    }
    runtime.calls.clear();
    Check(pal4::MainWindowFactory::RegisterCreateObject(runtime) ==
              0x4444u &&
              runtime.calls ==
                  (std::vector<std::string>{
                      "manager", "registered-name", "register-factory"}),
          "factory registration follows manager/name order");
    runtime.calls.clear();
    Check(pal4::MainWindowFactory::RegisterCreateObjectThunk(runtime) ==
              0x4444u && runtime.calls.back() == "register-factory",
          "factory thunk forwards registration result");
    std::cout << "MainWindow registration tests passed\n";
}
