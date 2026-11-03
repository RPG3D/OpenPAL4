#include "pal4/pathway_grid.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IPathwayGridRuntime {
public:
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        allocations.push_back(bytes);
        if (allocations.size() == failure_index)
            return 0u;
        const auto result = next_address;
        next_address += 0x100u;
        return result;
    }
    void Free(const std::uint32_t address) override {
        frees.push_back(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        memory[address] = value;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return memory.at(address);
    }
    std::uint32_t next_address = 0x1000u;
    std::size_t failure_index = 0u;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::map<std::uint32_t, std::uint32_t> memory;
};

float FloatAt(Runtime& runtime, const std::uint32_t address) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

void ConstructionAndOpenPath() {
    Runtime runtime;
    pal4::PathwayGridState state{};
    state.total_length_bits = 0xabcdefu;
    Check(&pal4::PathwayGrid::Construct(state, true) == &state &&
              state.primary_vtable == 0x84b5e4u &&
              state.vbtable == 0x84b5f8u &&
              state.secondary_vtable == 0x84b5d4u &&
              state.offset_to_top == 0u &&
              state.total_length_bits == 0xabcdefu,
          "constructor installs verified virtual tables and clears owned pointers");
    constexpr std::array points{
        pal4::PathwayPoint{{0.0f, 0.0f, 0.0f}},
        pal4::PathwayPoint{{3.0f, 4.0f, 0.0f}},
        pal4::PathwayPoint{{3.0f, 4.0f, 12.0f}}};
    Check(pal4::PathwayGrid::Initialize(state, points, 7u, false, runtime),
          "open pathway initialization succeeds");
    Check(state.point_count == 3u && state.kind == 7u &&
              (state.loop_word & 0xffu) == 0u &&
              state.lengths == 0x1000u && state.positions == 0x1100u &&
              state.directions == 0x1200u &&
              runtime.allocations ==
                  std::vector<std::uint32_t>{12u, 36u, 36u},
          "three owned arrays have IDA allocation sizes");
    Check(FloatAt(runtime, state.positions + 12u) == 3.0f &&
              FloatAt(runtime, state.positions + 16u) == 4.0f &&
              FloatAt(runtime, state.lengths) == 0.0f &&
              FloatAt(runtime, state.lengths + 4u) == 5.0f &&
              FloatAt(runtime, state.lengths + 8u) == 12.0f &&
              FloatAt(runtime, state.directions + 12u) == 0.6f &&
              FloatAt(runtime, state.directions + 16u) == 0.8f &&
              FloatAt(runtime, state.directions + 32u) == 1.0f &&
              std::bit_cast<float>(state.total_length_bits) == 17.0f,
          "positions, segment lengths, unit directions and total length");
    Check(pal4::PathwayGrid::Destroy(state, runtime) == 0x1200u &&
              runtime.frees ==
                  std::vector<std::uint32_t>{0x1000u, 0x1100u, 0x1200u} &&
              state.lengths == 0u && state.positions == 0u &&
              state.directions == 0u,
          "destructor frees lengths, positions, directions in IDA order");
}

void LoopAndReplacement() {
    Runtime runtime;
    pal4::PathwayGridState state{};
    (void)pal4::PathwayGrid::Construct(state, false);
    constexpr std::array points{
        pal4::PathwayPoint{{0.0f, 0.0f, 0.0f}},
        pal4::PathwayPoint{{0.0f, 3.0f, 0.0f}}};
    Check(pal4::PathwayGrid::Initialize(state, points, 2u, true, runtime) &&
              state.point_count == 3u &&
              (state.loop_word & 0xffu) == 1u &&
              FloatAt(runtime, state.positions + 24u) == 0.0f &&
              FloatAt(runtime, state.positions + 28u) == 0.0f &&
              FloatAt(runtime, state.lengths + 8u) == 3.0f &&
              FloatAt(runtime, state.directions + 28u) == -1.0f &&
              std::bit_cast<float>(state.total_length_bits) == 6.0f,
          "loop appends the first point and closing segment");
    Check(pal4::PathwayGrid::Initialize(state, points, 3u, false, runtime) &&
              runtime.frees ==
                  std::vector<std::uint32_t>{0x1000u, 0x1100u, 0x1200u} &&
              state.point_count == 2u && state.kind == 3u,
          "reinitialization releases prior buffers before replacement");
}

void AllocationFailure() {
    Runtime runtime;
    runtime.failure_index = 2u;
    pal4::PathwayGridState state{};
    (void)pal4::PathwayGrid::Construct(state, true);
    constexpr std::array points{pal4::PathwayPoint{{1.0f, 2.0f, 3.0f}}};
    Check(!pal4::PathwayGrid::Initialize(state, points, 0u, false,
                                          runtime) &&
              runtime.frees ==
                  std::vector<std::uint32_t>{0x1000u, 0x1100u} &&
              state.lengths == 0u && state.positions == 0u &&
              state.directions == 0u,
          "partial allocation is safely released");
}

