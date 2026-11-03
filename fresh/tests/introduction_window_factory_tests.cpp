#include "pal4/introduction_window_factory.h"

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

struct Runtime final : pal4::IntroductionWindowFactoryRuntimeStub {
    std::uint32_t allocated = 0x3000u;
    bool throw_construct{};
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::size_t bytes) override {
        Check(bytes == 0xb8u, "IntroductionWindow allocation is 0xb8 bytes");
        calls.emplace_back("allocate");
        return allocated;
    }
    std::uint32_t Construct(std::uint32_t storage,
                            std::uint32_t name) override {
        Check(storage == 0x3000u && name == 0x5000u,
              "constructor receives allocated storage and name");
        calls.emplace_back("construct");
        if (throw_construct) throw std::runtime_error("construct");
        return 0x3010u;
    }
    void Free(std::uint32_t storage) override {
        Check(storage == 0x3000u,
              "constructor unwind frees the allocated storage");
        calls.emplace_back("free");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    std::uint32_t RegisteredName() override {
        calls.emplace_back("name");
        return 0x5000u;
    }
    std::uint32_t RegisterFactory(
        std::uint32_t manager, std::uint32_t name,
        pal4::IntroductionWindowCreator creator) override {
        Check(manager == 0x8000u && name == 0x5000u &&
                  creator == &pal4::IntroductionWindowFactory::CreateObject,
              "manager receives IntroductionWindow name and factory callback");
        calls.emplace_back("register");
        Check(creator(name, *this) == 0x3010u,
              "registered factory reaches reconstructed object allocation");
        return 0x9000u;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::IntroductionWindowFactory::CreateObject(
              0x5000u, runtime) == 0x3010u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "construct"},
          "object factory returns constructor result");

    runtime.calls.clear();
    runtime.allocated = 0u;
    Check(pal4::IntroductionWindowFactory::CreateObject(
              0x5000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "failed allocation skips constructor");

    runtime.calls.clear();
    runtime.allocated = 0x3000u;
    runtime.throw_construct = true;
    try {
        (void)pal4::IntroductionWindowFactory::CreateObject(
            0x5000u, runtime);
        Check(false, "constructor exception propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "allocate", "construct", "free"},
              "constructor unwind frees storage once");
    }

    runtime.calls.clear();
    runtime.throw_construct = false;
    Check(pal4::IntroductionWindowFactory::RegisterCreateObject(runtime) ==
              0x9000u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "name", "register", "allocate", "construct"},
          "registration invokes manager then callback factory");

    runtime.calls.clear();
    Check(pal4::IntroductionWindowFactory::RegisterCreateObjectThunk(
              runtime) == 0x9000u &&
              runtime.calls.front() == "manager" &&
              runtime.calls.back() == "construct",
          "registration thunk forwards through managed registration");
    std::cout << "Introduction window factory tests passed\n";
}
