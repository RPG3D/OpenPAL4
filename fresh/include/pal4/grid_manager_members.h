#pragma once

#include "pal4/spatial_grid.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace pal4 {

#pragma pack(push, 1)
struct GridManagerMembersState final {
    std::uint8_t first_flag = 0u;
    std::uint8_t first_padding[3]{};
    std::uint32_t elements_begin = 0u;
    std::uint32_t elements_end = 0u;
    std::uint32_t elements_capacity = 0u;
    std::uint8_t second_flag = 0u;
    std::uint8_t second_padding[3]{};
    std::uint32_t secondary_begin = 0u;
    std::uint32_t secondary_end = 0u;
    std::uint32_t secondary_capacity = 0u;
    std::uint32_t grid_wrapper = 0u;
    std::uint8_t grid_ready = 0u;
    std::uint8_t update_disabled = 0u;
};
#pragma pack(pop)
static_assert(sizeof(GridManagerMembersState) == 38u);
static_assert(offsetof(GridManagerMembersState, second_flag) == 16u);
static_assert(offsetof(GridManagerMembersState, secondary_begin) == 20u);
static_assert(offsetof(GridManagerMembersState, secondary_end) == 24u);
static_assert(offsetof(GridManagerMembersState, secondary_capacity) == 28u);
static_assert(offsetof(GridManagerMembersState, grid_wrapper) == 32u);

class IGridManagerMembersRuntime : public ISpatialGridRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void DestroyGridWrapper(std::uint32_t address) = 0;
    virtual void WriteGridWrapper(std::uint32_t wrapper_address,
                                  std::uint32_t grid_address) = 0;
    virtual void ReleaseElement(std::uint32_t element) = 0;
    virtual void UpdateElement(std::uint32_t element,
                               GridManagerMembersState& state,
                               std::uint32_t argument) = 0;
};

class GridManagerMembers final {
public:
    // Copies the actual secondary vector in consumer order. Tokens refer to
    // caller-owned records; resolve/copy them before releasing the scene.
    // This observer does not populate the vector or query moving neighbors.
    [[nodiscard]] static std::vector<std::uint32_t> SecondaryElements(
        const GridManagerMembersState& state,
        IGridManagerMembersRuntime& runtime);
    [[nodiscard]] static GridManagerMembersState& Initialize(
        GridManagerMembersState& state);
    [[nodiscard]] static std::uint32_t Cleanup(
        GridManagerMembersState& state,
        IGridManagerMembersRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeGrid(
        GridManagerMembersState& state,
        IGridManagerMembersRuntime& runtime);
    static void ResetGrid(GridManagerMembersState& state,
                          IGridManagerMembersRuntime& runtime);
    [[nodiscard]] static bool AddElement(
        GridManagerMembersState& state, std::uint32_t element,
        IGridManagerMembersRuntime& runtime);
    [[nodiscard]] static std::uint32_t AppendVisitedElement(
        GridManagerMembersState& state, std::uint32_t element,
        std::uint32_t distance_bits,
        IGridManagerMembersRuntime& runtime);
    [[nodiscard]] static bool RemoveElement(
        GridManagerMembersState& state, std::uint32_t element,
        IGridManagerMembersRuntime& runtime);
    [[nodiscard]] static std::uint32_t ClearElements(
        GridManagerMembersState& state,
        IGridManagerMembersRuntime& runtime);
    // IDA65E7D0 (GridManager_ClearSecondaryElements): returns the old end,
    // then resets the secondary live range, retaining its storage/capacity.
    [[nodiscard]] static std::uint32_t PreviousElement(
        GridManagerMembersState& state);
    static void EnableUpdate(GridManagerMembersState& state);
    static void DisableUpdate(GridManagerMembersState& state);
    [[nodiscard]] static std::uint8_t UpdateElements(
        GridManagerMembersState& state, std::uint32_t argument,
        IGridManagerMembersRuntime& runtime);
};

}  // namespace pal4
