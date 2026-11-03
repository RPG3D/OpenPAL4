#pragma once

#include "pal4/grid_vector_statics.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

#pragma pack(push, 1)
struct GridManagerState final {
    std::array<std::uint32_t, 42u> base_words{};
    std::uint32_t nested_grid = 0u;
    std::uint32_t owned_memory = 0u;
    std::uint32_t reserved_a = 0u;
    std::uint32_t reserved_b = 0u;
    std::uint8_t update_enabled = 0u;
    std::uint8_t reset_pending = 0u;
};
#pragma pack(pop)
static_assert(sizeof(GridManagerState) == 186u);
static_assert(offsetof(GridManagerState, nested_grid) == 168u);

class IGridManagerRuntime {
public:
    virtual ~IGridManagerRuntime() = default;
    [[nodiscard]] virtual std::array<std::uint32_t, 3u> TransformBasis(
        GridManagerState& state,
        const std::array<std::uint32_t, 3u>& source) = 0;
    virtual void SetParameter(GridManagerState& state,
                              std::uint32_t vtable_offset,
                              std::uint32_t value_bits) = 0;
    [[nodiscard]] virtual std::array<std::uint32_t, 3u> ZeroVector() = 0;
    [[nodiscard]] virtual std::uint32_t NextIdentifier() = 0;
    virtual void DestroyNested(std::uint32_t address, bool free_self) = 0;
    virtual void FreeSelf(GridManagerState& state) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class IGridManagerDirectionRuntime {
public:
    virtual ~IGridManagerDirectionRuntime() = default;
    [[nodiscard]] virtual float ReadParameter(
        const GridManagerState& state, std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual GridVector3 ReadNormal(
        const GridManagerState& state) = 0;
    [[nodiscard]] virtual GridVector3 ReadZeroVector() = 0;
};

class IGridManagerMotionRuntime {
public:
    virtual ~IGridManagerMotionRuntime() = default;
    [[nodiscard]] virtual GridVector3 CalculateAcceleration(
        GridManagerState& state, float input, float delta_time) = 0;
    [[nodiscard]] virtual float ReadScalar(
        GridManagerState& state, std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual GridVector3 ReadVelocity(
        GridManagerState& state) = 0;
    [[nodiscard]] virtual GridVector3 ReadPosition(
        GridManagerState& state) = 0;
    [[nodiscard]] virtual GridVector3 ReadNormal(
        GridManagerState& state) = 0;
    [[nodiscard]] virtual GridVector3 ReadOrientation(
        GridManagerState& state) = 0;
    virtual void SetSpeed(GridManagerState& state, float speed) = 0;
    virtual void SetPosition(GridManagerState& state,
                             const GridVector3& position) = 0;
    virtual void CommitVelocity(GridManagerState& state,
                                const GridVector3& velocity,
                                float delta_time) = 0;
};

class IGridManagerVelocityRuntime {
public:
    virtual ~IGridManagerVelocityRuntime() = default;
    [[nodiscard]] virtual float ReadSpeed(GridManagerState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ApplyDirection(
        GridManagerState& state, const GridVector3& direction) = 0;
};

class GridManager final {
public:
    [[nodiscard]] static GridManagerState& ConstructBase(
        GridManagerState& state, IGridManagerRuntime& runtime);
    static void SetBaseVtable(GridManagerState& state);
    [[nodiscard]] static GridManagerState& Construct(
        GridManagerState& state, IGridManagerRuntime& runtime);
    [[nodiscard]] static GridManagerState& Destroy(
        GridManagerState& state, bool free_self,
        IGridManagerRuntime& runtime);
    static void Cleanup(GridManagerState& state,
                        IGridManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t FreeMemory(
        GridManagerState& state, IGridManagerRuntime& runtime);
    [[nodiscard]] static GridVector3& ConstrainVelocity(
        const GridManagerState& state, GridVector3& destination,
        const GridVector3& velocity,
        IGridManagerDirectionRuntime& runtime);
    [[nodiscard]] static GridManagerState& UpdatePositionAndVelocity(
        GridManagerState& state, float input, float delta_time,
        IGridManagerMotionRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetVelocityDirection(
        GridManagerState& state, const GridVector3& velocity,
        IGridManagerVelocityRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdatePhysics(
        GridManagerState& state, float delta_time,
        IGridManagerMotionRuntime& runtime);
    [[nodiscard]] static GridVector3 CalculatePosition(
        GridManagerState& state, float delta_time,
        IGridManagerMotionRuntime& runtime);
    [[nodiscard]] static float ClampValue(float value, float lower,
                                          float upper);
    [[nodiscard]] static GridVector3 InterpolateVector(
        float factor, const GridVector3& first,
        const GridVector3& second);
};

}  // namespace pal4
