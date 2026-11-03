#include "pal4/cpu_register_formatter.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::ICpuRegisterRuntime {
public:
    [[nodiscard]] bool IsBadReadPointer(
        const std::uint32_t address,
        const std::uint32_t bytes) override {
        Check(address == 0x1000u && bytes == 8u,
              "register formatter validates exception pointer record");
        ++checks;
        return bad_pointer;
    }

    [[nodiscard]] std::uint32_t Read(
        const std::uint32_t address) const override {
        const auto found = words.find(address);
        return found == words.end() ? 0u : found->second;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    int checks = 0;
    bool bad_pointer = false;
};

void CheckRegisterOrderingAndWidth() {
    Runtime runtime;
    runtime.words[0x1004u] = 0x3000u;
    const std::uint32_t indexes[] = {
        44u, 41u, 43u, 42u, 40u, 39u, 45u, 49u,
        46u, 48u, 47u, 38u, 50u, 37u, 36u, 35u};
    for (std::uint32_t index = 0u; index < 16u; ++index) {
        runtime.words[0x3000u + indexes[index] * 4u] = index + 1u;
    }
    pal4::CpuRegisterFormatter formatter;
    Check(formatter.Generate(0x1000u, runtime) != nullptr &&
              formatter.Report() ==
                  "EAX=00000001  EBX=00000002  ECX=00000003  "
                  "EDX=00000004  ESI=00000005\n"
                  "EDI=00000006  EBP=00000007  ESP=00000008  "
                  "EIP=00000009  FLG=0000000A\n"
                  "CS=000B   DS=000C  SS=000D  ES=000E   FS=000F  GS=0010",
          "all 16 registers use IDA context offsets and observed widths");
}

void CheckBadPointerPreservesLastText() {
    Runtime runtime;
    runtime.words[0x1004u] = 0x3000u;
    pal4::CpuRegisterFormatter formatter;
    Check(formatter.Generate(0x1000u, runtime) != nullptr,
          "valid record produces register text");
    const std::string previous = formatter.Report();
    runtime.bad_pointer = true;
    Check(formatter.Generate(0x1000u, runtime) == nullptr &&
              formatter.Report() == previous && runtime.checks == 2,
          "bad read pointer returns null before overwriting previous text");
}

}  // namespace

int main() {
    CheckRegisterOrderingAndWidth();
    CheckBadPointerPreservesLastText();
    std::cout << "2 CPU register formatter test cases passed\n";
}
