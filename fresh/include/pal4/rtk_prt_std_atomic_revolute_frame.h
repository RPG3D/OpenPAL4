#pragma once

#include "pal4/rtk_prt_std_atomic_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IRtkPrtStdAtomicRevoluteRuntime {
public:
    virtual ~IRtkPrtStdAtomicRevoluteRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetFrame(
        RtkPrtStdAtomicObjectState& state) = 0;
    virtual void TranslateMatrix(std::uint32_t matrix,
                                 const std::array<float, 3>& vector,
                                 std::uint32_t mode) = 0;
    virtual void RotateMatrix(std::uint32_t matrix,
                              const std::array<float, 3>& axis,
                              float angle, std::uint32_t mode) = 0;
    virtual void UpdateFrameObjects(std::uint32_t frame) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SetPosition(
        RtkPrtStdAtomicObjectState& state,
        const std::array<float, 3>& position,
        std::uint32_t mode) = 0;
};

class RtkPrtStdAtomicRevoluteRuntimeStub final
    : public IRtkPrtStdAtomicRevoluteRuntime {
public:
    [[nodiscard]] std::uint32_t GetFrame(
        RtkPrtStdAtomicObjectState&) override { return 0u; }
    void TranslateMatrix(std::uint32_t, const std::array<float, 3>&,
                         std::uint32_t) override {}
    void RotateMatrix(std::uint32_t, const std::array<float, 3>&,
                      float, std::uint32_t) override {}
    void UpdateFrameObjects(std::uint32_t) override {}
    [[nodiscard]] float ReadFloat(std::uint32_t) override { return 0.0f; }
    [[nodiscard]] std::uint32_t SetPosition(
        RtkPrtStdAtomicObjectState&, const std::array<float, 3>&,
        std::uint32_t) override { return 0u; }
};

class RtkPrtStdAtomicRevoluteFrame final {
public:
    // IDA 0x6147b0; owner +36 facet, dispatches to owner +24 facet.
    [[nodiscard]] static std::uint32_t Apply(
        RtkPrtStdAtomicObjectState& state,
        const std::array<float, 3>& target,
        const std::array<float, 3>& rotation,
        const std::array<float, 3>& step,
        std::uint32_t entry_eax,
        IRtkPrtStdAtomicRevoluteRuntime& runtime);
};

}  // namespace pal4
