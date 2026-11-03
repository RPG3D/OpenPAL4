#include "pal4/load_window_factory.h"
#include "pal4/load_window_lifecycle.h"

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

struct Runtime final : pal4::LoadWindowFactoryRuntimeStub {
    std::uint32_t storage = 0x3000u;
    std::uint32_t construction_result = 0x3000u;
    std::uint32_t registration_result = 0x5678u;
    pal4::LoadWindowCreator creator{};
    bool fail_construct{};
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::size_t bytes) override {
        Check(bytes == 0x64u, "load object allocation is 0x64 bytes");
        calls.emplace_back("allocate");
        return storage;
    }
    std::uint32_t Construct(std::uint32_t address,
                            std::uint32_t name) override {
        Check(address == 0x3000u && name == 0x2000u,
              "constructor receives allocated object and name");
        calls.emplace_back("construct");
        if (fail_construct) throw std::runtime_error("construct");
        return construction_result;
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x3000u, "exception frees allocated object");
        calls.emplace_back("free");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x9000u;
    }
    std::uint32_t RegisteredName() override {
        calls.emplace_back("name");
        return 0x2000u;
    }
    std::uint32_t RegisterFactory(std::uint32_t manager,
                                  std::uint32_t name,
                                  pal4::LoadWindowCreator callback) override {
        Check(manager == 0x9000u && name == 0x2000u &&
                  callback != nullptr,
              "factory registration receives manager and registered name");
        calls.emplace_back("register");
        creator = callback;
        return registration_result;
    }
};

struct IntegratedRuntime final : pal4::LoadWindowFactoryRuntimeStub,
                                 pal4::LoadWindowLifecycleRuntimeStub {
    pal4::LoadWindowState state{};
    std::uint32_t Allocate(std::size_t bytes) override {
        Check(bytes == 0x64u, "integrated allocation uses load object size");
        return 0x3000u;
    }
    std::uint32_t Construct(std::uint32_t storage,
                            std::uint32_t name) override {
        Check(storage == 0x3000u && name == 0x2000u,
              "factory passes object and name to lifecycle");
        (void)pal4::LoadWindowLifecycle::Construct(state, name, *this);
        return storage;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::LoadWindowFactory::CreateObject(0x2000u, runtime) ==
              0x3000u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "construct"},
          "factory allocates and constructs object");

    runtime.calls.clear();
    runtime.storage = 0u;
    Check(pal4::LoadWindowFactory::CreateObject(0x2000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "allocation failure skips construction");

    runtime.calls.clear();
    runtime.storage = 0x3000u;
    runtime.fail_construct = true;
    try {
        (void)pal4::LoadWindowFactory::CreateObject(0x2000u, runtime);
        Check(false, "construction failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "allocate", "construct", "free"},
              "constructor exception releases allocated storage");
    }

    runtime.calls.clear();
    runtime.fail_construct = false;
    Check(pal4::LoadWindowFactory::RegisterCreateObject(runtime) ==
              0x5678u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "name", "register"},
          "managed registration forwards factory and result");
    runtime.calls.clear();
    Check(pal4::LoadWindowFactory::RegisterCreateObjectThunk(runtime) ==
              0x5678u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "name", "register"},
          "managed thunk follows registration path");
    runtime.calls.clear();
    Check(runtime.creator != nullptr &&
              runtime.creator(0x2000u, runtime) == 0x3000u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "construct"},
          "registered creator invokes load object factory");

    IntegratedRuntime integrated;
    Check(pal4::LoadWindowFactory::CreateObject(
              0x2000u, integrated) == 0x3000u &&
              integrated.state.Read(0u) ==
                  pal4::LoadWindowLifecycle::kObjectVtable &&
              integrated.state.Read(0x28u) ==
                  pal4::LoadWindowLifecycle::kMemberVtable,
          "factory construction reaches the class lifecycle");
    std::cout << "Load window factory tests passed\n";
}
