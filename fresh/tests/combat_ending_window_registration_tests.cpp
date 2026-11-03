#include "pal4/combat_ending_window_registration.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

struct Runtime final : pal4::ICombatEndingWindowRegistrationRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address != 0x8b6a98u)
            throw std::runtime_error("ending name source address");
        calls.emplace_back("read");
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        if (address != 0x8f9590u)
            throw std::runtime_error("ending name slot address");
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
        runtime.words[0x8b6a98u] = 0x8b6aa0u;
        Check(pal4::CombatEndingWindowRegistration::InitializeName(runtime) ==
                  0x8b6aa0u &&
                  runtime.words[0x8f9590u] == 0x8b6aa0u &&
                  runtime.calls == std::vector<std::string>{"read", "write"},
              "ending name initializer stores the CombatEndingWindow pointer");
        runtime.calls.clear();
        runtime.words[0x8b6a98u] = 0u;
        Check(pal4::CombatEndingWindowRegistration::InitializeNameThunk(
                  runtime) == 0u &&
                  runtime.words[0x8f9590u] == 0u &&
                  runtime.calls == std::vector<std::string>{"read", "write"},
              "ending name thunk forwards null and does not cache the source");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
