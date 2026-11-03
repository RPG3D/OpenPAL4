#include "pal4/clump_object_uv.h"

namespace pal4 {
namespace {

std::uint32_t MatrixWord(const ClumpUvFacetKind kind,
                         const std::uint32_t index) {
    return (kind == ClumpUvFacetKind::Rotation ? 176u : 184u) / 4u + index;
}

std::uint32_t CountWord(const ClumpUvFacetKind kind,
                        const std::uint32_t index) {
    return (kind == ClumpUvFacetKind::Rotation ? 192u : 200u) / 4u + index;
}

bool Active(const ClumpObjectState& state) {
    return (state.words[1u] & 0xffu) != 0u;
}

}  // namespace

std::uint8_t ClumpObjectUvFacet::Acquire(
    ClumpObjectState& state, const ClumpUvFacetKind kind,
    const std::uint32_t index, IClumpObjectUvRuntime& runtime) {
    if (!Active(state))
        return 0u;

    auto& count = state.words[CountWord(kind, index)];
    if (count == 0u) {
        const auto matrix = runtime.CreateMatrix();
        state.words[MatrixWord(kind, index)] = matrix;
        const auto clump = runtime.ReadWord(state.words[18u] + 20u);
        runtime.ApplyToAtomics(clump, index != 0u, matrix);
    }
    ++count;
    return static_cast<std::uint8_t>(count);
}

std::uint8_t ClumpObjectUvFacet::Release(
    ClumpObjectState& state, const ClumpUvFacetKind kind,
    const std::uint32_t index, IClumpObjectUvRuntime& runtime) {
    if (!Active(state))
        return 0u;

    auto& count = state.words[CountWord(kind, index)];
    --count;
    if (count == 0u) {
        const auto result = runtime.DestroyMatrix(
            state.words[MatrixWord(kind, index)]);
        state.words[MatrixWord(kind, index)] = 0u;
        return result;
    }
    return static_cast<std::uint8_t>(count);
}

std::uint32_t ClumpObjectUvFacet::Rotate(
    const ClumpObjectState& state, const std::uint32_t index,
    const std::array<float, 3>& axis, const float angle,
    IClumpObjectUvRuntime& runtime) {
    if (!Active(state))
        return 0u;
    return runtime.RotateMatrix(
        state.words[MatrixWord(ClumpUvFacetKind::Rotation, index)],
        axis, angle, 1u);
}

std::uint8_t ClumpObjectUvFacet::Translate(
    const ClumpObjectState& state, const std::uint32_t index,
    const std::array<float, 3>& vector, IClumpObjectUvRuntime& runtime) {
    if (!Active(state))
        return 0u;
    return runtime.TranslateMatrix(
        state.words[MatrixWord(ClumpUvFacetKind::Translation, index)],
        vector, 1u);
}

}  // namespace pal4
