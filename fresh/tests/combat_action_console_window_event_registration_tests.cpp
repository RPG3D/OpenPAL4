#include "pal4/combat_action_console_window_registration.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IWindowRegistrationRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x8b6914u, 0x8b695cu}};
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x3000u;
    std::uint32_t tree{};
    std::uint32_t key{};
    std::uint32_t handler{};
    std::uint32_t destroyed{};
    std::uint32_t flags{};
    bool fail_insert{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 8u, "callback size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t) override {}
    std::uint32_t Construct(std::uint32_t, std::uint32_t) override {
        return 0u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x5000u;
    }
    std::uint32_t RegisterFactory(std::uint32_t, std::uint32_t,
                                  std::uint32_t) override {
        return 0u;
    }
    void InsertOrReplace(std::uint32_t target, std::uint32_t event_key,
                         std::uint32_t value) override {
        calls.emplace_back("insert");
        tree = target;
        key = event_key;
        handler = value;
        if (fail_insert) throw std::runtime_error("insert");
    }
    void DestroyHandler(std::uint32_t value,
                        std::uint32_t destroy_flags) override {
        calls.emplace_back("destroy");
        destroyed = value;
        flags = destroy_flags;
    }
};

void CheckSuccess() {
    Runtime runtime;
    Check(pal4::CombatActionConsoleWindowRegistration::RegisterEventThunk(
              runtime) == 0x8f94d0u,
          "thunk forwards the fixed registration result");
    Check(runtime.words.at(0x3004u) == 0x8b695cu &&
              runtime.words.at(0x3000u) == 0x843494u,
          "callback contains name and native vtable");
    Check(runtime.tree == 0x5104u && runtime.key == 0x8b68e4u &&
              runtime.handler == 0x3000u && runtime.destroyed == 0x3000u &&
              runtime.flags == 1u,
          "event is registered and temporary callback released");
    Check(runtime.calls == (std::vector<std::string>{
              "read", "allocate", "write", "write", "manager", "insert",
              "destroy"}),
          "registration call order");
}

void CheckAllocationFailure() {
    Runtime runtime;
    runtime.allocation = 0u;
    Check(pal4::CombatActionConsoleWindowRegistration::RegisterEvent(
              runtime) == 0x8f94d0u &&
              runtime.handler == 0u && runtime.destroyed == 0u,
          "null callback is passed to the tree without destruction");
    Check(runtime.calls == (std::vector<std::string>{
              "read", "allocate", "manager", "insert"}),
          "allocation failure call order");
}

void CheckExceptionCleanup() {
    Runtime runtime;
    runtime.fail_insert = true;
    bool caught = false;
    try {
        (void)pal4::CombatActionConsoleWindowRegistration::RegisterEvent(
            runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.destroyed == 0x3000u && runtime.flags == 1u &&
              runtime.calls.back() == "destroy",
          "insertion failure releases the temporary callback");
}

}  // namespace

int main() {
    try {
        CheckSuccess();
        CheckAllocationFailure();
        CheckExceptionCleanup();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
