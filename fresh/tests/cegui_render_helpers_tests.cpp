#include "pal4/cegui_render_helpers.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::CeguiRenderHelperStub {
public:
    std::vector<std::uint32_t> reads;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> states;
    std::vector<std::uint32_t> draw_args;
    bool throw_on_draw = false;

    std::uint32_t ReadDword(const std::uint32_t address) override {
        reads.push_back(address);
        return pal4::CeguiRenderHelperStub::ReadDword(address);
    }
    std::uint32_t SetRenderState(
        const std::uint32_t state_id,
        const std::uint32_t value) override {
        states.emplace_back(state_id, value);
        return 0x5000u + state_id + value;
    }
    std::uint32_t DrawIndexed(
        const std::uint32_t primitive_type,
        const std::uint32_t vertices,
        const std::uint32_t vertex_count,
        const std::uint32_t indices,
        const std::uint32_t index_count) override {
        draw_args = {primitive_type, vertices, vertex_count,
                     indices, index_count};
        if (throw_on_draw) {
            throw std::runtime_error("draw failed");
        }
        return 0xabcdu;
    }
};

void GameStateMinimumUsesUnsignedComparison() {
    Runtime runtime;
    runtime.WriteDword(0x155cu, 0x5000u);
    runtime.WriteDword(0x5058u, 100u);
    runtime.WriteDword(0x505cu, 40u);
    Check(pal4::CeguiRenderHelpers::MinGameStateExtent(
              0x1000u, runtime) == 40u &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x155cu, 0x505cu, 0x5058u},
          "minimum reads game-state pointer and fields in assembly order");
    runtime.WriteDword(0x5058u, 0x80000000u);
    runtime.WriteDword(0x505cu, 0x7fffffffu);
    Check(pal4::CeguiRenderHelpers::MinGameStateExtent(
              0x1000u, runtime) == 0x7fffffffu,
          "minimum compares unsigned DWORDs across sign bit");
    runtime.WriteDword(0x5058u, 77u);
    runtime.WriteDword(0x505cu, 77u);
    Check(pal4::CeguiRenderHelpers::MinGameStateExtent(
              0x1000u, runtime) == 77u,
          "equal values return the first field");
}

void RenderStatesKeepSixExactValuesAndLastResult() {
    Runtime runtime;
    Check(pal4::CeguiRenderHelpers::SetRenderStates(runtime) ==
              0x5000u + 2u + 3u &&
              runtime.states ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {12u, 1u}, {6u, 0u}, {8u, 0u},
                      {7u, 2u}, {9u, 2u}, {2u, 3u}},
          "common renderer setup uses IDA state IDs and values");
}

void EmptyFlushSkipsDrawAndPreservesFields() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    state.WriteDword(204u, 0x6000u);
    state.WriteDword(220u, 0x7000u);
    Check(!pal4::CeguiRenderHelpers::FlushGeometry(
              state, runtime).has_value() &&
              runtime.draw_args.empty() &&
              state.ReadDword(204u) == 0x6000u &&
              state.ReadDword(220u) == 0x7000u,
          "zero count returns without RenderWare draw");
}

void NonemptyFlushSubmitsAndResetsCounter() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    state.WriteDword(204u, 0x6000u);
    state.WriteDword(220u, 0x7000u);
    state.WriteDword(264u, 10u);
    Check(pal4::CeguiRenderHelpers::FlushGeometry(state, runtime) ==
              0xabcdu &&
              runtime.draw_args ==
                  std::vector<std::uint32_t>{3u, 0x6000u, 10u,
                                             0x7000u, 12u} &&
              state.ReadDword(264u) == 0u,
          "draw uses six indices per complete quad and clears counter");
}

void DrawFailureLeavesCounterUnchanged() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    runtime.throw_on_draw = true;
    state.WriteDword(264u, 4u);
    bool threw = false;
    try {
        static_cast<void>(pal4::CeguiRenderHelpers::FlushGeometry(
            state, runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && state.ReadDword(264u) == 4u,
          "counter is cleared only after draw returns");
}

}  // namespace

int main() {
    GameStateMinimumUsesUnsignedComparison();
    RenderStatesKeepSixExactValuesAndLastResult();
    EmptyFlushSkipsDrawAndPreservesFields();
    NonemptyFlushSubmitsAndResetsCounter();
    DrawFailureLeavesCounterUnchanged();
    std::cout << "5 CEGUI render helper test cases passed\n";
}
