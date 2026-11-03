#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace pal4 {

struct SpatialGridState final {
    std::array<std::uint32_t, 3u> origin_bits{};
    std::array<std::uint32_t, 3u> extent_bits{};
    std::array<std::uint32_t, 3u> dimensions{};
    std::uint32_t buckets = 0u;
    std::uint32_t overflow_head = 0u;
};
static_assert(sizeof(SpatialGridState) == 44u);
static_assert(offsetof(SpatialGridState, buckets) == 36u);
static_assert(offsetof(SpatialGridState, overflow_head) == 40u);

struct SpatialGridConfig final {
    std::array<float, 3u> origin{};
    std::array<float, 3u> extent{};
    std::array<std::uint32_t, 3u> dimensions{};
};

class ISpatialGridRuntime {
public:
    virtual ~ISpatialGridRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual SpatialGridState& GridAt(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

struct SpatialGridNodeState final {
    std::uint32_t previous = 0u;
    std::uint32_t next = 0u;
    std::uint32_t head_slot = 0u;
    std::uint32_t payload = 0u;
    std::array<std::uint32_t, 3u> position_bits{};
};
static_assert(sizeof(SpatialGridNodeState) == 28u);
static_assert(offsetof(SpatialGridNodeState, position_bits) == 16u);

class ISpatialGridNodeRuntime : public ISpatialGridRuntime {
public:
    [[nodiscard]] virtual SpatialGridNodeState& NodeAt(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
};

class SpatialGrid final {
public:
    using Visitor = std::function<std::uint32_t(
        std::uint32_t payload, std::uint32_t distance_bits,
        std::uint32_t context)>;
    // IDA 0x65d2d0/0x65d320: allocate grid and free buckets before header.
    [[nodiscard]] static std::uint32_t Create(
        const SpatialGridConfig& config, ISpatialGridRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t address, ISpatialGridRuntime& runtime);
    // IDA 0x65d340: write the 44-byte header and clear every bucket head.
    [[nodiscard]] static std::uint32_t Initialize(
        SpatialGridState& state, const SpatialGridConfig& config,
        ISpatialGridRuntime& runtime);
    // IDA 0x65d3c0: map a position to a bucket or +40 overflow head.
    [[nodiscard]] static std::uint32_t BucketAddress(
        const SpatialGridState& state, std::uint32_t grid_address,
        float x, float y, float z);
    // IDA 0x65d4b0..0x65d540: intrusive bucket-list node operations.
    [[nodiscard]] static SpatialGridNodeState& InitializeNode(
        SpatialGridNodeState& node, std::uint32_t payload);
    [[nodiscard]] static std::uint32_t InsertNode(
        std::uint32_t node_address, std::uint32_t head_slot,
        ISpatialGridNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RemoveNode(
        std::uint32_t node_address, ISpatialGridNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateNode(
        std::uint32_t grid_address, std::uint32_t node_address,
        float x, float y, float z, ISpatialGridNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessSphereCells(
        std::uint32_t grid_address, float x, float y, float z, float radius,
        Visitor visitor, std::uint32_t context,
        const std::array<int, 3u>& first, const std::array<int, 3u>& last,
        ISpatialGridNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessSphereOverflow(
        std::uint32_t grid_address, float x, float y, float z, float radius,
        Visitor visitor, std::uint32_t context,
        ISpatialGridNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessSphere(
        std::uint32_t grid_address, float x, float y, float z, float radius,
        Visitor visitor, std::uint32_t context,
        ISpatialGridNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t TraverseNodes(
        std::uint32_t first_node, Visitor visitor, std::uint32_t context,
        ISpatialGridNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessAllNodes(
        std::uint32_t grid_address, Visitor visitor, std::uint32_t context,
        ISpatialGridNodeRuntime& runtime);
};

}  // namespace pal4
