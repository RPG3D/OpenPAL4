#pragma once

#include "pal4/rtk_prt_std_atomic_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IRtkPrtStdAtomicTaskFacetRuntime {
public:
    virtual ~IRtkPrtStdAtomicTaskFacetRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint8_t UpdateFrame(std::uint32_t resource) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(double value) = 0;
};

class RtkPrtStdAtomicTaskFacetRuntimeStub final
    : public IRtkPrtStdAtomicTaskFacetRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    [[nodiscard]] std::uint8_t UpdateFrame(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::int32_t FloatToInt(double) override { return 0; }
};

class RtkPrtStdAtomicTaskFacets final {
public:
    // IDA 0x614a00, owner +40 facet.
    [[nodiscard]] static std::uint8_t SetPositionWords(
        const RtkPrtStdAtomicObjectState& state,
        const std::array<std::uint32_t, 3>& words,
        IRtkPrtStdAtomicTaskFacetRuntime& runtime);
    // IDA 0x614a50, owner +48 facet.
    [[nodiscard]] static std::uint8_t DisableResourceFlag(
        const RtkPrtStdAtomicObjectState& state,
        IRtkPrtStdAtomicTaskFacetRuntime& runtime);
    // IDA 0x614a70, owner +48 facet.
    [[nodiscard]] static std::uint32_t SetEmissionTime(
        const RtkPrtStdAtomicObjectState& state, float interval,
        std::uint32_t mode, std::uint32_t entry_eax,
        IRtkPrtStdAtomicTaskFacetRuntime& runtime);
    // IDA 0x614af0 / 0x614b20, owner +52 facet.
    [[nodiscard]] static std::uint8_t GetPositionWords(
        const RtkPrtStdAtomicObjectState& state,
        std::array<std::uint32_t, 3>& output,
        IRtkPrtStdAtomicTaskFacetRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetTargetWords(
        RtkPrtStdAtomicObjectState& state,
        const std::array<std::uint32_t, 3>& words);
    // IDA 0x614e00, owner +52 facet.
    [[nodiscard]] static std::uint8_t SetPosition(
        const RtkPrtStdAtomicObjectState& state,
        std::array<float, 3> values, bool relative,
        IRtkPrtStdAtomicTaskFacetRuntime& runtime);
};

}  // namespace pal4
