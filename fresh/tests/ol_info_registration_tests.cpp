#include "pal4/ol_info_registration.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::OlInfoRegistrationRuntimeStub {
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x5000u;
    bool fail_construct{};
    bool fail_insert{};
    bool replace_callback{};

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        return allocation;
    }
    std::uint32_t Construct(std::uint32_t storage,
                            std::uint32_t name) override {
        Check(storage == 0x5000u && name == 0x2000u,
              "factory passes allocated object and layout name");
        calls.emplace_back("construct");
        if (fail_construct) throw std::runtime_error("construct");
        return 0x6000u;
    }
    void Free(std::uint32_t storage) override {
        Check(storage == 0x5000u, "failed constructor frees storage");
        calls.emplace_back("free");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    std::uint32_t RegisteredName() override {
        calls.emplace_back("registered-name");
        return 0x2000u;
    }
    std::uint32_t WindowNameAddress() override {
        calls.emplace_back("literal-name");
        return 0x3000u;
    }
    std::uint32_t RegisterFactory(
        std::uint32_t manager, std::uint32_t name,
        Creator creator) override {
        Check(manager == 0x8000u && name == 0x2000u &&
                  creator == &pal4::OlInfoRegistration::CreateObject,
              "manager registers olInfo creator");
        calls.emplace_back("register-factory");
        return 0x7777u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        if (address == 0x5004u) {
            Check(value == 0x3000u,
                  "show callback targets literal olInfo name pointer");
            calls.emplace_back("target");
        } else {
            Check(address == 0x5000u && value ==
                      pal4::OlInfoRegistration::kCommandCallbackVtable,
                  "show callback uses name-dispatch vtable");
            calls.emplace_back("vtable");
        }
    }
    void InsertOrReplace(std::uint32_t tree,
                         std::string_view command,
                         std::uint32_t& callback) override {
        Check(tree == 0x8104u && command == "show_ol_info_ui" &&
                  callback == allocation,
              "command registers under manager tree +0x104");
        calls.emplace_back("insert");
        if (replace_callback) callback = 0x7000u;
        if (fail_insert) throw std::runtime_error("insert");
    }
    void DeleteCallback(std::uint32_t callback,
                        std::uint32_t flags) override {
        Check(callback == (replace_callback ? 0x7000u : 0x5000u) &&
                  flags == 1u,
              "local command owner releases callback with flag one");
        calls.emplace_back("release");
    }
};

}  // namespace

int main() {
    using Registration = pal4::OlInfoRegistration;
    const char* name = nullptr;
    Check(Registration::RegisterName(name) == name &&
              std::string_view(name) == "olInfo" &&
              Registration::RegisterNameThunk(name) == name,
          "name registrar and thunk preserve olInfo identity");

    Runtime runtime;
    Check(Registration::CreateObject(0x2000u, runtime) == 0x6000u &&
              runtime.calls == std::vector<std::string>{
                  "allocate:84", "construct"},
          "factory allocates 0x54 and forwards constructor result");
    runtime = Runtime{};
    runtime.allocation = 0u;
    Check(Registration::CreateObject(0x2000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate:84"},
          "null allocation skips constructor");
    runtime = Runtime{};
    runtime.fail_construct = true;
    try {
        (void)Registration::CreateObject(0x2000u, runtime);
        Check(false, "constructor exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "allocate:84", "construct", "free"},
              "constructor exception frees factory storage");
    }

    runtime = Runtime{};
    Check(Registration::RegisterFactory(runtime) == 0x7777u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "registered-name", "register-factory"},
          "managed factory registration preserves call order");
    runtime.calls.clear();
    Check(Registration::RegisterFactoryThunk(runtime) == 0x7777u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "registered-name", "register-factory"},
          "factory thunk forwards registered creator");

    runtime = Runtime{};
    Check(Registration::RegisterShowCommand(runtime) ==
                  Registration::kCommandResultAddress &&
              runtime.calls == std::vector<std::string>{
                  "literal-name", "allocate:8", "target", "vtable",
                  "manager", "insert", "release"},
          "show command records literal target and releases local callback");
    runtime = Runtime{};
    runtime.allocation = 0u;
    Check(Registration::RegisterShowCommandThunk(runtime) ==
                  Registration::kCommandResultAddress &&
              runtime.calls == std::vector<std::string>{
                  "literal-name", "allocate:8", "manager", "insert"},
          "null callback allocation still registers show command");
    runtime = Runtime{};
    runtime.replace_callback = true;
    runtime.fail_insert = true;
    try {
        (void)Registration::RegisterShowCommand(runtime);
        Check(false, "insert exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "release",
              "insertion exception releases replaced local callback");
    }
    std::cout << "olInfo registration tests passed\n";
}
