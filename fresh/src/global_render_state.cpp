#include "pal4/global_render_state.h"

#include <cassert>
#include <bit>
#include <cmath>

namespace pal4 {

std::uint32_t GlobalRenderState::ReadDword(
    const std::uint32_t offset) const {
    assert(offset % 4u == 0u && offset / 4u < words_.size());
    return words_[offset / 4u];
}

void GlobalRenderState::WriteDword(const std::uint32_t offset,
                                    const std::uint32_t value) {
    assert(offset % 4u == 0u && offset / 4u < words_.size());
    words_[offset / 4u] = value;
}

std::uint32_t GlobalRenderState::Initialize(
    IGlobalRenderStateRuntime& runtime) {
    words_.fill(0u);
    words_[0] = kVtableAddress;
    return InitializeDefaultBasis(runtime);
}

std::uint32_t GlobalRenderState::InitializeDefaultBasis(
    IGlobalRenderStateRuntime& runtime) {
    words_[7] = 0u;
    words_[8] = 0u;
    words_[9] = 0x3f800000u;
    const auto direction = runtime.TransformDirection(
        *this, {words_[7], words_[8], words_[9]});
    words_[1] = direction[0];
    words_[2] = direction[1];
    words_[3] = direction[2];
    words_[4] = 0u;
    words_[5] = 0x3f800000u;
    words_[6] = 0u;
    words_[10] = 0u;
    words_[11] = 0u;
    words_[12] = 0u;
    return direction[2];
}

std::array<std::uint32_t, 3>& GlobalRenderState::SetBasisY(
    std::array<std::uint32_t, 3>& output,
    const std::array<std::uint32_t, 3>& values) {
    for (std::uint32_t i = 0u; i < 3u; ++i) words_[4u + i] = values[i];
    output = values;
    return output;
}

std::array<std::uint32_t, 3>& GlobalRenderState::SetBasisZ(
    std::array<std::uint32_t, 3>& output,
    const std::array<std::uint32_t, 3>& values) {
    for (std::uint32_t i = 0u; i < 3u; ++i) words_[7u + i] = values[i];
    output = values;
    return output;
}

std::array<float, 3>& GlobalRenderState::TransformVector(
    std::array<float, 3>& output,
    const std::array<float, 3>& input) const {
    const auto values = input;
    for (std::uint32_t row = 0u; row < 3u; ++row) {
        const auto base = 1u + row * 3u;
        const auto x = std::bit_cast<float>(words_[base]);
        const auto y = std::bit_cast<float>(words_[base + 1u]);
        const auto z = std::bit_cast<float>(words_[base + 2u]);
        output[row] = static_cast<float>(
            static_cast<double>(z) * values[2] +
            static_cast<double>(y) * values[1] +
            static_cast<double>(x) * values[0]);
    }
    return output;
}

std::uint32_t GlobalRenderState::SetNormalizedDirection(
    const std::array<float, 3>& direction,
    const std::array<std::uint32_t, 3>& basis_y,
    IGlobalRenderStateRuntime& runtime) {
    for (std::uint32_t i = 0u; i < 3u; ++i) words_[4u + i] = basis_y[i];
    const auto length = std::sqrt(
        static_cast<double>(direction[0]) * direction[0] +
        static_cast<double>(direction[1]) * direction[1] +
        static_cast<double>(direction[2]) * direction[2]);
    auto normalized = direction;
    if (length > 0.0) {
        const auto inverse = 1.0 / length;
        for (auto& component : normalized)
            component = static_cast<float>(inverse * component);
    }
    return runtime.CommitDirection(*this, normalized);
}

void GlobalRenderState::InitializeGlobalAngle(float& storage) {
    constexpr float first = 1.5707963705062866f;
    constexpr float second = 3.1415927410125732f;
    storage = first + second;
}

double Vector3Math::Dot(const std::array<float, 3>& left,
                        const std::array<float, 3>& right) {
    return static_cast<double>(left[2]) * right[2] +
           static_cast<double>(left[1]) * right[1] +
           static_cast<double>(left[0]) * right[0];
}

}  // namespace pal4
