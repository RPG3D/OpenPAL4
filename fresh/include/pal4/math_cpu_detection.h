#pragma once

#include <array>
#include <cstdint>

#include "pal4/math_dispatch_table.h"

namespace pal4 {

struct CpuIdRegisters final {
    std::uint32_t eax = 0;
    std::uint32_t ebx = 0;
    std::uint32_t ecx = 0;
    std::uint32_t edx = 0;
};

class ICpuIdProvider {
public:
    virtual ~ICpuIdProvider() = default;
    [[nodiscard]] virtual bool Query(std::uint32_t leaf,
                                     CpuIdRegisters& registers) = 0;
};

class MathCpuDetection final {
public:
    explicit MathCpuDetection(ICpuIdProvider& provider) : provider_(provider) {}

    [[nodiscard]] bool GetBrand(char* destination, std::uint32_t capacity);
    [[nodiscard]] char* WriteInfo(bool hyphens, char* destination,
                                  std::uint32_t capacity);
    [[nodiscard]] bool GetRegisters(std::uint32_t leaf,
                                    CpuIdRegisters& registers);
    [[nodiscard]] bool HasMmx();
    [[nodiscard]] bool HasSse();
    [[nodiscard]] bool HasSse2();
    [[nodiscard]] bool Has3DNow();

private:
    [[nodiscard]] bool HasLeaf1Bit(std::uint32_t bit);
    ICpuIdProvider& provider_;
};

class MathCpuInitializationRuntime final : public MathRuntimeInitializationStub {
public:
    explicit MathCpuInitializationRuntime(ICpuIdProvider& provider)
        : detector_(provider) {}
    void CpuBrand(char* output, std::uint32_t capacity) override;
    void CpuSerial(std::uint32_t selector, char* output,
                   std::uint32_t capacity) override;
    [[nodiscard]] std::uint8_t HasMmx() override;
    [[nodiscard]] std::uint8_t HasSse() override;
    [[nodiscard]] std::uint8_t HasSse2() override;
    [[nodiscard]] std::uint8_t Has3DNow() override;

private:
    MathCpuDetection detector_;
};

}  // namespace pal4
