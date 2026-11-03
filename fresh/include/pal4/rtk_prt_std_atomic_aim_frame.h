#pragma once

#include "pal4/rtk_prt_std_atomic_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct RtkPrtStdAtomicAimMatrix final {
    std::array<float, 3> right{};
    std::uint32_t flags = 0x20003u;
    std::array<float, 3> up{};
    std::uint32_t pad1 = 0u;
    std::array<float, 3> at{};
    std::uint32_t pad2 = 0u;
    std::array<float, 3> position{};
    std::uint32_t pad3 = 0u;
};
static_assert(sizeof(RtkPrtStdAtomicAimMatrix) == 64u);

class IRtkPrtStdAtomicAimRuntime {
public:
    virtual ~IRtkPrtStdAtomicAimRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual double Normalize(std::array<float, 3>& vector) = 0;
    virtual void UpdateMatrix(RtkPrtStdAtomicAimMatrix& matrix) = 0;
    [[nodiscard]] virtual std::uint32_t GetFrame(
        const RtkPrtStdAtomicObjectState& state) = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void ScaleFrame(std::uint32_t frame,
                            const std::array<std::uint32_t, 3>& scale,
                            std::uint32_t mode) = 0;
    virtual void ApplyMatrix(std::uint32_t frame,
                             const RtkPrtStdAtomicAimMatrix& matrix,
                             std::uint32_t mode) = 0;
    virtual void TranslateFrame(std::uint32_t frame,
                                const std::array<std::uint32_t, 3>& position,
                                std::uint32_t mode) = 0;
    virtual void UpdateFrameRotation(
        double normalization_result, std::uint32_t frame,
        const std::array<std::uint32_t, 3>& offset,
        std::array<float, 3>& output) = 0;
    [[nodiscard]] virtual std::uint8_t SetRotation(
        RtkPrtStdAtomicObjectState& state,
        const std::array<float, 3>& angles,
        std::uint32_t mode) = 0;
};

class RtkPrtStdAtomicAimRuntimeStub final : public IRtkPrtStdAtomicAimRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] double Normalize(std::array<float, 3>&) override { return 0.0; }
    void UpdateMatrix(RtkPrtStdAtomicAimMatrix&) override {}
    [[nodiscard]] std::uint32_t GetFrame(
        const RtkPrtStdAtomicObjectState&) override { return 0u; }
    void SetFrameIdentity(std::uint32_t) override {}
    void ScaleFrame(std::uint32_t,
                    const std::array<std::uint32_t, 3>&,
                    std::uint32_t) override {}
    void ApplyMatrix(std::uint32_t,
                     const RtkPrtStdAtomicAimMatrix&,
                     std::uint32_t) override {}
    void TranslateFrame(std::uint32_t,
                        const std::array<std::uint32_t, 3>&,
                        std::uint32_t) override {}
    void UpdateFrameRotation(double, std::uint32_t,
        const std::array<std::uint32_t, 3>&,
        std::array<float, 3>&) override {}
    [[nodiscard]] std::uint8_t SetRotation(
        RtkPrtStdAtomicObjectState&,
        const std::array<float, 3>&,
        std::uint32_t) override { return 0u; }
};

class RtkPrtStdAtomicAimFrame final {
public:
    // IDA 0x614b60, owner +52 facet.
    [[nodiscard]] static std::uint8_t Update(
        RtkPrtStdAtomicObjectState& state, std::uint8_t enabled,
        IRtkPrtStdAtomicAimRuntime& runtime);
};

}  // namespace pal4
