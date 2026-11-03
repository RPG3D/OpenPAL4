#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

using GridVector3 = std::array<float, 3u>;

struct GridVectorStaticsState final {
    GridVector3 negative_x{}; // 0x93cf48
    GridVector3 positive_z{}; // 0x93cf54
    GridVector3 positive_y{}; // 0x93cf60
    GridVector3 zero{};       // 0x93cf6c
};
static_assert(sizeof(GridVectorStaticsState) == 48u);

class IGridRandomRuntime {
public:
    virtual ~IGridRandomRuntime() = default;
    [[nodiscard]] virtual std::int32_t Rand() = 0;
};

class GridVectorStatics final {
public:
    static void InitializeZeroThunk(GridVectorStaticsState& state);
    static void InitializeZero(GridVectorStaticsState& state);
    static void InitializePositiveYThunk(GridVectorStaticsState& state);
    static void InitializePositiveY(GridVectorStaticsState& state);
    static void InitializePositiveZThunk(GridVectorStaticsState& state);
    static void InitializePositiveZ(GridVectorStaticsState& state);
    static void InitializeNegativeXThunk(GridVectorStaticsState& state);
    static void InitializeNegativeX(GridVectorStaticsState& state);
    [[nodiscard]] static GridVector3& RandomInsideUnitSphere(
        GridVector3& destination, IGridRandomRuntime& runtime);
};

}  // namespace pal4
