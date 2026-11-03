#include "pal4/combat_action_console_window_registration.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

struct Runtime final : pal4::ICombatActionConsoleWindowRegistrationRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address != 0x8b6914u)
            throw std::runtime_error("name source address");
        calls.emplace_back("read");
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        if (address != 0x8f94d4u)
            throw std::runtime_error("name slot address");
        calls.emplace_back("write");
        words[address] = value;
    }
};

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

}  // namespace

int main() {
    try {
        Runtime runtime;
        runtime.words[0x8b6914u] = 0x8b695cu;
        runtime.words[0x8f94d4u] = 0xdeadbeefu;
        Check(pal4::CombatActionConsoleWindowRegistration::InitializeName(
                  runtime) == 0x8b695cu &&
                  runtime.words[0x8f94d4u] == 0x8b695cu &&
                  runtime.calls == std::vector<std::string>{"read", "write"},
              "name initializer reads, stores and returns source pointer");
        runtime.calls.clear();
        runtime.words[0x8b6914u] = 0u;
        Check(pal4::CombatActionConsoleWindowRegistration::InitializeNameThunk(
                  runtime) == 0u &&
                  runtime.words[0x8f94d4u] == 0u &&
                  runtime.calls == std::vector<std::string>{"read", "write"},
              "thunk forwards repeated null source without caching");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
