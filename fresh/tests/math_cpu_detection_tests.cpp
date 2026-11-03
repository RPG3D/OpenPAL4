#include "pal4/math_cpu_detection.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Provider final : public pal4::ICpuIdProvider {
public:
    bool Query(std::uint32_t leaf, pal4::CpuIdRegisters& result) override {
        calls.push_back(leaf);
        const auto found = leaves.find(leaf);
        if (found == leaves.end()) return false;
        result = found->second;
        return true;
    }
    std::map<std::uint32_t, pal4::CpuIdRegisters> leaves;
    std::vector<std::uint32_t> calls;
};

void CheckBrand() {
    Provider provider;
    pal4::MathCpuDetection cpu(provider);
    std::array<char, 50> output{};
    output.fill('!');
    Check(!cpu.GetBrand(nullptr, 49u) && provider.calls.empty(),
          "null destination does not query CPUID");
    Check(!cpu.GetBrand(output.data(), 48u) && provider.calls.empty(),
          "short destination does not query CPUID");
    provider.leaves[0x80000000u].eax = 0x80000003u;
    Check(!cpu.GetBrand(output.data(), 49u) && output[0] == '!',
          "unsupported brand leaves output untouched");
    provider.leaves[0x80000000u].eax = 0x80000004u;
    constexpr char brand[] = "Example CPU brand fixture for PAL4 math module";
    std::array<char, 48> raw{};
    std::memcpy(raw.data(), brand, sizeof(brand));
    for (std::uint32_t index = 0; index < 3; ++index) {
        std::uint32_t words[4]{};
        std::memcpy(words, raw.data() + index * 16u, 16u);
        provider.leaves[0x80000002u + index] = {
            words[0], words[1], words[2], words[3]};
    }
    provider.calls.clear();
    Check(cpu.GetBrand(output.data(), 49u) &&
          std::strcmp(output.data(), brand) == 0 && output[49] == '!' &&
          provider.calls == std::vector<std::uint32_t>({
              0x80000000u, 0x80000002u, 0x80000003u, 0x80000004u}),
          "brand word order, leaf order and destination bound");
}

void CheckInfo() {
    Provider provider;
    provider.leaves[1u] = {0x12345678u, 0u, 0u, 0u};
    provider.leaves[3u] = {0u, 0u, 0x9ABCDEF0u, 0x13579BDFu};
    pal4::MathCpuDetection cpu(provider);
    std::array<char, 31> output{};
    output.fill('!');
    char* end = cpu.WriteInfo(true, output.data(), 30u);
    Check(end == output.data() + 29u &&
          std::strcmp(output.data(), "1234-5678-1357-9BDF-9ABC-DEF0") == 0 &&
          output[30] == '!' &&
          provider.calls == std::vector<std::uint32_t>({1u, 3u}),
          "serial register order, uppercase nibbles, separators and return");
    Check(cpu.WriteInfo(false, output.data(), 25u) == output.data() + 24u &&
          std::strcmp(output.data(), "1234567813579BDF9ABCDEF0") == 0,
          "serial without separators");
    provider.leaves.erase(3u);
    output.fill('!');
    Check(cpu.WriteInfo(true, output.data(), 30u) == nullptr &&
          output[0] == '!', "provider failure leaves destination untouched");
}

void CheckFeaturesAndIntegration() {
    Provider provider;
    provider.leaves[1u].edx = (1u << 23u) | (1u << 26u);
    provider.leaves[0x80000000u].eax = 0x80000001u;
    provider.leaves[0x80000001u].edx = 1u << 31u;
    pal4::MathCpuDetection cpu(provider);
    Check(cpu.HasMmx() && !cpu.HasSse() && cpu.HasSse2() && cpu.Has3DNow(),
          "MMX/SSE/SSE2/3DNow bits are independent");
    pal4::MathCpuInitializationRuntime runtime(provider);
    Check(runtime.HasMmx() == 1u && runtime.HasSse() == 0u &&
          runtime.HasSse2() == 1u && runtime.Has3DNow() == 1u,
          "math initializer adapter uses CPU detector");
    provider.leaves.erase(1u);
    provider.leaves.erase(0x80000001u);
    Check(!cpu.HasMmx() && !cpu.HasSse() && !cpu.HasSse2() &&
          !cpu.Has3DNow(), "failed CPUID queries do not report features");
}

}  // namespace

int main() {
    CheckBrand();
    CheckInfo();
    CheckFeaturesAndIntegration();
    std::cout << "3 math CPU detection test cases passed\n";
}
