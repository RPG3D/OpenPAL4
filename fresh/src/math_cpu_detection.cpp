#include "pal4/math_cpu_detection.h"

#include <cstring>

namespace pal4 {

bool MathCpuDetection::GetRegisters(const std::uint32_t leaf,
                                    CpuIdRegisters& registers) {
    return provider_.Query(leaf, registers);
}

bool MathCpuDetection::GetBrand(char* destination, const std::uint32_t capacity) {
    if (destination == nullptr || capacity < 49u) return false;
    CpuIdRegisters maximum;
    if (!GetRegisters(0x80000000u, maximum) ||
        maximum.eax < 0x80000004u) return false;

    std::array<char, 49> bytes{};
    for (std::uint32_t index = 0; index < 3u; ++index) {
        CpuIdRegisters registers;
        if (!GetRegisters(0x80000002u + index, registers)) return false;
        const std::uint32_t words[] = {
            registers.eax, registers.ebx, registers.ecx, registers.edx};
        std::memcpy(bytes.data() + index * 16u, words, sizeof(words));
    }
    std::memcpy(destination, bytes.data(), bytes.size());
    return true;
}

char* MathCpuDetection::WriteInfo(const bool hyphens, char* destination,
                                  const std::uint32_t capacity) {
    const std::uint32_t needed = hyphens ? 30u : 25u;
    if (destination == nullptr || capacity < needed) return nullptr;
    CpuIdRegisters leaf1;
    CpuIdRegisters leaf3;
    if (!GetRegisters(1u, leaf1) || !GetRegisters(3u, leaf3)) return nullptr;

    constexpr char digits[] = "0123456789ABCDEF";
    const std::uint32_t words[] = {leaf1.eax, leaf3.edx, leaf3.ecx};
    char* output = destination;
    for (std::uint32_t index = 0; index < 3u; ++index) {
        const std::uint32_t word = words[index];
        for (int shift = 28; shift >= 0; shift -= 4) {
            *output++ = digits[(word >> shift) & 15u];
            if (hyphens && (shift == 16 || (shift == 0 && index < 2u)))
                *output++ = '-';
        }
    }
    *output = '\0';
    return output;
}

bool MathCpuDetection::HasLeaf1Bit(const std::uint32_t bit) {
    CpuIdRegisters registers;
    return GetRegisters(1u, registers) && ((registers.edx >> bit) & 1u) != 0u;
}

bool MathCpuDetection::HasMmx() { return HasLeaf1Bit(23u); }
bool MathCpuDetection::HasSse() { return HasLeaf1Bit(25u); }
bool MathCpuDetection::HasSse2() { return HasLeaf1Bit(26u); }

bool MathCpuDetection::Has3DNow() {
    CpuIdRegisters maximum;
    CpuIdRegisters features;
    return GetRegisters(0x80000000u, maximum) &&
           maximum.eax > 0x80000000u &&
           GetRegisters(0x80000001u, features) &&
           ((features.edx >> 31u) & 1u) != 0u;
}

void MathCpuInitializationRuntime::CpuBrand(char* output,
                                             const std::uint32_t capacity) {
    (void)detector_.GetBrand(output, capacity);
}
void MathCpuInitializationRuntime::CpuSerial(const std::uint32_t selector,
                                              char* output,
                                              const std::uint32_t capacity) {
    (void)detector_.WriteInfo(selector != 0u, output, capacity);
}
std::uint8_t MathCpuInitializationRuntime::HasMmx() {
    return detector_.HasMmx();
}
std::uint8_t MathCpuInitializationRuntime::HasSse() {
    return detector_.HasSse();
}
std::uint8_t MathCpuInitializationRuntime::HasSse2() {
    return detector_.HasSse2();
}
std::uint8_t MathCpuInitializationRuntime::Has3DNow() {
    return detector_.Has3DNow();
}

}  // namespace pal4
