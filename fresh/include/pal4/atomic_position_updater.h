#pragma once

#include <array>
#include <cstdint>

#include "pal4/atomic_bounding_box_builder.h"

namespace pal4 {

using AtomicSphere = std::array<float, 4u>;
using AtomicVector3 = std::array<float, 3u>;

class IAtomicPositionRuntime {
public:
    virtual ~IAtomicPositionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void ResyncInterpolatedSphere(std::uint32_t atomic) = 0;
    [[nodiscard]] virtual bool UseDirectTransform() = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(
        std::uint32_t object) = 0;
    virtual void SetupTransformMatrix(
        AtomicMatrix& destination, std::uint32_t source) = 0;
    virtual void MultiplyMatrices(
        AtomicMatrix& destination, std::uint32_t object_transform,
        const AtomicMatrix& parent_transform) = 0;
    virtual void TransformSphere(AtomicSphere& sphere,
                                 const AtomicMatrix& transform) = 0;
    virtual void CalculatePosition(std::uint32_t destination,
                                   std::uint32_t source,
                                   std::uint32_t transform) = 0;
    [[nodiscard]] virtual bool IsInitialSphere() = 0;
    virtual void ClearInitialSphere() = 0;
    [[nodiscard]] virtual float Length(const AtomicVector3& vector) = 0;
};

class AtomicPositionRuntimeStub final : public IAtomicPositionRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void ResyncInterpolatedSphere(std::uint32_t) override {}
    [[nodiscard]] bool UseDirectTransform() override { return true; }
    [[nodiscard]] std::uint32_t GetObjectTransform(
        std::uint32_t) override { return 0u; }
    void SetupTransformMatrix(AtomicMatrix&, std::uint32_t) override {}
    void MultiplyMatrices(AtomicMatrix&, std::uint32_t,
                          const AtomicMatrix&) override {}
    void TransformSphere(AtomicSphere&, const AtomicMatrix&) override {}
    void CalculatePosition(std::uint32_t, std::uint32_t,
                           std::uint32_t) override {}
    [[nodiscard]] bool IsInitialSphere() override { return true; }
    void ClearInitialSphere() override {}
    [[nodiscard]] float Length(const AtomicVector3&) override { return 0.0f; }
};

class AtomicPositionUpdater final {
public:
    // IDA 0x5f9a40.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t atomic, std::uint32_t destination,
        IAtomicPositionRuntime& runtime);
    // IDA 0x667c60: mode and initial flag are callback-context fields.
    [[nodiscard]] static std::uint32_t ProcessBoundingSphere(
        std::uint32_t atomic, std::uint32_t mode,
        std::uint32_t& initial, std::uint32_t destination,
        IAtomicPositionRuntime& runtime);
    // IDA 0x5f9cf0.
    [[nodiscard]] static std::uint32_t UpdateCameraPosition(
        std::uint32_t record, std::uint32_t atomic,
        IAtomicPositionRuntime& runtime);
};

}  // namespace pal4
