#include "pal4/equipment_window_registration.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::EquipmentWindowRegistrationRuntimeStub {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t allocation{0x5000u};
    bool fail_construct{};
    bool fail_insert{};

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate:" + std::to_string(bytes));
        return allocation;
    }
    std::uint32_t Construct(std::uint32_t storage,
                            std::uint32_t name) override {
        Check(storage == 0x5000u && name == 0x7000u,
              "factory forwards allocation and name");
        calls.emplace_back("construct");
        if (fail_construct) throw std::runtime_error("construct");
        return storage;
    }
    void Free(std::uint32_t storage) override {
        Check(storage == 0x5000u, "failed object storage freed");
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
    std::uint32_t RegisterFactory(std::uint32_t manager,
                                  std::uint32_t name,
                                  Creator creator) override {
        Check(manager == 0x8000u && name == 0x9000u &&
                  creator == &pal4::EquipmentWindowRegistration::CreateObject,
              "factory registers equipment creator with cached name");
        calls.emplace_back("register-factory");
        return 0xabceu;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write");
    }
    void InsertOrReplace(std::uint32_t tree,
                         std::string_view command,
                         std::uint32_t& callback) override {
        Check(tree == 0x8104u && command == "show_equipment_ui" &&
                  callback == allocation,
              "show command enters manager tree");
        calls.emplace_back("insert");
        if (fail_insert) throw std::runtime_error("insert");
    }
    void DeleteCallback(std::uint32_t callback,
                        std::uint32_t flags) override {
        Check(callback == 0x5000u && flags == 1u,
              "temporary callback uses deleting destructor");
        calls.emplace_back("delete");
    }
};

void CheckNormal() {
    using Registration = pal4::EquipmentWindowRegistration;
    const char* name = nullptr;
    Check(std::string_view(Registration::RegisterName(name)) ==
                  "EquipmentWindow" && name != nullptr,
          "name registration stores equipment literal");
    name = nullptr;
    Check(Registration::RegisterNameThunk(name) == name &&
                  std::string_view(name) == "EquipmentWindow" &&
                  Registration::kNameCacheAddress == 0x8e4544u,
          "name thunk and cache address");
    Runtime runtime;
    Check(Registration::CreateObject(0x7000u, runtime) == 0x5000u &&
                  runtime.calls == std::vector<std::string>{
                      "allocate:308", "construct"},
          "factory allocates 0x134 bytes");
    runtime.calls.clear();
    Check(Registration::RegisterFactoryThunk(runtime) == 0xabceu &&
                  runtime.calls == std::vector<std::string>{
                      "manager", "cached-name", "register-factory"},
          "factory thunk forwards manager registration");
    runtime = Runtime{};
    Check(Registration::RegisterShowCommandThunk(runtime) == 0x8e4540u &&
                  runtime.words.at(0x5000u) == 0x843494u &&
                  runtime.words.at(0x5004u) == 0x9100u &&
                  runtime.calls == std::vector<std::string>{
                      "literal-name", "allocate:8", "write", "write",
                      "manager", "insert", "delete"},
          "show command constructs and releases callback");
}

void CheckFailures() {
    using Registration = pal4::EquipmentWindowRegistration;
    Runtime runtime;
    runtime.allocation = 0u;
    Check(Registration::CreateObject(0x7000u, runtime) == 0u &&
                  runtime.calls == std::vector<std::string>{"allocate:308"},
          "null allocation skips constructor");
    runtime = Runtime{};
    runtime.fail_construct = true;
    try {
        (void)Registration::CreateObject(0x7000u, runtime);
        Check(false, "constructor failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "allocate:308", "construct", "free"},
              "constructor failure frees storage");
    }
    runtime = Runtime{};
    runtime.fail_insert = true;
    try {
        (void)Registration::RegisterShowCommand(runtime);
        Check(false, "insertion failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "delete",
              "insertion failure releases temporary callback");
    }
    runtime = Runtime{};
    runtime.allocation = 0u;
    Check(Registration::RegisterShowCommand(runtime) == 0x8e4540u &&
                  runtime.calls == std::vector<std::string>{
                      "literal-name", "allocate:8", "manager", "insert"},
          "null callback still reaches command tree");
}

}  // namespace

int main() {
    CheckNormal();
    CheckFailures();
    std::cout << "Equipment window registration tests passed\n";
}
