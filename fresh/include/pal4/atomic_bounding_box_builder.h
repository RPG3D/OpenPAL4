#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IAtomicPositionRuntime;

using AtomicMatrix = std::array<std::uint32_t, 16u>;
using AtomicBoxCorners = std::array<float, 24u>;

class IAtomicBoundingBoxRuntime {
public:
    virtual ~IAtomicBoundingBoxRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t size, std::uint32_t flags) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool UseDirectTransform() = 0;
    [[nodiscard]] virtual bool UseSecondaryDirectTransform() = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(
        std::uint32_t object) = 0;
    virtual void SetupTransformMatrix(
        AtomicMatrix& destination, std::uint32_t source) = 0;
    virtual void MultiplyMatrices(
        AtomicMatrix& destination, std::uint32_t object_transform,
        const AtomicMatrix& parent_transform) = 0;
    virtual void TransformPoints(
        std::uint32_t destination, std::uint32_t source,
        std::int32_t count, const AtomicMatrix& transform) = 0;
    virtual void AddPoint(std::uint32_t box, std::uint32_t point) = 0;
};

class IAtomicBoxTransformRuntime {
public:
    virtual ~IAtomicBoxTransformRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(
        std::uint32_t object) = 0;
    virtual void TransformCorners(AtomicBoxCorners& corners,
                                  std::uint32_t transform) = 0;
    [[nodiscard]] virtual std::uint32_t CalculateBox(
        std::uint32_t destination, const AtomicBoxCorners& corners) = 0;
};

class IAtomicBoundsInitRuntime {
public:
    virtual ~IAtomicBoundsInitRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ForEachAtomicVertexBounds(
        std::uint32_t clump, std::uint32_t box) = 0;
    [[nodiscard]] virtual std::uint32_t ForEachAtomicPosition(
        std::uint32_t clump, std::uint32_t sphere) = 0;
};

class AtomicBoundsInitRuntimeStub final : public IAtomicBoundsInitRuntime {
public:
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ForEachAtomicVertexBounds(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ForEachAtomicPosition(
        std::uint32_t, std::uint32_t) override { return 0u; }
};

class AtomicBoundingBoxBuilder final {
public:
    // IDA 0x5f9950, formerly named TGA_GetBoundingBox.
    [[nodiscard]] static std::uint32_t AddTransformedVertices(
        std::uint32_t atomic, std::uint32_t box,
        IAtomicBoundingBoxRuntime& runtime);
    // IDA 0x5fa200: same vertex path with direct flag 0x92f560.
    [[nodiscard]] static std::uint32_t AddTransformedVerticesSecondary(
        std::uint32_t atomic, std::uint32_t box,
        IAtomicBoundingBoxRuntime& runtime);
    // IDA 0x667b70: mode and destination are the two callback-context words.
    [[nodiscard]] static std::uint32_t ProcessBoundingBox(
        std::uint32_t atomic, std::uint32_t mode, std::uint32_t box,
        IAtomicBoundingBoxRuntime& runtime);
    // IDA 0x5f9c60.
    [[nodiscard]] static std::uint32_t UpdateTransformedBox(
        std::uint32_t box, std::uint32_t atomic,
        IAtomicBoxTransformRuntime& runtime);
    // IDA 0x5f9d20.
    [[nodiscard]] static std::uint32_t InitializeClumpBounds(
        std::uint32_t record, std::uint32_t clump,
        IAtomicBoundsInitRuntime& runtime);
    // IDA 0x5f9d70.
    [[nodiscard]] static std::uint32_t UpdateBoxAndCamera(
        std::uint32_t record, std::uint32_t atomic,
        IAtomicBoxTransformRuntime& box_runtime,
        IAtomicPositionRuntime& position_runtime);
};

}  // namespace pal4
