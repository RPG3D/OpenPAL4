#include "pal4/grid_vector_statics.h"
#include "support/grid_x87_reference.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IGridRandomRuntime {
public:
    std::int32_t Rand() override {
        Check(index < values.size(), "random sequence has enough draws");
        return values[index++];
    }
    std::vector<std::int32_t> values;
    std::size_t index = 0u;
};

}  // namespace

int main() {
#if defined(_MSC_VER) && defined(_M_IX86)
    const auto boundary=pal4::test::GridSpherePc24(13000,4506,27149);
    Check(!boundary.accepted && boundary.length==1.f,
          "original PC24 FSQRT rejects the captured near-unit candidate");
#endif
    Runtime near_unit;
    near_unit.values={13000,4506,27149,16384,10000,16384};
    pal4::GridVector3 near_result{};
    (void)pal4::GridVectorStatics::RandomInsideUnitSphere(near_result,near_unit);
    Check(near_unit.index==6u && std::bit_cast<std::uint32_t>(near_result[0])==0xbec77d90u,
          "PC24 unit-sphere boundary redraws and rounds accepted coordinates");
#if defined(_MSC_VER) && defined(_M_IX86)
    const auto accepted=pal4::test::GridSpherePc24(16384,10000,16384);
    Check(accepted.accepted && std::bit_cast<std::array<std::uint32_t,3>>(near_result)==
              std::bit_cast<std::array<std::uint32_t,3>>(accepted.coordinates),
          "accepted sphere coordinates match actual original x87 instructions");
    for(const auto sample:{0,1,33,8191,10000,15000,20000,24577,32766,32767}) {
        for(std::size_t axis=0;axis<3u;++axis) {
            std::array<std::int32_t,3> draws{16384,16384,16384};draws[axis]=sample;
            const auto first=pal4::test::GridSpherePc24(draws[0],draws[1],draws[2]);
            const auto wanted=first.accepted?first:
                pal4::test::GridSpherePc24(16384,16384,16384);
            Runtime values;values.values={draws[0],draws[1],draws[2],16384,16384,16384};
            pal4::GridVector3 point{};
            (void)pal4::GridVectorStatics::RandomInsideUnitSphere(point,values);
            Check(values.index==(first.accepted?3u:6u) &&
                      std::bit_cast<std::array<std::uint32_t,3>>(point)==
                      std::bit_cast<std::array<std::uint32_t,3>>(wanted.coordinates),
                  "sphere samples preserve x87 bits, axis order and rejection draws");
        }
    }
#endif
    pal4::GridVectorStaticsState state{};
    state.zero = {5.0f, 5.0f, 5.0f};
    state.positive_y = {5.0f, 5.0f, 5.0f};
    state.positive_z = {5.0f, 5.0f, 5.0f};
    state.negative_x = {5.0f, 5.0f, 5.0f};
    pal4::GridVectorStatics::InitializeZeroThunk(state);
    pal4::GridVectorStatics::InitializePositiveYThunk(state);
    pal4::GridVectorStatics::InitializePositiveZThunk(state);
    pal4::GridVectorStatics::InitializeNegativeXThunk(state);
    Check(state.zero == pal4::GridVector3{0.0f, 0.0f, 0.0f} &&
              state.positive_y == pal4::GridVector3{0.0f, 1.0f, 0.0f} &&
              state.positive_z == pal4::GridVector3{0.0f, 0.0f, 1.0f} &&
              state.negative_x == pal4::GridVector3{-1.0f, 0.0f, 0.0f},
          "four thunks initialize vectors at their IDA global addresses");
    state.zero = {1.0f, 1.0f, 1.0f};
    pal4::GridVectorStatics::InitializeZero(state);
    Check(state.zero == pal4::GridVector3{0.0f, 0.0f, 0.0f},
          "direct vector initializer matches thunk target");
    Runtime runtime;
    runtime.values = {0, 0, 0, 8192, 24576, 16384};
    pal4::GridVector3 result{};
    Check(&pal4::GridVectorStatics::RandomInsideUnitSphere(
              result, runtime) == &result &&
              runtime.index == 6u &&
              result[0] > 0.4f && result[0] < 0.6f &&
              result[1] < -0.4f && result[1] > -0.6f &&
              result[2] > -0.01f && result[2] < 0.01f,
          "rejection sampling consumes y, x, z draws then accepts inside point");
    std::cout << "Grid vector statics tests passed\n";
}
