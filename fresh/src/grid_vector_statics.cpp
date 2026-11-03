#include "pal4/grid_vector_statics.h"

#include <bit>
#include <cmath>
#include <cstdint>

namespace pal4 {

void GridVectorStatics::InitializeZeroThunk(GridVectorStaticsState& state) {
    InitializeZero(state);
}
void GridVectorStatics::InitializeZero(GridVectorStaticsState& state) {
    state.zero = {0.0f, 0.0f, 0.0f};
}
void GridVectorStatics::InitializePositiveYThunk(
    GridVectorStaticsState& state) {
    InitializePositiveY(state);
}
void GridVectorStatics::InitializePositiveY(
    GridVectorStaticsState& state) {
    state.positive_y = {0.0f, 1.0f, 0.0f};
}
void GridVectorStatics::InitializePositiveZThunk(
    GridVectorStaticsState& state) {
    InitializePositiveZ(state);
}
void GridVectorStatics::InitializePositiveZ(
    GridVectorStaticsState& state) {
    state.positive_z = {0.0f, 0.0f, 1.0f};
}
void GridVectorStatics::InitializeNegativeXThunk(
    GridVectorStaticsState& state) {
    InitializeNegativeX(state);
}
void GridVectorStatics::InitializeNegativeX(
    GridVectorStaticsState& state) {
    state.negative_x = {-1.0f, 0.0f, 0.0f};
}

GridVector3& GridVectorStatics::RandomInsideUnitSphere(
    GridVector3& destination, IGridRandomRuntime& runtime) {
    const float scale = std::bit_cast<float>(0x38000100u);
    GridVector3 candidate{};
    float distance{};
    const auto coordinate = [scale](std::int32_t sample) {
        // 65ECF3..65ED34: PC24 FMUL, doubling, then subtraction.
        const float fraction = static_cast<float>(sample) * scale;
        const float doubled = fraction + fraction;
        return doubled - 1.f;
    };
    do {
        const auto y_draw = runtime.Rand();
        const auto x_draw = runtime.Rand();
        const auto z_draw = runtime.Rand();
        candidate = {
            coordinate(x_draw),coordinate(y_draw),coordinate(z_draw)};
        // 65ED38..65ED56: preserve Y^2 + Z^2, then X^2 and PC24 FSQRT.
        const float y_squared = candidate[1] * candidate[1];
        const float z_squared = candidate[2] * candidate[2];
        const float yz_squared = y_squared + z_squared;
        const float x_squared = candidate[0] * candidate[0];
        const float squared = yz_squared + x_squared;
        distance = std::sqrt(squared);
    } while (distance >= 1.f);
    destination = candidate;
    return destination;
}

}  // namespace pal4
