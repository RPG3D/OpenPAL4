#include "pal4/role_state_window_registration.h"

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
    int Register(pal4::CrtExitCallback callback) override {
        Check(callback != nullptr, "nullsub callback address exists");
        callback();
        ++calls;
        return 17;
    }
};

struct Runtime final : pal4::IRoleStateWindowRegistrationRuntime {
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
        Check(storage == 0x7000u && name == 0x8b00e8u,
              "constructor storage and name");
        calls.emplace_back("construct");
        if (throw_construct) throw std::runtime_error("construct");
        return constructed;
    }
    void Free(std::uint32_t storage) override {
        Check(storage == 0x7000u, "failed constructor frees storage");
        calls.emplace_back("free");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    std::uint32_t ProcessFactory(
        std::uint32_t manager, std::uint32_t name,
        Creator creator) override {
        Check(manager == 0x8000u && name == 0x8b00e8u &&
                  creator == &pal4::RoleStateWindowRegistration::CreateObject,
              "factory registration arguments");
        calls.emplace_back("process-factory");
        return factory_result;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
    }
    void InsertOrReplace(std::uint32_t tree, std::uint32_t command,
                         std::uint32_t& callback) override {
        Check(tree == 0x8104u && command == 0x8b00d8u &&
                  callback == allocation,
              "show command tree, name and callback");
        calls.emplace_back("insert");
        if (throw_insert) throw std::runtime_error("insert");
    }
    void DeleteCallback(std::uint32_t callback,
                        std::uint32_t flags) override {
        Check(callback == allocation && flags == 1u,
              "local callback ownership released");
        calls.emplace_back("delete");
    }
};

void TestGuardAndName() {
    ExitApi api;
    std::uint8_t guard = 0x82u;
    Check(pal4::RoleStateWindowRegistration::InitializeGuard(guard, api) ==
              17 && guard == 0x83u && api.calls == 1,
          "CRT guard bit set and callback registered");
    Check(pal4::RoleStateWindowRegistration::InitializeGuard(guard, api) ==
              17 && api.calls == 2,
          "CRT exit registration repeats even when guard was set");
    std::uint32_t cache = 0u;
    Check(pal4::RoleStateWindowRegistration::RegisterNameThunk(cache) ==
              0x8b00e8u && cache == 0x8b00e8u,
          "name thunk stores roleStateWindow literal");
    cache = 0u;
    Check(pal4::RoleStateWindowRegistration::RegisterName(cache) ==
              0x8b00e8u && cache == 0x8b00e8u,
          "name registration returns stored pointer");
}

void TestCreationAndFactory() {
    Runtime runtime;
    Check(pal4::RoleStateWindowRegistration::CreateObject(
              0x8b00e8u, runtime) == 0x7100u &&
              runtime.calls == std::vector<std::string>{
                  "allocate:600", "construct"},
          "factory allocates 0x258 and forwards constructor result");
    runtime.calls.clear();
    Check(pal4::RoleStateWindowRegistration::RegisterFactoryThunk(
              0x8b00e8u, runtime) == 0x7200u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "process-factory"},
          "factory thunk forwards manager, registered name and creator");

    runtime.calls.clear();
    runtime.allocation = 0u;
    Check(pal4::RoleStateWindowRegistration::CreateObject(
              0x8b00e8u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate:600"},
          "null allocation skips constructor");

    runtime.calls.clear();
    runtime.allocation = 0x7000u;
    runtime.throw_construct = true;
    try {
        static_cast<void>(pal4::RoleStateWindowRegistration::CreateObject(
            0x8b00e8u, runtime));
        Check(false, "constructor should throw");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "allocate:600", "construct", "free"},
              "constructor exception frees allocation");
    }
}

void TestShowCommand() {
    Runtime runtime;
    Check(pal4::RoleStateWindowRegistration::RegisterShowCommandThunk(
              runtime) == 0x8e7950u &&
              runtime.calls == std::vector<std::string>{
                  "allocate:8", "write:28676:9109736",
                  "write:28672:8664212", "manager", "insert", "delete"},
          "callback layout, registration, release and result address");

    runtime.calls.clear();
    runtime.allocation = 0u;
    Check(pal4::RoleStateWindowRegistration::RegisterShowCommand(runtime) ==
              0x8e7950u &&
              runtime.calls == std::vector<std::string>{
                  "allocate:8", "manager", "insert"},
          "null callback still registers command");

    runtime.calls.clear();
    runtime.allocation = 0x7000u;
    runtime.throw_insert = true;
    try {
        static_cast<void>(
            pal4::RoleStateWindowRegistration::RegisterShowCommand(runtime));
        Check(false, "insert should throw");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "delete",
              "callback released on insertion exception");
    }
}

}  // namespace

int main() {
    TestGuardAndName();
    TestCreationAndFactory();
    TestShowCommand();
}
