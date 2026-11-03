#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ClumpBoundsVector final {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct ClumpBoundsState final {
    ClumpBoundsVector maximum;
    ClumpBoundsVector minimum;
    std::array<float, 16> matrix{};
    ClumpBoundsVector half_extent;
    ClumpBoundsVector center;
    ClumpBoundsVector transformed_center;
};

static_assert(sizeof(ClumpBoundsState) == 124);
static_assert(offsetof(ClumpBoundsState, matrix) == 24);
static_assert(offsetof(ClumpBoundsState, half_extent) == 88);
static_assert(offsetof(ClumpBoundsState, center) == 100);
static_assert(offsetof(ClumpBoundsState, transformed_center) == 112);

struct ClumpBoundsVertex final {
    ClumpBoundsVector position;
    std::array<std::byte, 12> padding{};
    std::uint32_t color = 0;
    std::array<std::byte, 8> trailing{};
};

static_assert(sizeof(ClumpBoundsVertex) == 36);
static_assert(offsetof(ClumpBoundsVertex, color) == 24);

class IClumpBoundsRenderRuntime {
public:
    virtual ~IClumpBoundsRenderRuntime() = default;
    [[nodiscard]] virtual int SetRenderState(int state, int value) = 0;
    [[nodiscard]] virtual bool TransformImmediate(
        const std::array<ClumpBoundsVertex, 8>& vertices,
        std::uint32_t matrix, int flags) = 0;
    virtual void RenderIndexedLines(int primitive, int index_count) = 0;
    virtual void EndImmediate() = 0;
};

class IClumpSpatialBoundsRuntime : public IClumpBoundsRenderRuntime {
public:
    virtual ~IClumpSpatialBoundsRuntime() = default;
    virtual void SetupMatrix(std::array<float, 16>& output,
                             std::uint32_t source) = 0;
    virtual void MultiplyMatrix(std::array<float, 16>& output,
                                std::uint32_t lhs,
                                const std::array<float, 16>& rhs) = 0;
    [[nodiscard]] virtual std::uint32_t AllocatePoints(
        std::uint32_t bytes, std::uint32_t hint) = 0;
    virtual void TransformPoints(std::uint32_t destination,
                                 std::uint32_t source, std::uint32_t count,
                                 const std::array<float, 16>& matrix) = 0;
    virtual void CalculateBox(ClumpBoundsVector& maximum,
                              ClumpBoundsVector& minimum,
                              std::uint32_t points, std::uint32_t count) = 0;
    virtual void TransformVector(ClumpBoundsVector& destination,
                                 const ClumpBoundsVector& source,
                                 const std::array<float, 16>& matrix) = 0;
    [[nodiscard]] virtual std::uint32_t FreePoints(std::uint32_t points) = 0;
    virtual void CalculateCameraPosition(
        ClumpBoundsVector& output, std::uint32_t camera,
        const std::array<float, 16>& matrix) = 0;

};

class ClumpSpatialBounds final {
public:
    // IDA 0x5fac90.
    [[nodiscard]] static std::uint32_t CalculateBoundingBox(
        ClumpBoundsState& state, std::uint32_t vertices,
        std::uint32_t count, std::uint32_t matrix,
        std::uint32_t setup, IClumpSpatialBoundsRuntime& runtime);
    // IDA 0x5fad70.
    [[nodiscard]] static bool CheckIntersection(
        const ClumpBoundsState& state, std::uint32_t camera,
        ClumpBoundsVector* closest,
        IClumpSpatialBoundsRuntime& runtime);
    // IDA 0x5fade0.
    [[nodiscard]] static double GetIntersectionDistance(
        const ClumpBoundsState& state, const ClumpBoundsVector& point,
        ClumpBoundsVector* closest);
    // IDA 0x5fb010.
    [[nodiscard]] static int RenderBoundingBox(
        const ClumpBoundsState& state, std::uint32_t color,
        std::uint32_t matrix, IClumpBoundsRenderRuntime& runtime);
};

}  // namespace pal4
