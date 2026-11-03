#include "pal4/property_window_registration.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct AtExitRuntime final : pal4::CrtNoopAtExitRuntimeStub {
    std::vector<std::uint32_t> callbacks;
    int RegisterAtExit(std::uint32_t callback) override {
        callbacks.push_back(callback);
        return 37;
    }
};

struct Runtime final : pal4::PropertyWindowRegistrationRuntimeStub {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t allocation{0x5000u};
    bool fail_construct{};
    bool fail_insert{};

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        return allocation;
    }
    std::uint32_t Construct(std::uint32_t storage,
                            std::uint32_t name) override {
        Check(storage == 0x5000u && name == 0x7000u,
              "factory forwards storage and name");
        calls.emplace_back("construct");
        if (fail_construct) throw std::runtime_error("construct");
        return storage;
    }
    void Free(std::uint32_t storage) override {
        Check(storage == 0x5000u, "failed construction frees storage");
        calls.emplace_back("free");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    std::uint32_t RegisteredName() override {
        calls.emplace_back("cached-name");
        return 0x9000u;
    }
    std::uint32_t LiteralName() override {
        calls.emplace_back("literal-name");
        return 0x9100u;
    }
    std::uint32_t RegisterFactory(
        std::uint32_t manager, std::uint32_t name,
        Creator creator) override {
        Check(manager == 0x8000u && name == 0x9000u &&
                  creator == &pal4::PropertyWindowRegistration::CreateObject,
              "factory registration passes manager, cached name and creator");
        calls.emplace_back("register-factory");
        return 0xaaaau;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    void InsertOrReplace(std::uint32_t tree, std::string_view command,
                         std::uint32_t& callback) override {
        Check(tree == 0x8104u && command == "show_property_ui" &&
                  callback == allocation,
              "show command uses manager command tree and callback");
        calls.emplace_back("insert");
        if (fail_insert) throw std::runtime_error("insert");
    }
    void DeleteCallback(std::uint32_t callback,
                        std::uint32_t flags) override {
        Check(callback == 0x5000u && flags == 1u,
              "local callback is deleted with flag one");
        calls.emplace_back("delete");
    }
};

void TestStaticAndName() {
    using Registration = pal4::PropertyWindowRegistration;
    std::uint8_t flag = 0x80u;
    AtExitRuntime runtime;
    Check(Registration::InitializeStatic(flag, runtime) == 37 &&
              flag == 0x81u &&
              runtime.callbacks == std::vector<std::uint32_t>{0x5e0560u},
          "static initializer sets low flag and registers no-op callback");
    Check(Registration::InitializeStatic(flag, runtime) == 37 &&
              flag == 0x81u && runtime.callbacks.size() == 2u,
          "static initializer registers callback on repeat call");
    const char* name = nullptr;
    Check(std::string_view(Registration::RegisterNameThunk(name)) ==
                  "PropertyWindow" && name != nullptr &&
              Registration::kNameCacheAddress == 0x8e781cu,
          "name thunk caches PropertyWindow literal");
}

void TestFactoryAndCommand() {
    using Registration = pal4::PropertyWindowRegistration;
    Runtime runtime;
    Check(Registration::CreateObject(0x7000u, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  "allocate:184", "construct"},
          "factory allocates 0xb8 bytes before construction");
    runtime.calls.clear();
    Check(Registration::RegisterFactoryThunk(runtime) == 0xaaaau &&
              runtime.calls == std::vector<std::string>{
                  "manager", "cached-name", "register-factory"},
          "factory thunk forwards manager, cached name and creator");
    runtime.calls.clear();
    Check(Registration::RegisterShowCommandThunk(runtime) ==
                  Registration::kCommandResultAddress &&
              runtime.words.at(0x5004u) == 0x9100u &&
              runtime.words.at(0x5000u) ==
                  Registration::kCommandCallbackVtable &&
              runtime.calls == std::vector<std::string>{
                  "literal-name", "allocate:8", "write:20484",
                  "write:20480", "manager", "insert", "delete"},
          "show command writes callback, inserts and releases local owner");
}

void TestFailures() {
    using Registration = pal4::PropertyWindowRegistration;
    Runtime runtime;
    runtime.allocation = 0u;
    Check(Registration::CreateObject(0x7000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate:184"},
          "null allocation skips constructor");
    runtime = Runtime{};
    runtime.fail_construct = true;
    try {
        (void)Registration::CreateObject(0x7000u, runtime);
        Check(false, "construction exception propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "allocate:184", "construct", "free"},
              "construction exception frees storage");
    }
    runtime = Runtime{};
    runtime.fail_insert = true;
    try {
        (void)Registration::RegisterShowCommand(runtime);
        Check(false, "insertion exception propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "delete",
              "insertion exception releases callback");
    }
    runtime = Runtime{};
    runtime.allocation = 0u;
    Check(Registration::RegisterShowCommand(runtime) == 0x8e7818u &&
              runtime.calls == std::vector<std::string>{
                  "literal-name", "allocate:8", "manager", "insert"},
          "null callback still inserts command");
}

}  // namespace

int main() {
    TestStaticAndName();
    TestFactoryAndCommand();
    TestFailures();
    std::cout << "Property window registration tests passed\n";
}
