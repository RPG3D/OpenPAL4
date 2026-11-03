#include "pal4/scene_ui_registration.h"

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

struct ExitApi final : pal4::ICrtAtexitApi {
    int calls = 0;
    int result = 19;
    int Register(pal4::CrtExitCallback callback) override {
        Check(callback != nullptr, "exit callback exists");
        callback();
        ++calls;
        return result;
    }
};

struct Runtime final : pal4::ISceneUiRegistrationRuntime {
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x7000u;
    std::uint32_t constructed = 0x7100u;
    std::uint32_t factory_result = 0x7200u;
    bool throw_construct = false;
    bool throw_insert = false;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        return allocation;
    }
    std::uint32_t Construct(std::uint32_t storage,
                            std::uint32_t name) override {
        Check(storage == allocation && name == 0x8b0460u,
              "scene constructor arguments");
        calls.emplace_back("construct");
        if (throw_construct) throw std::runtime_error("construct");
        return constructed;
    }
    void Free(std::uint32_t storage) override {
        Check(storage == allocation, "scene storage freed");
        calls.emplace_back("free");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    std::uint32_t ProcessFactory(
        std::uint32_t manager, std::uint32_t name, Creator creator) override {
        Check(manager == 0x8000u && name == 0x8b0460u &&
                  creator == &pal4::SceneUiRegistration::CreateObject,
              "scene factory arguments and identity");
        calls.emplace_back("factory");
        return factory_result;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
    }
    void InsertOrReplace(std::uint32_t tree, std::uint32_t command,
                         std::uint32_t& callback) override {
        Check(tree == 0x8104u && command == 0x8a80a4u &&
                  callback == allocation,
              "scene command tree, key and callback");
        calls.emplace_back("insert");
        if (throw_insert) throw std::runtime_error("insert");
    }
    void DeleteCallback(std::uint32_t callback,
                        std::uint32_t flags) override {
        Check(callback == allocation && flags == 1u,
              "local scene callback released");
        calls.emplace_back("delete");
    }
};

void CheckGuardAndName() {
    ExitApi api;
    std::uint8_t guard = 0x82u;
    Check(pal4::SceneUiRegistration::InitializeGuard(guard, api) == 19 &&
              guard == 0x83u && api.calls == 1,
          "scene guard sets bit zero and registers cleanup");
    api.result = -3;
    Check(pal4::SceneUiRegistration::InitializeGuard(guard, api) == -3 &&
              guard == 0x83u && api.calls == 2,
          "already-set guard still registers and forwards result");
    std::uint32_t cache = 0u;
    Check(pal4::SceneUiRegistration::RegisterNameThunk(cache) == 0x8b0460u &&
              cache == 0x8b0460u,
          "name thunk caches portrait literal");
    cache = 0u;
    Check(pal4::SceneUiRegistration::RegisterName(cache) == 0x8b0460u &&
              cache == 0x8b0460u,
          "name entry returns cached literal");
    Check(pal4::SceneUiRegistration::kGuardAddress == 0x978b3cu &&
              pal4::SceneUiRegistration::kNamePointerCell == 0x8b045cu &&
              pal4::SceneUiRegistration::kNameCache == 0x8e795cu &&
              pal4::SceneUiRegistration::kCommandResult == 0x8e7958u,
          "scene global addresses match IDA");
}

void CheckCreationAndFactory() {
    Runtime runtime;
    Check(pal4::SceneUiRegistration::CreateObject(0x8b0460u, runtime) ==
              0x7100u && runtime.calls ==
              std::vector<std::string>{"allocate:248", "construct"},
          "scene object allocation and construction");

    runtime.calls.clear();
    runtime.allocation = 0u;
    Check(pal4::SceneUiRegistration::CreateObject(0x8b0460u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate:248"},
          "allocation failure skips constructor");

    runtime.allocation = 0x7000u;
    runtime.throw_construct = true;
    runtime.calls.clear();
    bool caught = false;
    try {
        (void)pal4::SceneUiRegistration::CreateObject(0x8b0460u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls ==
              std::vector<std::string>{"allocate:248", "construct", "free"},
          "constructor exception frees object storage");

    runtime.throw_construct = false;
    runtime.calls.clear();
    Check(pal4::SceneUiRegistration::RegisterFactoryThunk(0x8b0460u, runtime) ==
              0x7200u && runtime.calls ==
              std::vector<std::string>{"manager", "factory"},
          "factory thunk forwards manager, name and creator");
    runtime.calls.clear();
    Check(pal4::SceneUiRegistration::RegisterFactory(0x8b0460u, runtime) ==
              0x7200u && runtime.calls ==
              std::vector<std::string>{"manager", "factory"},
          "factory entry returns manager result");
}

void CheckShowCommand() {
    Runtime runtime;
    const std::vector<std::string> expected{
        "allocate:8", "write:28676:9110624", "write:28672:8664212",
        "manager", "insert", "delete"};
    Check(pal4::SceneUiRegistration::RegisterShowCommandThunk(runtime) ==
              0x8e7958u && runtime.calls == expected,
          "show command thunk writes callback and releases local owner");
    runtime.calls.clear();
    Check(pal4::SceneUiRegistration::RegisterShowCommand(runtime) ==
              0x8e7958u && runtime.calls == expected,
          "show command entry matches thunk");

    runtime.calls.clear();
    runtime.allocation = 0u;
    Check(pal4::SceneUiRegistration::RegisterShowCommand(runtime) ==
              0x8e7958u && runtime.calls ==
              std::vector<std::string>{"allocate:8", "manager", "insert"},
          "failed callback allocation still inserts null owner");

    runtime.allocation = 0x7000u;
    runtime.throw_insert = true;
    runtime.calls.clear();
    bool caught = false;
    try {
        (void)pal4::SceneUiRegistration::RegisterShowCommand(runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls == expected,
          "insertion exception releases callback once");
}

}  // namespace

int main() {
    CheckGuardAndName();
    CheckCreationAndFactory();
    CheckShowCommand();
}
