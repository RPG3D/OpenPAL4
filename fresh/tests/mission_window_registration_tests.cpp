#include "pal4/mission_window_registration.h"

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

struct Runtime final : pal4::MissionWindowRegistrationRuntimeStub {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t allocation{0x5000u};
    bool fail_construct{};
    bool fail_insert{};

    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back("allocate:" + std::to_string(size));
        return allocation;
    }
    std::uint32_t Construct(std::uint32_t storage,
                            std::uint32_t name) override {
        Check(storage == 0x5000u && name == 0x7000u,
              "mission factory forwards allocation and name");
        calls.emplace_back("construct");
        if (fail_construct) throw std::runtime_error("construct");
        return storage;
    }
    void Free(std::uint32_t storage) override {
        Check(storage == 0x5000u, "mission factory frees failed allocation");
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
                  creator == &pal4::MissionWindowRegistration::CreateObject,
              "mission factory registration uses cache and creator");
        calls.emplace_back("register-factory");
        return 0xaaaau;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write");
    }
    void InsertOrReplace(std::uint32_t tree, std::string_view command,
                         std::uint32_t& callback) override {
        Check(tree == 0x8104u && command == "show_mission_ui" &&
                  callback == allocation,
              "mission command uses manager tree and callback");
        calls.emplace_back("insert");
        if (fail_insert) throw std::runtime_error("insert");
    }
    void DeleteCallback(std::uint32_t callback,
                        std::uint32_t flags) override {
        Check(callback == 0x5000u && flags == 1u,
              "mission command deletes local callback");
        calls.emplace_back("delete");
    }
};

void TestRegistration() {
    using Registration = pal4::MissionWindowRegistration;
    const char* name = nullptr;
    Check(std::string_view(Registration::RegisterNameThunk(name)) ==
                  "MissionWindow" &&
              name != nullptr &&
              Registration::kNameCacheAddress == 0x8e4554u,
          "name thunk stores MissionWindow literal");
    Runtime runtime;
    Check(Registration::CreateObject(0x7000u, runtime) == 0x5000u &&
              runtime.calls ==
                  std::vector<std::string>{"allocate:140", "construct"},
          "mission factory allocates 0x8c bytes then constructs");
    runtime.calls.clear();
    Check(Registration::RegisterFactoryThunk(runtime) == 0xaaaau &&
              runtime.calls == std::vector<std::string>{
                  "manager", "cached-name", "register-factory"},
          "factory thunk registers cached name");
    runtime = Runtime{};
    Check(Registration::RegisterShowCommandThunk(runtime) ==
                  0x8e4550u &&
              runtime.words.at(0x5000u) == 0x843494u &&
              runtime.words.at(0x5004u) == 0x9100u &&
              runtime.calls == std::vector<std::string>{
                  "literal-name", "allocate:8", "write", "write",
                  "manager", "insert", "delete"},
          "show command stores callback and releases local ownership");
}

void TestFailures() {
    using Registration = pal4::MissionWindowRegistration;
    Runtime runtime;
    runtime.allocation = 0u;
    Check(Registration::CreateObject(0x7000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate:140"},
          "null factory allocation skips constructor");
    runtime = Runtime{};
    runtime.fail_construct = true;
    try {
        (void)Registration::CreateObject(0x7000u, runtime);
        Check(false, "constructor failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "allocate:140", "construct", "free"},
              "constructor failure frees mission storage");
    }
    runtime = Runtime{};
    runtime.fail_insert = true;
    try {
        (void)Registration::RegisterShowCommand(runtime);
        Check(false, "command insertion failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "delete",
              "command insertion failure deletes local callback");
    }
    runtime = Runtime{};
    runtime.allocation = 0u;
    Check(Registration::RegisterShowCommand(runtime) == 0x8e4550u &&
              runtime.calls == std::vector<std::string>{
                  "literal-name", "allocate:8", "manager", "insert"},
          "null callback still inserts mission command");
}

}  // namespace

int main() {
    TestRegistration();
    TestFailures();
    std::cout << "Mission window registration tests passed\n";
}
