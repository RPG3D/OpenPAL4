#include "pal4/pathway_grid.h"
#include "support/random_movement_x87_reference.h"

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

void CapturedM15PathDirections() {
    Runtime runtime;pal4::PathwayGridState state{};
    (void)pal4::PathwayGrid::Construct(state,true);
    constexpr std::array points{
        pal4::PathwayPoint{{6218.28466796875f,1572.0003662109375f,-121.49848175048828f}},
        pal4::PathwayPoint{{5782.4052734375f,1476.740966796875f,75.1475601196289f}},
        pal4::PathwayPoint{{5209.92822265625f,1328.1846923828125f,694.409423828125f}},
        pal4::PathwayPoint{{5041.6552734375f,1374.58837890625f,1203.50244140625f}}};
    Check(pal4::PathwayGrid::Initialize(state,points,0u,false,runtime),
          "captured M15 helper7 path initializes");
    constexpr std::array<std::uint32_t,12> directions{0,0,0,
        0xbf64daceu,0xbe480f7cu,0x3ece7e9eu,
        0xbf2b24e3u,0xbe31a550u,0x3f39216du,
        0xbea015d5u,0x3db09552u,0x3f722939u};
    for(std::size_t i=0;i<directions.size();++i)
        Check(runtime.ReadWord(state.directions+static_cast<std::uint32_t>(i)*4u)==directions[i],
              "captured path direction bits match original reciprocal multiplication");
    const pal4::PathwayPoint prediction{{std::bit_cast<float>(0x45c1e212u),
        std::bit_cast<float>(0x44c47fc1u),std::bit_cast<float>(0xc2f05e36u)}};
    state.segment_length_bits=runtime.ReadWord(state.lengths+4u);
    for(unsigned axis=0;axis<3u;++axis)state.direction_bits[axis]=directions[3u+axis];
#if defined(_MSC_VER) && defined(_M_IX86)
    pal4::BehaviorVector3 difference{},direction{};
    for(unsigned axis=0;axis<3u;++axis){
        difference[axis]=prediction.coordinates[axis]-points[0].coordinates[axis];
        direction[axis]=std::bit_cast<float>(directions[3u+axis]);
    }
    Check(std::bit_cast<std::uint32_t>(pal4::test::MovementVectorDotPc24(direction,difference))==0x4151201fu,
          "actual original projection instructions reproduce frame88 input");
#endif
    (void)pal4::PathwayGrid::DistanceToSegment(state,prediction,points[0],points[1]);
    Check(state.projection_bits==0x4151201fu && state.nearest_bits[2]==0xc2e87444u,
          "frame88 segment projection and nearest point match original PC24");
    pal4::PathwayPoint tangent{};float outside{};
    const auto nearest=pal4::PathwayGrid::ProjectToTube(state,prediction,tangent,outside,runtime);
    Check(std::bit_cast<std::array<std::uint32_t,3>>(nearest.coordinates)==
              std::array<std::uint32_t,3>{0x45c1f4cdu,0x44c42e4cu,0xc2e87444u},
          "full frame88 path query selects the original nearest point");
}

}  // namespace

int main() {
    ConstructionAndOpenPath();
    LoopAndReplacement();
    AllocationFailure();
    QueryPath();
    CapturedM15PathDirections();
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
    return 0;
}
