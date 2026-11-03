#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "pal4/d3d_triangle_face.h"

namespace pal4 {

struct D3dFaceTraversalState final {
    std::array<std::uint32_t, 9u> words{};
};

static_assert(sizeof(D3dFaceTraversalState) == 36);

struct D3dFaceCandidate final {
    std::uint32_t face = 0u;
    float priority = 0.0f;
};

static_assert(sizeof(D3dFaceCandidate) == 8);

class ID3dFaceTraversalQueueRuntime {
public:
    virtual ~ID3dFaceTraversalQueueRuntime() = default;
    [[nodiscard]] virtual std::span<D3dFaceCandidate> Append(
        D3dFaceTraversalState& state,
        const D3dFaceCandidate& candidate) = 0;
    [[nodiscard]] virtual std::span<D3dFaceCandidate> Entries(
        D3dFaceTraversalState& state) = 0;
};

class D3dFaceTraversal final {
public:
    // IDA 0x602980.
    [[nodiscard]] static std::uint32_t ResetSeed(
        D3dFaceTraversalState& state, std::uint32_t generation,
        const D3dTriangleVector3& seed);
    // IDA 0x6029c0.
    static void PushFace(D3dFaceTraversalState& state,
                         std::uint32_t face_address,
                         const D3dTriangleFaceState& face,
                         ID3dFaceTraversalQueueRuntime& runtime);
    // IDA 0x602a80.
    [[nodiscard]] static std::uint32_t UpdateFacePriority(
        D3dFaceTraversalState& state, std::uint32_t face_address,
        const D3dTriangleFaceState& face,
        ID3dFaceTraversalQueueRuntime& runtime);
    // IDA 0x602ad0.
    [[nodiscard]] static bool HasCandidates(
        const D3dFaceTraversalState& state);
    // IDA 0x602af0.
    [[nodiscard]] static std::uint32_t PopFace(
        D3dFaceTraversalState& state, D3dFaceCandidate& output,
        ID3dFaceTraversalQueueRuntime& runtime);
    // IDA 0x602b50/0x602b70.
    [[nodiscard]] static std::uint32_t FindFace(
        D3dFaceTraversalState& state, std::uint32_t face_address,
        ID3dFaceTraversalQueueRuntime& runtime);
    [[nodiscard]] static std::uint32_t SeedVectorAddress(
        std::uint32_t state_address);
    // IDA 0x602b80/0x602bb0/0x602c10/0x602c70/0x602cb0.
    [[nodiscard]] static std::uint32_t Reheapify(
        std::span<D3dFaceCandidate> entries, std::size_t index,
        std::uint8_t mode);
    static void RemoveRoot(std::span<D3dFaceCandidate> entries,
                           std::uint8_t mode);
    [[nodiscard]] static std::uint32_t SiftUp(
        std::span<D3dFaceCandidate> entries, std::size_t index,
        std::size_t floor, const D3dFaceCandidate& candidate);
    [[nodiscard]] static std::uint32_t ExtractRoot(
        std::span<D3dFaceCandidate> entries, std::uint8_t mode);
    [[nodiscard]] static std::uint32_t SiftDownThenUp(
        std::span<D3dFaceCandidate> entries, std::size_t active_count,
        const D3dFaceCandidate& candidate);
};

}  // namespace pal4
