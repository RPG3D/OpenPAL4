#include "pal4/d3d_triangle_face.h"

#include <bit>
#include <cmath>

namespace pal4 {

void D3dTriangleFace::Destroy(D3dTriangleFaceState&) {}

namespace {

void WriteFloat(D3dTriangleFaceState& state,
                const std::uint32_t offset, const float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}

void WriteVector(D3dTriangleFaceState& state,
                 const std::uint32_t offset,
                 const D3dTriangleVector3& value) {
    WriteFloat(state, offset, value.x);
    WriteFloat(state, offset + 4u, value.y);
    WriteFloat(state, offset + 8u, value.z);
}

D3dTriangleVector3 ReadVector(const D3dTriangleFaceState& state,
                              const std::uint32_t offset) {
    return {D3dTriangleFace::FloatAt(state, offset),
            D3dTriangleFace::FloatAt(state, offset + 4u),
            D3dTriangleFace::FloatAt(state, offset + 8u)};
}

D3dTriangleVector3 Subtract(const D3dTriangleVector3& a,
                            const D3dTriangleVector3& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

float Length(const D3dTriangleVector3& value) {
    return std::sqrt(value.x * value.x + value.y * value.y +
                     value.z * value.z);
}

void WriteEdge(D3dTriangleFaceState& state,
               const std::uint32_t offset,
               const D3dTriangleVector3& first,
               const D3dTriangleVector3& last) {
    WriteFloat(state, offset, first.x);
    WriteFloat(state, offset + 4u, first.z);
    WriteFloat(state, offset + 8u, last.x);
    WriteFloat(state, offset + 12u, last.z);
    state.words[(offset + 24u) / 4u] &= 0xffffff00u;
}

D3dTriangleVector3 Midpoint(const D3dTriangleVector3& a,
                            const D3dTriangleVector3& b) {
    return {(a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f,
            (a.z + b.z) * 0.5f};
}

bool Equals(const D3dTriangleVector3& a,
            const D3dTriangleVector3& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

}  // namespace

float D3dTriangleFace::FloatAt(const D3dTriangleFaceState& state,
                               const std::uint32_t byte_offset) {
    return std::bit_cast<float>(state.words[byte_offset / 4u]);
}

void D3dTriangleFace::Construct(
    D3dTriangleFaceState& state, const D3dTriangleVector3& a,
    const D3dTriangleVector3& b, const D3dTriangleVector3& c) {
    WriteVector(state, 28u, a);
    WriteVector(state, 40u, b);
    WriteVector(state, 52u, c);
    for (std::uint32_t index = 40u; index <= 45u; ++index) {
        state.words[index] = 0u;
    }
    state.words[46u] &= 0xffffff00u;
    state.words[47u] = 0xffffffffu;
    UpdateGeometry(state);
}

void D3dTriangleFace::UpdateGeometry(D3dTriangleFaceState& state) {
    const auto a = ReadVector(state, 28u);
    const auto b = ReadVector(state, 40u);
    const auto c = ReadVector(state, 52u);
    WriteEdge(state, 76u, a, b);
    WriteEdge(state, 104u, b, c);
    WriteEdge(state, 132u, c, a);

    const auto ab = Subtract(b, a);
    const auto ac = Subtract(c, a);
    D3dTriangleVector3 normal{
        ab.y * ac.z - ab.z * ac.y,
        ab.z * ac.x - ab.x * ac.z,
        ab.x * ac.y - ab.y * ac.x};
    const auto magnitude = Length(normal);
    const auto inverse = magnitude == 0.0f ? 0.0f : 1.0f / magnitude;
    normal.x *= inverse;
    normal.y *= inverse;
    normal.z *= inverse;
    WriteVector(state, 0u, normal);
    WriteVector(state, 12u, a);
    WriteFloat(state, 24u,
               -(a.z * normal.z + a.y * normal.y + a.x * normal.x));

    constexpr float third = 0.33333334f;
    WriteVector(state, 64u,
                {(c.x + b.x + a.x) * third,
                 (b.y + c.y + a.y) * third,
                 (b.z + c.z + a.z) * third});

    const auto midpoint_ab = Midpoint(a, b);
    const auto midpoint_bc = Midpoint(b, c);
    const auto midpoint_ca = Midpoint(c, a);
    WriteVector(state, 192u, midpoint_ab);
    WriteVector(state, 204u, midpoint_bc);
    WriteVector(state, 216u, midpoint_ca);
    WriteFloat(state, 228u, Length(Subtract(midpoint_ab, midpoint_bc)));
    WriteFloat(state, 232u, Length(Subtract(midpoint_bc, midpoint_ca)));
    WriteFloat(state, 236u, Length(Subtract(midpoint_ca, midpoint_ab)));
}

std::uint8_t D3dTriangleFace::LinkNeighbor(
    D3dTriangleFaceState& state,
    const D3dTriangleVector3& first,
    const D3dTriangleVector3& last,
    const std::uint32_t neighbor) {
    const auto a = ReadVector(state, 28u);
    const auto b = ReadVector(state, 40u);
    const auto c = ReadVector(state, 52u);
    if (Equals(a, first)) {
        if (Equals(b, last)) {
            state.words[40u] = neighbor;
            return 1u;
        }
        if (Equals(c, last)) {
            state.words[42u] = neighbor;
            return 1u;
        }
    } else if (Equals(b, first)) {
        if (Equals(a, last)) {
            state.words[40u] = neighbor;
            return 1u;
        }
        if (Equals(c, last)) {
            state.words[41u] = neighbor;
            return 1u;
        }
    } else if (Equals(c, first)) {
        if (Equals(a, last)) {
            state.words[42u] = neighbor;
            return 1u;
        }
        if (Equals(b, last)) {
            state.words[41u] = neighbor;
            return 1u;
        }
    }
    return 0u;
}

std::uint32_t D3dTriangleFace::SetNeighbor(
    D3dTriangleFaceState& state, const std::uint32_t edge,
    const std::uint32_t neighbor) {
    state.words[40u + edge] = neighbor;
    return neighbor;
}

bool D3dTriangleFace::IsOutsideEdges2D(
    D3dTriangleFaceState& state,
    const D3dTriangleVector2& point,
    ID3dTriangleEdgeQueryRuntime& runtime) {
    std::uint32_t outside_count = 0u;
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        if (runtime.Classify(state, 76u + 28u * index,
                             point, 0.0f) != 1u) {
            ++outside_count;
        }
    }
    return outside_count == 3u;
}

bool D3dTriangleFace::IsOutsideEdges3D(
    D3dTriangleFaceState& state,
    const D3dTriangleVector3& point,
    ID3dTriangleEdgeQueryRuntime& runtime) {
    return IsOutsideEdges2D(state, {point.x, point.z}, runtime);
}

std::uint32_t D3dTriangleFace::TraceEdges(
    D3dTriangleFaceState& state,
    const D3dTriangleEdgeProbe& probe,
    std::uint32_t& neighbor,
    std::uint32_t& edge_index,
    D3dTriangleVector2& intersection,
    ID3dTriangleTraceRuntime& runtime) {
    std::uint32_t first_inside_count = 0u;
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        const auto offset = 76u + 28u * index;
        if (runtime.Classify(state, offset, probe.last, 0.0f) == 2u) {
            ++first_inside_count;
            continue;
        }
        if (runtime.Classify(state, offset, probe.first, 0.0f) == 1u) {
            continue;
        }
        const auto hit = runtime.Intersect(probe, state, offset,
                                           intersection);
        if (hit == 2u || hit == 3u) {
            neighbor = state.words[40u + index];
            edge_index = index;
            return 2u;
        }
    }
    return first_inside_count == 3u ? 1u : 0u;
}

bool D3dTriangleFace::PushPointFromEdge(
    D3dTriangleFaceState& state, const std::uint32_t edge,
    D3dTriangleVector2& point) {
    constexpr float epsilon = 0.001f;
    const auto offset = 76u + edge * 28u;
    const auto flag_index = (offset + 24u) / 4u;
    if ((state.words[flag_index] & 0xffu) == 0u) {
        const auto dx = FloatAt(state, offset + 8u) -
            FloatAt(state, offset);
        const auto dz = FloatAt(state, offset + 12u) -
            FloatAt(state, offset + 4u);
        const auto length = std::sqrt(dx * dx + dz * dz);
        const auto inv = length == 0.0f ? 0.0f : 1.0f / length;
        WriteFloat(state, offset + 16u, dz * inv);
        WriteFloat(state, offset + 20u, -dx * inv);
        state.words[flag_index] =
            (state.words[flag_index] & 0xffffff00u) | 1u;
    }
    const auto nx = FloatAt(state, offset + 16u);
    const auto nz = FloatAt(state, offset + 20u);
    auto distance = (point.x - FloatAt(state, offset)) * nx +
        (point.z - FloatAt(state, offset + 4u)) * nz;
    if (distance > epsilon) return false;
    if (distance <= 0.0f) distance -= epsilon;
    auto shift = std::fabs(distance);
    if (shift <= epsilon) shift = epsilon;
    point.x += nx * shift;
    point.z += nz * shift;
    return true;
}

bool D3dTriangleFace::ClampProbeToFace(
    D3dTriangleFaceState& state, D3dTriangleVector2& point,
    ID3dTriangleTraceRuntime& runtime) {
    const D3dTriangleVector2 center{FloatAt(state, 64u),
                                     FloatAt(state, 72u)};
    const D3dTriangleEdgeProbe probe{center, point};
    std::uint32_t neighbor = 0u;
    std::uint32_t edge = 0u;
    D3dTriangleVector2 intersection{};
    const auto status = TraceEdges(
        state, probe, neighbor, edge, intersection, runtime);
    if (status == 2u) {
        constexpr float retreat = 0.89999998f;
        point.x = center.x + (intersection.x - center.x) * retreat;
        point.z = center.z + (intersection.z - center.z) * retreat;
        return true;
    }
    if (status == 0u) {
        point = center;
        return true;
    }
    return false;
}

bool D3dTriangleFace::ClampPointToFace(
    D3dTriangleFaceState& state, D3dTriangleVector3& point,
    ID3dTriangleTraceRuntime& runtime) {
    D3dTriangleVector2 horizontal{point.x, point.z};
    if (!ClampProbeToFace(state, horizontal, runtime)) return false;
    point.x = horizontal.x;
    point.z = horizontal.z;
    return true;
}

bool D3dTriangleFace::PropagateNeighborState(
    D3dTriangleFaceState& state, const std::uint32_t instruction,
    ID3dTriangleNeighborRuntime& runtime) {
    if (state.words[43u] != runtime.InstructionData(instruction)) return false;
    state.words[46u] &= 0xffffff00u;
    const auto origin = static_cast<std::int32_t>(state.words[47u]);
    for (std::uint32_t edge = 0u; edge < 3u; ++edge) {
        const auto neighbor = state.words[40u + edge];
        if (neighbor == 0u) continue;
        if (origin < 0) return false;
        const auto distance_index = static_cast<std::uint32_t>(
            std::abs(static_cast<std::int32_t>(edge) - origin));
        runtime.UpdateNeighbor(neighbor, instruction, state,
                               FloatAt(state, 228u + 4u * distance_index) +
                                   FloatAt(state, 176u));
    }
    return true;
}

bool D3dTriangleFace::UpdateState(
    D3dTriangleFaceState& state, const std::uint32_t instruction,
    const std::uint32_t source_neighbor, const float distance,
    ID3dTriangleStateRuntime& runtime) {
    const auto instruction_data = runtime.InstructionData(instruction);
    if (state.words[43u] != instruction_data) {
        state.words[43u] = runtime.InstructionData(instruction);
        if (source_neighbor == 0u) {
            state.words[46u] &= 0xffffff00u;
            state.words[44u] = 0u;
            state.words[45u] = 0u;
            state.words[47u] = 0u;
        } else {
            state.words[46u] = (state.words[46u] & 0xffffff00u) | 1u;
            UpdateVectorRadius(state, runtime.VectorData(instruction));
            WriteFloat(state, 176u, distance);
            if (source_neighbor == state.words[40u]) {
                state.words[47u] = 0u;
            } else if (source_neighbor == state.words[41u]) {
                state.words[47u] = 1u;
            } else if (source_neighbor == state.words[42u]) {
                state.words[47u] = 2u;
            }
        }
        runtime.UpdateTransform(instruction, state, false);
        return true;
    }
    if ((state.words[46u] & 0xffu) == 0u) return false;
    const auto accumulated = FloatAt(state, 180u);
    const auto previous = FloatAt(state, 176u);
    if (!(static_cast<double>(accumulated) + previous >
          static_cast<double>(accumulated) + distance)) return false;
    WriteFloat(state, 176u, distance);
    if (source_neighbor == state.words[40u]) {
        state.words[47u] = 0u;
    } else if (source_neighbor == state.words[41u]) {
        state.words[47u] = 1u;
    } else if (source_neighbor == state.words[42u]) {
        state.words[47u] = 2u;
    }
    runtime.UpdateTransform(instruction, state, true);
    return true;
}

void D3dTriangleFace::UpdateVectorRadius(
    D3dTriangleFaceState& state, const D3dTriangleVector3& vector) {
    const auto x = std::fabs(vector.x - FloatAt(state, 64u));
    const auto y = std::fabs(vector.y - FloatAt(state, 68u));
    const auto z = std::fabs(vector.z - FloatAt(state, 72u));
    const auto xy = x > y ? x : y;
    WriteFloat(state, 180u, xy > z ? xy : z);
}

D3dTriangleVector2& D3dTriangleFace::AdjustProbeAlongEdge(
    D3dTriangleFaceState& state, const std::uint32_t edge,
    D3dTriangleEdgeProbe& probe,
    ID3dTriangleTraceRuntime& runtime) {
    const auto offset = 76u + edge * 28u;
    auto dx = FloatAt(state, offset + 8u) - FloatAt(state, offset);
    auto dz = FloatAt(state, offset + 12u) -
        FloatAt(state, offset + 4u);
    const auto length = std::sqrt(dx * dx + dz * dz);
    const auto inverse = length == 0.0f ? 0.0f : 1.0f / length;
    dx *= inverse;
    dz *= inverse;
    const auto projected =
        (probe.last.x - probe.first.x) * dx +
        (probe.last.z - probe.first.z) * dz;
    probe.last = {probe.first.x + dx * projected,
                  probe.first.z + dz * projected};
    probe.flags[0u] = 0u;

    auto point = probe.first;
    (void)ClampProbeToFace(state, point, runtime);
    probe.first = point;
    probe.flags[0u] = 0u;

    point = probe.last;
    (void)PushPointFromEdge(state, edge, point);
    probe.last = point;
    probe.flags[0u] = 0u;
    return probe.last;
}

std::uint32_t D3dTriangleFace::VertexAddress(
    const std::uint32_t state_address, const std::uint32_t index) {
    return state_address + 28u + index * 12u;
}

std::uint32_t D3dTriangleFace::CentroidAddress(
    const std::uint32_t state_address) {
    return state_address + 64u;
}

std::uint32_t D3dTriangleFace::Neighbor(
    const D3dTriangleFaceState& state, const std::uint32_t edge) {
    return state.words[40u + edge];
}

double D3dTriangleFace::WeightSum(const D3dTriangleFaceState& state) {
    return static_cast<double>(FloatAt(state, 180u)) +
        static_cast<double>(FloatAt(state, 176u));
}

std::uint32_t D3dTriangleFace::Marker(
    const D3dTriangleFaceState& state) {
    return state.words[47u];
}

D3dTriangleVector3& D3dTriangleFace::CopyMidpoint(
    const D3dTriangleFaceState& state, const std::uint32_t edge,
    D3dTriangleVector3& output) {
    output = ReadVector(state, 192u + edge * 12u);
    return output;
}

}  // namespace pal4
