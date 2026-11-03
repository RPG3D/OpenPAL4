#pragma once

#include "pal4/clump_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

enum class ClumpUvFacetKind : std::uint8_t {
    Rotation,
    Translation
};

class IClumpObjectUvRuntime {
public:
    virtual ~IClumpObjectUvRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CreateMatrix() = 0;
    [[nodiscard]] virtual std::uint8_t DestroyMatrix(
        std::uint32_t matrix) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void ApplyToAtomics(std::uint32_t clump,
                                bool second_uv, std::uint32_t matrix) = 0;
    [[nodiscard]] virtual std::uint32_t RotateMatrix(
        std::uint32_t matrix, const std::array<float, 3>& axis,
        float angle, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint8_t TranslateMatrix(
        std::uint32_t matrix, const std::array<float, 3>& vector,
        std::uint32_t mode) = 0;
};

class ClumpObjectUvRuntimeStub final : public IClumpObjectUvRuntime {
public:
    [[nodiscard]] std::uint32_t CreateMatrix() override { return 0u; }
    [[nodiscard]] std::uint8_t DestroyMatrix(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override {
        return 0u;
    }
    void ApplyToAtomics(std::uint32_t, bool, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t RotateMatrix(
        std::uint32_t, const std::array<float, 3>&,
        float, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t TranslateMatrix(
        std::uint32_t, const std::array<float, 3>&,
        std::uint32_t) override { return 0u; }
};

class ClumpObjectUvFacet final {
public:
    // IDA 0x60b2d0/0x60b3c0.
    [[nodiscard]] static std::uint8_t Acquire(
        ClumpObjectState& state, ClumpUvFacetKind kind,
        std::uint32_t index, IClumpObjectUvRuntime& runtime);
    // IDA 0x60b330/0x60b420.
    [[nodiscard]] static std::uint8_t Release(
        ClumpObjectState& state, ClumpUvFacetKind kind,
        std::uint32_t index, IClumpObjectUvRuntime& runtime);
    // IDA 0x60b370.
    [[nodiscard]] static std::uint32_t Rotate(
        const ClumpObjectState& state, std::uint32_t index,
        const std::array<float, 3>& axis, float angle,
        IClumpObjectUvRuntime& runtime);
    // IDA 0x60b460.
    [[nodiscard]] static std::uint8_t Translate(
        const ClumpObjectState& state, std::uint32_t index,
        const std::array<float, 3>& vector,
        IClumpObjectUvRuntime& runtime);
};

}  // namespace pal4
