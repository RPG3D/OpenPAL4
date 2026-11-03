#include "pal4/d3d_triangle_face.h"

#include <cstdlib>
#include <iostream>
#include <cmath>
#include <tuple>
#include <bit>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(const float value, const float expected) {
    return std::fabs(value - expected) < 0.00001f;
}

struct EdgeQueryRuntime final : pal4::ID3dTriangleEdgeQueryRuntime {
    std::array<std::uint32_t, 3u> results{};
    std::uint32_t calls = 0u;
    float expected_x = 0.0f;
    float expected_z = 0.0f;

    std::uint32_t Classify(
        pal4::D3dTriangleFaceState&,
        const std::uint32_t offset,
        const pal4::D3dTriangleVector2& point,
        const float threshold) override {
        Check(calls < 3u && offset == 76u + calls * 28u &&
                  point.x == expected_x && point.z == expected_z &&
                  threshold == 0.0f,
              "edge query uses three consecutive edges and X/Z input");
        return results[calls++];
    }
};

struct TraceRuntime final : pal4::ID3dTriangleTraceRuntime {
    std::array<std::uint32_t, 3u> last_classes{};
    std::array<std::uint32_t, 3u> first_classes{};
    std::uint32_t hit_class = 0u;
    std::uint32_t last_calls = 0u;
    std::uint32_t first_calls = 0u;
    std::uint32_t intersections = 0u;
    float expected_last_x = 8.0f;
    float expected_first_x = 1.0f;
    pal4::D3dTriangleVector2 intersection_value{4.0f, 5.0f};

    std::uint32_t Classify(
        pal4::D3dTriangleFaceState&,
        const std::uint32_t offset,
        const pal4::D3dTriangleVector2& point,
        const float threshold) override {
        const auto index = (offset - 76u) / 28u;
        Check(index < 3u && threshold == 0.0f,
              "trace classifies one of three edges at zero threshold");
        if (point.x == expected_last_x) {
            ++last_calls;
            return last_classes[index];
        }
        Check(point.x == expected_first_x,
              "trace uses first point only after last-point mismatch");
        ++first_calls;
        return first_classes[index];
    }
    std::uint32_t Intersect(
        const pal4::D3dTriangleEdgeProbe& probe,
        pal4::D3dTriangleFaceState&,
        const std::uint32_t offset,
        pal4::D3dTriangleVector2& output) override {
        Check(probe.first.x == expected_first_x &&
                  probe.last.x == expected_last_x &&
                  offset >= 76u && offset <= 132u,
              "intersection receives complete probe and current edge");
        ++intersections;
        output = intersection_value;
        return hit_class;
    }
};

struct NeighborRuntime final : pal4::ID3dTriangleNeighborRuntime {
    std::uint32_t instruction_data = 0u;
    std::uint32_t instruction_calls = 0u;
    std::uint32_t update_calls = 0u;
    std::array<std::uint32_t, 3u> neighbors{};
    std::array<float, 3u> distances{};
    std::uint32_t expected_instruction = 0x4567u;

    std::uint32_t InstructionData(const std::uint32_t instruction) override {
        Check(instruction == expected_instruction,
              "neighbor propagation forwards instruction to lookup");
        ++instruction_calls;
        return instruction_data;
    }
    void UpdateNeighbor(const std::uint32_t neighbor,
                        const std::uint32_t instruction,
                        pal4::D3dTriangleFaceState&,
                        const float distance) override {
        Check(instruction == expected_instruction && update_calls < 3u,
              "neighbor update receives instruction and bounded edge count");
        neighbors[update_calls] = neighbor;
        distances[update_calls++] = distance;
    }
};

struct StateRuntime final : pal4::ID3dTriangleStateRuntime {
    std::uint32_t data = 0x9911u;
    std::uint32_t lookups = 0u;
    std::uint32_t vectors = 0u;
    std::uint32_t transforms = 0u;
    bool last_incremental = false;

