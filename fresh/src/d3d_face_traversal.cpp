#include "pal4/d3d_face_traversal.h"

#include <bit>

namespace pal4 {

namespace {

std::size_t FindCandidate(
    const std::span<const D3dFaceCandidate> entries,
    const std::uint32_t face_address) {
    for (std::size_t index = 0u; index < entries.size(); ++index) {
        if (entries[index].face == face_address) return index;
    }
    return entries.size();
}

}  // namespace

std::uint32_t D3dFaceTraversal::ResetSeed(
    D3dFaceTraversalState& state, const std::uint32_t generation,
    const D3dTriangleVector3& seed) {
    state.words[6u] = std::bit_cast<std::uint32_t>(seed.x);
    state.words[7u] = std::bit_cast<std::uint32_t>(seed.y);
    state.words[8u] = std::bit_cast<std::uint32_t>(seed.z);
    state.words[5u] = generation;
    const auto previous_end = state.words[3u];
    state.words[3u] = state.words[2u];
    return previous_end;
}

void D3dFaceTraversal::PushFace(
    D3dFaceTraversalState& state, const std::uint32_t face_address,
    const D3dTriangleFaceState& face,
    ID3dFaceTraversalQueueRuntime& runtime) {
    auto entries = runtime.Append(
        state, {face_address,
                static_cast<float>(D3dTriangleFace::WeightSum(face))});
    const auto candidate = entries.back();
    (void)SiftUp(entries, entries.size() - 1u, 0u, candidate);
}

std::uint32_t D3dFaceTraversal::UpdateFacePriority(
    D3dFaceTraversalState& state, const std::uint32_t face_address,
    const D3dTriangleFaceState& face,
    ID3dFaceTraversalQueueRuntime& runtime) {
    auto entries = runtime.Entries(state);
    const auto index = FindCandidate(entries, face_address);
    if (index == entries.size()) return state.words[3u];
    entries[index].priority = static_cast<float>(
        D3dTriangleFace::WeightSum(face));
    return Reheapify(entries, index,
                     static_cast<std::uint8_t>(state.words[0u] & 0xffu));
}

bool D3dFaceTraversal::HasCandidates(
    const D3dFaceTraversalState& state) {
    if (state.words[2u] == 0u) return false;
    const auto bytes = std::bit_cast<std::int32_t>(
        state.words[3u] - state.words[2u]);
    return bytes < 0 || bytes >= 8;
}

std::uint32_t D3dFaceTraversal::PopFace(
    D3dFaceTraversalState& state, D3dFaceCandidate& output,
    ID3dFaceTraversalQueueRuntime& runtime) {
    auto entries = runtime.Entries(state);
    output = entries[0u];
    RemoveRoot(entries,
               static_cast<std::uint8_t>(state.words[0u] & 0xffu));
    state.words[3u] -= 8u;
    return state.words[3u];
}

std::uint32_t D3dFaceTraversal::FindFace(
    D3dFaceTraversalState& state, const std::uint32_t face_address,
    ID3dFaceTraversalQueueRuntime& runtime) {
    const auto entries = runtime.Entries(state);
    const auto index = FindCandidate(entries, face_address);
    if (index == entries.size()) return state.words[3u];
    return state.words[2u] + static_cast<std::uint32_t>(index * 8u);
}

std::uint32_t D3dFaceTraversal::SeedVectorAddress(
    const std::uint32_t state_address) {
    return state_address + 24u;
}

std::uint32_t D3dFaceTraversal::Reheapify(
    const std::span<D3dFaceCandidate> entries, const std::size_t index,
    const std::uint8_t) {
    const auto candidate = entries[index];
    return SiftUp(entries, index, 0u, candidate);
}

void D3dFaceTraversal::RemoveRoot(
    const std::span<D3dFaceCandidate> entries, const std::uint8_t mode) {
    (void)ExtractRoot(entries, mode);
}

std::uint32_t D3dFaceTraversal::SiftUp(
    const std::span<D3dFaceCandidate> entries, std::size_t index,
    const std::size_t floor, const D3dFaceCandidate& candidate) {
    while (floor < index) {
        const auto parent = (index - 1u) / 2u;
        if (!(entries[parent].priority > candidate.priority)) break;
        entries[index] = entries[parent];
        index = parent;
    }
    entries[index] = candidate;
    return candidate.face;
}

std::uint32_t D3dFaceTraversal::ExtractRoot(
    const std::span<D3dFaceCandidate> entries, const std::uint8_t) {
    const auto candidate = entries.back();
    entries.back() = entries.front();
    return SiftDownThenUp(entries, entries.size() - 1u, candidate);
}

std::uint32_t D3dFaceTraversal::SiftDownThenUp(
    const std::span<D3dFaceCandidate> entries,
    const std::size_t active_count,
    const D3dFaceCandidate& candidate) {
    std::size_t index = 0u;
    auto right = 2u;
    while (right < active_count) {
        const auto child = entries[right].priority >
                entries[right - 1u].priority ? right - 1u : right;
        entries[index] = entries[child];
        index = child;
        right = 2u * child + 2u;
    }
    if (right == active_count) {
        entries[index] = entries[right - 1u];
        index = right - 1u;
    }
    return SiftUp(entries, index, 0u, candidate);
}

}  // namespace pal4
