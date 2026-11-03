#include "pal4/global_variable_state.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGlobalVariableStateRuntime {
public:
    [[nodiscard]] std::uint32_t Read(
        const std::uint32_t address) const override {
        const auto found = words.find(address);
        return found == words.end() ? 0u : found->second;
    }

    void Write(const std::uint32_t address,
               const std::uint32_t value) override {
        words[address] = value;
    }

    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        const std::uint32_t aligned = address & ~3u;
        const unsigned shift = (address & 3u) * 8u;
        words[aligned] = (Read(aligned) & ~(0xffu << shift)) |
                         (static_cast<std::uint32_t>(value) << shift);
    }

    void Free(const std::uint32_t address) override {
        freed.push_back(address);
    }

    [[nodiscard]] std::uint32_t RegisterExitHandler(
        const std::uint32_t address) override {
        registered = address;
        byte_at_registration = Read(pal4::GlobalVariableState::kAddress);
        return registration_result;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> freed;
    std::uint32_t registered = 0u;
    std::uint32_t byte_at_registration = 0u;
    std::uint32_t registration_result = 17u;
};

void CheckSetupAndRegistration() {
    Runtime runtime;
    constexpr std::uint32_t base = pal4::GlobalVariableState::kAddress;
    runtime.words[base] = 0x88776655u;
    runtime.words[base + 4u] = 1u;
    runtime.words[base + 8u] = 2u;
    runtime.words[base + 12u] = 3u;
    Check(pal4::GlobalVariableState::Setup(0xa1b2c3d4u, runtime) == 0u,
          "setup returns zero");
    Check(runtime.Read(base) == 0x887766a1u &&
              runtime.Read(base + 4u) == 0u &&
              runtime.Read(base + 8u) == 0u &&
              runtime.Read(base + 12u) == 0u,
          "setup copies high input byte and clears three global DWORDs");

    Check(pal4::GlobalVariableState::InitializeAndRegisterExit(
              0x12345678u, runtime) == 17u,
          "initialization returns CRT exit registration result");
    Check(runtime.registered == pal4::GlobalVariableState::kCleanupEntry &&
              runtime.byte_at_registration == 0x88776612u,
          "registration sees completed setup and cleanup entry address");
}

void CheckCleanupWithAndWithoutAllocation() {
    Runtime runtime;
    constexpr std::uint32_t base = pal4::GlobalVariableState::kAddress;
    runtime.words[base + 4u] = 0x1000u;
    runtime.words[base + 8u] = 44u;
    runtime.words[base + 12u] = 55u;
    pal4::GlobalVariableState::Cleanup(runtime);
    Check(runtime.freed == std::vector<std::uint32_t>{0x1000u} &&
              runtime.Read(base + 4u) == 0u &&
              runtime.Read(base + 8u) == 0u &&
              runtime.Read(base + 12u) == 0u,
          "cleanup frees allocation then clears all three words");
    pal4::GlobalVariableState::Cleanup(runtime);
    Check(runtime.freed == std::vector<std::uint32_t>{0x1000u, 0u},
          "cleanup also forwards null pointer to MemFree");
}

}  // namespace

int main() {
    CheckSetupAndRegistration();
    CheckCleanupWithAndWithoutAllocation();
    std::cout << "2 global variable state test cases passed\n";
}