    std::uint32_t InstructionData(const std::uint32_t instruction) override {
        Check(instruction == 0x4567u, "state lookup receives instruction");
        ++lookups;
        return data;
    }
    pal4::D3dTriangleVector3 VectorData(
        const std::uint32_t instruction) override {
        Check(instruction == 0x4567u, "state vector lookup receives instruction");
        ++vectors;
        return {3.0f, 4.0f, 5.0f};
    }
    void UpdateTransform(const std::uint32_t instruction,
                         pal4::D3dTriangleFaceState&,
                         const bool incremental) override {
        Check(instruction == 0x4567u,
              "state transform receives instruction");
        ++transforms;
        last_incremental = incremental;
    }
};

}  // namespace

int main() {
    pal4::D3dTriangleFaceState state{};
    state.words.fill(0xaaaaaaaau);
    const pal4::D3dTriangleVector3 a{0.0f, 0.0f, 0.0f};
    const pal4::D3dTriangleVector3 b{2.0f, 0.0f, 0.0f};
    const pal4::D3dTriangleVector3 c{0.0f, 0.0f, 2.0f};
    pal4::D3dTriangleFace::Construct(state, a, b, c);
    const auto before_destroy = state.words;
    pal4::D3dTriangleFace::Destroy(state);
    Check(state.words == before_destroy,
          "face destructor preserves all words after no-op edge cleanup");
    const auto at = [&state](const std::uint32_t offset) {
        return pal4::D3dTriangleFace::FloatAt(state, offset);
    };
    Check(Near(at(28u), 0.0f) && Near(at(40u), 2.0f) &&
              Near(at(60u), 2.0f) &&
              Near(at(0u), 0.0f) && Near(at(4u), -1.0f) &&
              Near(at(8u), 0.0f) && Near(at(24u), 0.0f),
          "constructor copies vertices and computes oriented plane");
    Check(Near(at(76u), 0.0f) && Near(at(80u), 0.0f) &&
              Near(at(84u), 2.0f) && Near(at(88u), 0.0f) &&
              Near(at(104u), 2.0f) && Near(at(112u), 0.0f) &&
              Near(at(116u), 2.0f) &&
              Near(at(132u), 0.0f) && Near(at(136u), 2.0f),
          "three edge records use vertex X/Z pairs in cycle order");
    Check(state.words[25u] == 0xaaaaaa00u &&
              state.words[32u] == 0xaaaaaa00u &&
              state.words[39u] == 0xaaaaaa00u &&
              state.words[40u] == 0u &&
              state.words[45u] == 0u &&
              state.words[46u] == 0xaaaaaa00u &&
              state.words[47u] == 0xffffffffu,
          "constructor preserves untouched high bytes and initializes flags");
    Check(Near(at(64u), 2.0f / 3.0f) &&
              Near(at(68u), 0.0f) &&
              Near(at(72u), 2.0f / 3.0f) &&
              Near(at(192u), 1.0f) && Near(at(200u), 0.0f) &&
              Near(at(204u), 1.0f) && Near(at(212u), 1.0f) &&
              Near(at(216u), 0.0f) && Near(at(224u), 1.0f) &&
              Near(at(228u), 1.0f) && Near(at(232u), 1.0f) &&
              Near(at(236u), std::sqrt(2.0f)),
          "update computes centroid, midpoints and midpoint-edge lengths");

    pal4::D3dTriangleFaceState degenerate{};
    pal4::D3dTriangleFace::Construct(
        degenerate, a, a, a);
    Check(pal4::D3dTriangleFace::FloatAt(degenerate, 0u) == 0.0f &&
              pal4::D3dTriangleFace::FloatAt(degenerate, 4u) == 0.0f &&
              pal4::D3dTriangleFace::FloatAt(degenerate, 8u) == 0.0f &&
              pal4::D3dTriangleFace::FloatAt(degenerate, 228u) == 0.0f &&
              pal4::D3dTriangleFace::FloatAt(degenerate, 232u) == 0.0f &&
              pal4::D3dTriangleFace::FloatAt(degenerate, 236u) == 0.0f,
          "degenerate triangle has zero normal and midpoint distances");

    for (const auto& [first, last, word] :
         std::array<std::tuple<pal4::D3dTriangleVector3,
                               pal4::D3dTriangleVector3,
                               std::uint32_t>, 6u>{{
             {a, b, 40u}, {b, a, 40u},
             {b, c, 41u}, {c, b, 41u},
             {c, a, 42u}, {a, c, 42u}}}) {
        state.words[40u] = 0u;
        state.words[41u] = 0u;
        state.words[42u] = 0u;
        Check(pal4::D3dTriangleFace::LinkNeighbor(
                  state, first, last, 0x9000u) == 1u &&
                  state.words[word] == 0x9000u &&
                  state.words[40u] + state.words[41u] +
                      state.words[42u] == 0x9000u,
              "matching directed edge stores neighbor in shared edge slot");
    }
    const pal4::D3dTriangleVector3 unknown{3.0f, 0.0f, 0.0f};
    Check(pal4::D3dTriangleFace::LinkNeighbor(
              state, a, unknown, 0x1234u) == 0u &&
              pal4::D3dTriangleFace::LinkNeighbor(
                  state, unknown, b, 0x1234u) == 0u &&
              pal4::D3dTriangleFace::LinkNeighbor(
                  state, a, a, 0x1234u) == 0u &&
              state.words[40u] + state.words[41u] +
                  state.words[42u] == 0x9000u,
          "unmatched endpoints leave neighbor slots untouched");
    Check(pal4::D3dTriangleFace::LinkNeighbor(
              degenerate, a, a, 0x7777u) == 1u &&
              degenerate.words[40u] == 0x7777u &&
              degenerate.words[41u] == 0u &&
              degenerate.words[42u] == 0u,
          "coincident vertices choose the first AB branch");
    Check(pal4::D3dTriangleFace::SetNeighbor(
              state, 1u, 0x1234u) == 0x1234u &&
              state.words[41u] == 0x1234u &&
              state.words[40u] == 0u &&
              state.words[42u] == 0x9000u,
          "indexed setter changes only selected neighbor and returns value");

    EdgeQueryRuntime edges;
    edges.expected_x = 0.5f;
    edges.expected_z = 1.5f;
    edges.results = {0u, 2u, 3u};
    Check(pal4::D3dTriangleFace::IsOutsideEdges2D(
              state, {0.5f, 1.5f}, edges) && edges.calls == 3u,
          "three non-one classifications satisfy outside condition");
    edges.calls = 0u;
    edges.results = {1u, 2u, 3u};
    Check(!pal4::D3dTriangleFace::IsOutsideEdges3D(
               state, {0.5f, 99.0f, 1.5f}, edges) &&
              edges.calls == 3u,
          "one inside classification fails but all three edges are queried");

    Check(pal4::D3dTriangleFace::VertexAddress(
              0x5000u, 0u) == 0x501cu &&
              pal4::D3dTriangleFace::VertexAddress(
                  0x5000u, 2u) == 0x5034u &&
              pal4::D3dTriangleFace::CentroidAddress(
                  0x5000u) == 0x5040u &&
              pal4::D3dTriangleFace::Neighbor(state, 1u) == 0x1234u &&
              pal4::D3dTriangleFace::Marker(state) == 0xffffffffu,
          "accessors use exact vertex, center, neighbor and marker offsets");
    state.words[44u] = std::bit_cast<std::uint32_t>(1.25f);
    state.words[45u] = std::bit_cast<std::uint32_t>(2.5f);
    Check(pal4::D3dTriangleFace::WeightSum(state) == 3.75,
          "weight accessor adds float fields at +176 and +180");
    pal4::D3dTriangleVector3 midpoint{};
    Check(&pal4::D3dTriangleFace::CopyMidpoint(
              state, 1u, midpoint) == &midpoint &&
              midpoint.x == 1.0f && midpoint.y == 0.0f &&
              midpoint.z == 1.0f,
          "indexed midpoint copy forwards three fields and output address");

    const pal4::D3dTriangleEdgeProbe probe{{1.0f, 2.0f},
                                            {8.0f, 9.0f}};
    std::uint32_t neighbor = 0xaaaau;
    std::uint32_t edge_index = 0xbbbbu;
    pal4::D3dTriangleVector2 intersection{7.0f, 7.0f};
    TraceRuntime trace;
    trace.last_classes = {2u, 2u, 2u};
    Check(pal4::D3dTriangleFace::TraceEdges(
              state, probe, neighbor, edge_index,
              intersection, trace) == 1u &&
              trace.last_calls == 3u && trace.first_calls == 0u &&
              trace.intersections == 0u &&
              neighbor == 0xaaaau && edge_index == 0xbbbbu,
          "three last-point class-two edges return one without outputs");

    trace = {};
    trace.last_classes = {2u, 0u, 2u};
    trace.first_classes = {0u, 0u, 0u};
    trace.hit_class = 3u;
    Check(pal4::D3dTriangleFace::TraceEdges(
              state, probe, neighbor, edge_index,
              intersection, trace) == 2u &&
              trace.last_calls == 2u && trace.first_calls == 1u &&
              trace.intersections == 1u &&
              neighbor == 0x1234u && edge_index == 1u &&
              intersection.x == 4.0f && intersection.z == 5.0f,
          "class-three intersection returns selected neighbor and edge");

    trace = {};
    trace.first_classes = {1u, 1u, 1u};
    neighbor = 0xaaaau;
    edge_index = 0xbbbbu;
    Check(pal4::D3dTriangleFace::TraceEdges(
              state, probe, neighbor, edge_index,
              intersection, trace) == 0u &&
              trace.last_calls == 3u && trace.first_calls == 3u &&
              trace.intersections == 0u &&
              neighbor == 0xaaaau && edge_index == 0xbbbbu,
          "first-point class-one misses scan all edges without output writes");

    pal4::D3dTriangleVector2 point{1.0f, 0.0f};
    Check(pal4::D3dTriangleFace::PushPointFromEdge(
              state, 0u, point) &&
              Near(point.x, 1.0f) && Near(point.z, -0.001f) &&
              (state.words[25u] & 0xffu) == 1u,
          "edge push computes cached normal and applies boundary tolerance");
    point = {1.0f, -0.01f};
    Check(!pal4::D3dTriangleFace::PushPointFromEdge(
               state, 0u, point) &&
              point.x == 1.0f && point.z == -0.01f,
          "point beyond positive tolerance remains unchanged");
    point = {1.0f, 0.1f};
    Check(pal4::D3dTriangleFace::PushPointFromEdge(
              state, 0u, point) && Near(point.z, -0.001f),
          "negative signed distance is moved beyond the edge");

    const auto center_x = pal4::D3dTriangleFace::FloatAt(state, 64u);
    const auto center_z = pal4::D3dTriangleFace::FloatAt(state, 72u);
    trace = {};
    trace.expected_first_x = center_x;
    trace.expected_last_x = 8.0f;
    trace.first_classes = {1u, 1u, 1u};
    point = {8.0f, 9.0f};
    Check(pal4::D3dTriangleFace::ClampProbeToFace(
              state, point, trace) &&
              Near(point.x, center_x) && Near(point.z, center_z),
          "zero trace result replaces target with face center");
    trace = {};
    trace.expected_first_x = center_x;
    trace.expected_last_x = 8.0f;
    trace.last_classes = {2u, 2u, 2u};
    point = {8.0f, 9.0f};
    Check(!pal4::D3dTriangleFace::ClampProbeToFace(
               state, point, trace) &&
              point.x == 8.0f && point.z == 9.0f,
          "class-one trace leaves target unchanged and reports false");
    trace = {};
    trace.expected_first_x = center_x;
    trace.expected_last_x = 8.0f;
    trace.hit_class = 2u;
    point = {8.0f, 9.0f};
    Check(pal4::D3dTriangleFace::ClampProbeToFace(
              state, point, trace) &&
              Near(point.x, center_x + (4.0f - center_x) * 0.9f) &&
              Near(point.z, center_z + (5.0f - center_z) * 0.9f),
          "intersection retreats to ninety percent of center-to-hit span");

    trace = {};
    trace.expected_first_x = center_x;
    trace.expected_last_x = 0.0f;
    trace.first_classes = {1u, 1u, 1u};
    pal4::D3dTriangleEdgeProbe moving{{0.0f, 0.0f},
                                       {1.0f, 1.0f}};
    moving.flags[1u] = 0x5au;
    Check(&pal4::D3dTriangleFace::AdjustProbeAlongEdge(
              state, 0u, moving, trace) == &moving.last &&
              Near(moving.first.x, center_x) &&
              Near(moving.first.z, center_z) &&
              Near(moving.last.x, 1.0f) &&
              Near(moving.last.z, -0.001f) &&
              moving.flags[0u] == 0u &&
              moving.flags[1u] == 0x5au,
          "edge adjustment projects last point, clamps first and pushes last");

    trace = {};
    trace.expected_first_x = center_x;
    trace.expected_last_x = 8.0f;
    trace.first_classes = {1u, 1u, 1u};
    pal4::D3dTriangleVector3 point3{8.0f, 17.0f, 9.0f};
    Check(pal4::D3dTriangleFace::ClampPointToFace(
              state, point3, trace) &&
              Near(point3.x, center_x) && Near(point3.z, center_z) &&
              point3.y == 17.0f,
          "3D clamp forwards X/Z and preserves Y after success");
    trace = {};
    trace.expected_first_x = center_x;
    trace.expected_last_x = 8.0f;
    trace.last_classes = {2u, 2u, 2u};
    point3 = {8.0f, 17.0f, 9.0f};
    Check(!pal4::D3dTriangleFace::ClampPointToFace(
               state, point3, trace) &&
              point3.x == 8.0f && point3.y == 17.0f &&
              point3.z == 9.0f,
          "3D clamp leaves all coordinates unchanged after failure");

    NeighborRuntime neighbor_runtime;
    state.words[43u] = 0x1122u;
    state.words[46u] = 0xaabbcc7fu;
    neighbor_runtime.instruction_data = 0x3344u;
    Check(!pal4::D3dTriangleFace::PropagateNeighborState(
               state, 0x4567u, neighbor_runtime) &&
              state.words[46u] == 0xaabbcc7fu &&
              neighbor_runtime.update_calls == 0u,
          "instruction mismatch leaves face and neighbors untouched");
    neighbor_runtime.instruction_data = 0x1122u;
    state.words[40u] = 0u;
    state.words[41u] = 0u;
    state.words[42u] = 0u;
    state.words[47u] = 0xffffffffu;
    Check(pal4::D3dTriangleFace::PropagateNeighborState(
              state, 0x4567u, neighbor_runtime) &&
              state.words[46u] == 0xaabbcc00u &&
              neighbor_runtime.update_calls == 0u,
          "empty neighbor list succeeds even with unset origin edge");
    state.words[40u] = 0x1000u;
    state.words[42u] = 0x3000u;
    state.words[46u] = 0x123456ffu;
    Check(!pal4::D3dTriangleFace::PropagateNeighborState(
               state, 0x4567u, neighbor_runtime) &&
              state.words[46u] == 0x12345600u &&
              neighbor_runtime.update_calls == 0u,
          "unset origin aborts at first nonzero neighbor after clearing flag");
    state.words[47u] = 1u;
    state.words[44u] = std::bit_cast<std::uint32_t>(2.5f);
    state.words[57u] = std::bit_cast<std::uint32_t>(10.0f);
    state.words[58u] = std::bit_cast<std::uint32_t>(20.0f);
    Check(pal4::D3dTriangleFace::PropagateNeighborState(
              state, 0x4567u, neighbor_runtime) &&
              neighbor_runtime.update_calls == 2u &&
              neighbor_runtime.neighbors[0u] == 0x1000u &&
              neighbor_runtime.neighbors[1u] == 0x3000u &&
              Near(neighbor_runtime.distances[0u], 22.5f) &&
              Near(neighbor_runtime.distances[1u], 22.5f),
          "neighbor propagation skips empty edge and uses origin-relative distance");

    pal4::D3dTriangleFaceState stateful{};
    StateRuntime state_runtime;
    stateful.words[40u] = 0x1000u;
    stateful.words[41u] = 0x2000u;
    stateful.words[42u] = 0x3000u;
    stateful.words[43u] = 0x7777u;
    stateful.words[46u] = 0xabcdef80u;
    stateful.words[47u] = 0xffffffffu;
    Check(pal4::D3dTriangleFace::UpdateState(
              stateful, 0x4567u, 0x2000u, 12.0f,
              state_runtime) &&
              stateful.words[43u] == 0x9911u &&
              stateful.words[46u] == 0xabcdef01u &&
              stateful.words[47u] == 1u &&
              pal4::D3dTriangleFace::FloatAt(stateful, 176u) == 12.0f &&
              state_runtime.lookups == 2u &&
              state_runtime.vectors == 1u &&
              state_runtime.transforms == 1u &&
              !state_runtime.last_incremental,
          "new instruction activates matching source and refreshes transform");
    stateful.words[47u] = 7u;
    state_runtime.data = 0x9912u;
    Check(pal4::D3dTriangleFace::UpdateState(
              stateful, 0x4567u, 0x9999u, 8.0f,
              state_runtime) &&
              stateful.words[47u] == 7u &&
              pal4::D3dTriangleFace::FloatAt(stateful, 176u) == 8.0f &&
              state_runtime.transforms == 2u,
          "unmatched nonzero source retains origin on instruction change");
    state_runtime.data = 0x9913u;
    stateful.words[45u] = std::bit_cast<std::uint32_t>(6.0f);
    Check(pal4::D3dTriangleFace::UpdateState(
              stateful, 0x4567u, 0u, 99.0f,
              state_runtime) &&
              stateful.words[44u] == 0u && stateful.words[45u] == 0u &&
              stateful.words[47u] == 0u &&
              stateful.words[46u] == 0xabcdef00u &&
              state_runtime.vectors == 2u &&
              state_runtime.transforms == 3u,
          "null source clears propagation fields without vector lookup");
    Check(!pal4::D3dTriangleFace::UpdateState(
               stateful, 0x4567u, 0x1000u, 2.0f,
               state_runtime) &&
              state_runtime.transforms == 3u &&
              stateful.words[44u] == 0u,
          "same instruction with inactive flag returns without update");
    stateful.words[46u] |= 1u;
    stateful.words[44u] = std::bit_cast<std::uint32_t>(8.0f);
    stateful.words[45u] = std::bit_cast<std::uint32_t>(2.0f);
    Check(!pal4::D3dTriangleFace::UpdateState(
               stateful, 0x4567u, 0x3000u, 8.0f,
               state_runtime) &&
              state_runtime.transforms == 3u &&
              stateful.words[47u] == 0u,
          "equal distance is rejected without changing source edge");
    Check(pal4::D3dTriangleFace::UpdateState(
              stateful, 0x4567u, 0x3000u, 5.0f,
              state_runtime) &&
              pal4::D3dTriangleFace::FloatAt(stateful, 176u) == 5.0f &&
              stateful.words[47u] == 2u &&
              state_runtime.transforms == 4u &&
              state_runtime.last_incremental,
          "shorter distance updates source edge through incremental transform");

    stateful.words[16u] = std::bit_cast<std::uint32_t>(1.0f);
    stateful.words[17u] = std::bit_cast<std::uint32_t>(2.0f);
    stateful.words[18u] = std::bit_cast<std::uint32_t>(3.0f);
    pal4::D3dTriangleFace::UpdateVectorRadius(
        stateful, {6.0f, 4.0f, 4.0f});
    Check(pal4::D3dTriangleFace::FloatAt(stateful, 180u) == 5.0f,
          "vector radius selects X component when greatest");
    pal4::D3dTriangleFace::UpdateVectorRadius(
        stateful, {2.0f, -5.0f, 4.0f});
    Check(pal4::D3dTriangleFace::FloatAt(stateful, 180u) == 7.0f,
          "vector radius selects absolute Y component when greatest");
    pal4::D3dTriangleFace::UpdateVectorRadius(
        stateful, {2.0f, 4.0f, -8.0f});
    Check(pal4::D3dTriangleFace::FloatAt(stateful, 180u) == 11.0f,
          "vector radius selects absolute Z component when greatest");
}
