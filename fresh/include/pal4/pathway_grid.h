#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace pal4 {

struct PathwayPoint final {
    std::array<float, 3u> coordinates{};
};
static_assert(sizeof(PathwayPoint) == 12u);

struct PathwayGridState final {
    std::uint32_t primary_vtable = 0u;
    std::uint32_t vbtable = 0u;
    std::uint32_t point_count = 0u;
    std::uint32_t positions = 0u;
    std::uint32_t kind = 0u;
    std::uint32_t loop_word = 0u;
    std::uint32_t segment_length_bits = 0u;
    std::uint32_t projection_bits = 0u;
    std::array<std::uint32_t, 3u> difference_bits{};
    std::array<std::uint32_t, 3u> nearest_bits{};
    std::array<std::uint32_t, 3u> direction_bits{};
    std::uint32_t lengths = 0u;
    std::uint32_t directions = 0u;
    std::uint32_t total_length_bits = 0u;
    std::uint32_t offset_to_top = 0u;
    std::uint32_t secondary_vtable = 0u;
};
static_assert(sizeof(PathwayGridState) == 88u);
static_assert(offsetof(PathwayGridState, lengths) == 68u);
static_assert(offsetof(PathwayGridState, directions) == 72u);
static_assert(offsetof(PathwayGridState, secondary_vtable) == 84u);

class IPathwayGridRuntime {
public:
    virtual ~IPathwayGridRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class PathwayGrid final {
public:
    [[nodiscard]] static PathwayGridState& Construct(
        PathwayGridState& state, bool initialize_virtual_base);
    [[nodiscard]] static std::uint32_t Destroy(
        PathwayGridState& state, IPathwayGridRuntime& runtime);
    [[nodiscard]] static bool Initialize(
        PathwayGridState& state, std::span<const PathwayPoint> points,
        std::uint32_t kind, bool loop, IPathwayGridRuntime& runtime);
    [[nodiscard]] static float FindClosestPosition(
        PathwayGridState& state, const PathwayPoint& point,
        IPathwayGridRuntime& runtime);
    [[nodiscard]] static PathwayPoint CoordinatesAt(
        PathwayGridState& state, float position,
        IPathwayGridRuntime& runtime);
    // IDA65DD50, secondary interface slot0. The +10 field stores radius bits.
    // Direction is unchanged when no segment wins the ordered distance test.
    [[nodiscard]] static PathwayPoint ProjectToTube(
        PathwayGridState& state, const PathwayPoint& point,
        PathwayPoint& direction, float& outside,
        IPathwayGridRuntime& runtime);
    [[nodiscard]] static float DistanceToSegment(
        PathwayGridState& state, const PathwayPoint& point,
        const PathwayPoint& first, const PathwayPoint& last);
};

}  // namespace pal4