void QueryPath() {
    Runtime runtime;
    pal4::PathwayGridState state{};
    (void)pal4::PathwayGrid::Construct(state, true);
    constexpr std::array points{
        pal4::PathwayPoint{{0.0f, 0.0f, 0.0f}},
        pal4::PathwayPoint{{5.0f, 0.0f, 0.0f}},
        pal4::PathwayPoint{{5.0f, 5.0f, 0.0f}}};
    Check(pal4::PathwayGrid::Initialize(state, points, 0u, false, runtime),
          "query path initialized");
    Check(pal4::PathwayGrid::FindClosestPosition(
              state, pal4::PathwayPoint{{2.0f, 2.0f, 0.0f}},
              runtime) == 2.0f &&
              std::bit_cast<float>(state.projection_bits) == 2.0f &&
              std::bit_cast<float>(state.nearest_bits[0]) == 5.0f,
          "closest path position selects the first segment on equal distance");
    Check(pal4::PathwayGrid::FindClosestPosition(
              state, pal4::PathwayPoint{{7.0f, 3.0f, 0.0f}},
              runtime) == 8.0f &&
              std::bit_cast<float>(state.nearest_bits[1]) == 3.0f,
          "closest path position includes prior segment lengths");
    state.segment_length_bits = std::bit_cast<std::uint32_t>(5.0f);
    state.direction_bits = {std::bit_cast<std::uint32_t>(1.0f), 0u, 0u};
    Check(pal4::PathwayGrid::DistanceToSegment(
              state, pal4::PathwayPoint{{-2.0f, 1.0f, 0.0f}},
              points[0], points[1]) > 2.0f &&
              state.projection_bits == 0u &&
              state.nearest_bits[0] == 0u,
          "negative projection clamps to first endpoint");
    Check(pal4::PathwayGrid::DistanceToSegment(
              state, pal4::PathwayPoint{{8.0f, 1.0f, 0.0f}},
              points[0], points[1]) > 3.0f &&
              std::bit_cast<float>(state.projection_bits) == 5.0f &&
              std::bit_cast<float>(state.nearest_bits[0]) == 5.0f,
          "projection beyond segment clamps to last endpoint");
    Check(pal4::PathwayGrid::CoordinatesAt(state, -1.0f, runtime)
                  .coordinates == points[0].coordinates &&
              pal4::PathwayGrid::CoordinatesAt(state, 10.0f, runtime)
                  .coordinates == points[2].coordinates,
          "coordinate query clamps both path ends");
    Check(pal4::PathwayGrid::CoordinatesAt(state, 2.5f, runtime)
                  .coordinates ==
              std::array<float, 3u>{2.5f, 0.0f, 0.0f} &&
              pal4::PathwayGrid::CoordinatesAt(state, 7.5f, runtime)
                  .coordinates ==
              std::array<float, 3u>{5.0f, 2.5f, 0.0f},
          "coordinate query interpolates within each segment");
}

}  // namespace

int main() {
    ConstructionAndOpenPath();
    LoopAndReplacement();
    AllocationFailure();
    QueryPath();
    {
        Runtime runtime;
        pal4::PathwayGridState state{};
        (void)pal4::PathwayGrid::Construct(state, true);
        constexpr std::array points{
            pal4::PathwayPoint{{0.f,0.f,0.f}},
            pal4::PathwayPoint{{10.f,0.f,0.f}},
            pal4::PathwayPoint{{10.f,0.f,10.f}}};
        Check(pal4::PathwayGrid::Initialize(state,points,
            std::bit_cast<std::uint32_t>(1.f),false,runtime),"tube initialized");
        pal4::PathwayPoint direction{{9.f,8.f,7.f}};
        float outside=0.f;
        auto nearest=pal4::PathwayGrid::ProjectToTube(state,{{8.f,0.f,2.f}},direction,outside,runtime);
        Check(nearest.coordinates==std::array<float,3>{8.f,0.f,0.f} &&
              direction.coordinates==std::array<float,3>{1.f,0.f,0.f} && outside==1.f,
              "equidistant corner keeps earlier segment, tangent and radius bits");
        nearest=pal4::PathwayGrid::ProjectToTube(state,{{9.5f,0.f,6.f}},direction,outside,runtime);
        Check(nearest.coordinates==std::array<float,3>{10.f,0.f,6.f} &&
              direction.coordinates==std::array<float,3>{0.f,0.f,1.f} && outside==-0.5f,
              "nearest later segment and negative inside distance");
        nearest=pal4::PathwayGrid::ProjectToTube(state,{{-3.f,0.f,0.f}},direction,outside,runtime);
        Check(nearest.coordinates==points[0].coordinates && outside==2.f,
              "tube endpoint distance uses clamped projection");
        (void)pal4::PathwayGrid::Destroy(state,runtime);
        state.point_count=0u;
        direction={{9.f,8.f,7.f}};
        nearest=pal4::PathwayGrid::ProjectToTube(state,{{0.f,3.f,4.f}},direction,outside,runtime);
        Check(nearest.coordinates==std::array<float,3>{} && outside==4.f &&
              direction.coordinates==std::array<float,3>{9.f,8.f,7.f},
              "no winning segment preserves tangent and returns original zero-nearest distance");
    }
    std::cout << "Pathway grid tests passed\n";
}
