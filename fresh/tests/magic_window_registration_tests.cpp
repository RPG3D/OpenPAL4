#include "pal4/magic_window_registration.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}

struct Runtime final : pal4::MagicWindowRegistrationRuntimeStub {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t allocation{0x5000u};
    bool fail_construct{};
    bool fail_insert{};

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        return allocation;
    }
    std::uint32_t Construct(std::uint32_t storage, std::uint32_t name) override {
        Check(storage == 0x5000u && name == 0x7000u, "constructor inputs");
        calls.push_back("construct");
        if (fail_construct) throw std::runtime_error("construct");
        return storage;
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x5000u, "failed construction storage");
        calls.push_back("free");
    }
    std::uint32_t FrameManager() override { calls.push_back("manager"); return 0x8000u; }
    std::uint32_t CachedName() override { calls.push_back("cached-name"); return 0x9000u; }
    std::uint32_t LiteralName() override { calls.push_back("literal-name"); return 0x9100u; }
    std::uint32_t RegisterFactory(std::uint32_t manager, std::uint32_t name,
                                   Creator creator) override {
        Check(manager == 0x8000u && name == 0x9000u &&
                  creator == &pal4::MagicWindowRegistration::CreateObject,
              "magic factory inputs");
        calls.push_back("factory");
        return 0xabceu;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write");
    }
    void InsertOrReplace(std::uint32_t tree, std::string_view command,
                         std::uint32_t& callback) override {
        Check(tree == 0x8104u && command == "show_magic_ui" &&
                  callback == allocation, "magic command inputs");
        calls.push_back("insert");
        if (fail_insert) throw std::runtime_error("insert");
    }
    void DeleteCallback(std::uint32_t address, std::uint32_t flags) override {
        Check(address == 0x5000u && flags == 1u, "temporary callback cleanup");
        calls.push_back("delete");
    }
};

void TestNormal() {
    using Registration = pal4::MagicWindowRegistration;
    const char* name = nullptr;
    Check(std::string_view(Registration::RegisterNameThunk(name)) == "magicWindow" &&
              Registration::RegisterName(name) == name &&
              Registration::kNameCacheAddress == 0x8e454cu, "magic name and cache");
    Runtime runtime;
    Check(Registration::CreateObject(0x7000u, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{"allocate:480", "construct"},
          "magic object allocation");
    runtime.calls.clear();
    Check(Registration::RegisterFactoryThunk(runtime) == 0xabceu &&
              runtime.calls == std::vector<std::string>{"manager", "cached-name", "factory"},
          "magic factory registration");
    runtime = Runtime{};
    Check(Registration::RegisterShowCommandThunk(runtime) == 0x8e4548u &&
              runtime.words.at(0x5000u) == 0x843494u &&
              runtime.words.at(0x5004u) == 0x9100u &&
              runtime.calls == std::vector<std::string>{
                  "literal-name", "allocate:8", "write", "write", "manager", "insert", "delete"},
          "magic show command layout and cleanup");
}

void TestFailures() {
    using Registration = pal4::MagicWindowRegistration;
    Runtime runtime;
    runtime.allocation = 0;
    Check(Registration::CreateObject(0x7000u, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{"allocate:480"},
          "allocation failure");
    runtime = Runtime{};
    runtime.fail_construct = true;
    try { (void)Registration::CreateObject(0x7000u, runtime); Check(false, "expected throw"); }
    catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{"allocate:480", "construct", "free"},
              "constructor failure cleanup");
    }
    runtime = Runtime{};
    runtime.fail_insert = true;
    try { (void)Registration::RegisterShowCommand(runtime); Check(false, "expected throw"); }
    catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "delete", "insert failure cleanup");
    }
    runtime = Runtime{};
    runtime.allocation = 0;
    Check(Registration::RegisterShowCommand(runtime) == 0x8e4548u &&
              runtime.calls == std::vector<std::string>{
                  "literal-name", "allocate:8", "manager", "insert"},
          "null callback reaches tree");
}

}  // namespace

int main() { TestNormal(); TestFailures(); }
