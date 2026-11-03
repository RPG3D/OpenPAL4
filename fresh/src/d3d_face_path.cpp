#include "pal4/d3d_face_path.h"

#include <bit>

namespace pal4 {

namespace {

void StoreVector(D3dFacePathState& state, const std::uint32_t first_word,
                 const D3dTriangleVector3& vector) {
    state.words[first_word] = std::bit_cast<std::uint32_t>(vector.x);
    state.words[first_word + 1u] = std::bit_cast<std::uint32_t>(vector.y);
    state.words[first_word + 2u] = std::bit_cast<std::uint32_t>(vector.z);
}

D3dTriangleVector3 LoadVector(const D3dFacePathState& state,
                              const std::uint32_t first_word) {
    return {std::bit_cast<float>(state.words[first_word]),
            std::bit_cast<float>(state.words[first_word + 1u]),
            std::bit_cast<float>(state.words[first_word + 2u])};
}

D3dTriangleVector3 ReadNodeVector(const std::uint32_t node,
                                  ID3dFacePathRuntime& runtime) {
    return {std::bit_cast<float>(runtime.ReadWord(node + 8u)),
            std::bit_cast<float>(runtime.ReadWord(node + 12u)),
            std::bit_cast<float>(runtime.ReadWord(node + 16u))};
}

std::uint32_t AppendPoint(D3dFacePathState& state,
                          const D3dFacePathPoint& point,
                          ID3dFacePathRuntime& runtime) {
    const auto sentinel = state.words[10u];
    const auto previous = runtime.ReadWord(sentinel + 4u);
    const auto node = runtime.Allocate(24u);
    runtime.WriteWord(node, sentinel);
    runtime.WriteWord(node + 4u, previous == 0u ? node : previous);
    runtime.WriteWord(sentinel + 4u, node);
    runtime.WriteWord(previous == 0u ? node : previous, node);
    D3dFacePathPoint copy{};
    D3dFacePath::CopyPoint(&copy, point);
    runtime.WriteWord(node + 8u, std::bit_cast<std::uint32_t>(copy.position.x));
    runtime.WriteWord(node + 12u, std::bit_cast<std::uint32_t>(copy.position.y));
    runtime.WriteWord(node + 16u, std::bit_cast<std::uint32_t>(copy.position.z));
    runtime.WriteWord(node + 20u, copy.face);
    return ++state.words[11u];
}

}  // namespace

std::uint32_t D3dFacePath::Reset(
    D3dFacePathState& state, const std::uint32_t storage_address,
    const D3dTriangleVector3& start, const std::uint32_t start_face,
    const D3dTriangleVector3& end, const std::uint32_t end_face,
    ID3dFacePathRuntime& runtime) {
    const auto sentinel = state.words[10u];
    for (auto node = runtime.ReadWord(sentinel); node != sentinel;) {
        const auto next = runtime.ReadWord(node);
        const auto previous = runtime.ReadWord(node + 4u);
        runtime.WriteWord(previous, next);
        runtime.WriteWord(next + 4u, previous);
        runtime.Free(node);
        --state.words[11u];
        node = next;
    }
    state.words[0u] = storage_address;
    StoreVector(state, 1u, start);
    state.words[4u] = start_face;
    StoreVector(state, 5u, end);
    state.words[8u] = end_face;
    return Append(state, start, start_face, runtime);
}

std::uint32_t D3dFacePath::Append(
    D3dFacePathState& state, const D3dTriangleVector3& point,
    const std::uint32_t face, ID3dFacePathRuntime& runtime) {
    return AppendPoint(state, {point, face}, runtime);
}

std::uint32_t D3dFacePath::AppendEnd(
    D3dFacePathState& state, ID3dFacePathRuntime& runtime) {
    return AppendPoint(state, {LoadVector(state, 5u), state.words[8u]},
                       runtime);
}

std::uint32_t D3dFacePath::ListFieldsAddress(
    const std::uint32_t state_address) {
    return state_address + 36u;
}

std::uint32_t D3dFacePath::SkipReachable(
    const D3dFacePathState& state, const std::uint32_t cursor,
    ID3dFacePathRuntime& runtime,
    ID3dFaceTraceReachabilityRuntime& trace_runtime) {
    const auto sentinel = state.words[10u];
    if (cursor == sentinel) return sentinel;
    auto candidate = runtime.ReadWord(cursor);
    if (candidate == sentinel) return sentinel;
    auto lookahead = runtime.ReadWord(candidate);
    if (lookahead == sentinel) return candidate;

    const auto source_face = runtime.ReadWord(cursor + 20u);
    const auto source_point = ReadNodeVector(cursor, runtime);
    while (D3dFaceTraceReachability::CanReach(
        source_face, source_point, ReadNodeVector(lookahead, runtime),
        trace_runtime)) {
        candidate = lookahead;
        lookahead = runtime.ReadWord(lookahead);
        if (lookahead == sentinel) break;
    }
    return candidate;
}

void D3dFacePath::CopyPoint(D3dFacePathPoint* destination,
                            const D3dFacePathPoint& source) {
    if (destination != nullptr) *destination = source;
}

}  // namespace pal4
