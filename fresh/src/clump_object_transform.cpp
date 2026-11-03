#include "pal4/clump_object_transform.h"

#include <bit>

namespace pal4 {
namespace {

std::array<float, 3> ReadVector(IClumpObjectTransformRuntime& runtime,
                                const std::uint32_t address) {
    return {std::bit_cast<float>(runtime.ReadWord(address)),
            std::bit_cast<float>(runtime.ReadWord(address + 4u)),
            std::bit_cast<float>(runtime.ReadWord(address + 8u))};
}

}  // namespace

std::uint32_t ClumpObject::SetPosition(
    const ClumpObjectState& state, const std::array<float, 3>& value,
    const bool relative, IClumpObjectTransformRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    const auto manager = state.words[18u];
    auto position = ReadVector(runtime, manager + 172u);
    for (std::size_t i = 0u; i < position.size(); ++i)
        position[i] = relative ? position[i] + value[i] : value[i];
    return runtime.Translate(manager, position, 0u);
}

std::uint8_t ClumpObject::SetRotation(
    const ClumpObjectState& state, const std::array<float, 3>& value,
    const bool relative, IClumpObjectTransformRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    return runtime.Rotate(state.words[18u], value, relative ? 1u : 0u);
}

std::uint32_t ClumpObject::SetScale(
    const ClumpObjectState& state, const std::array<float, 3>& value,
    const bool relative, IClumpObjectTransformRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    const auto manager = state.words[18u];
    auto scale = ReadVector(runtime, manager + 184u);
    for (std::size_t i = 0u; i < scale.size(); ++i) {
        scale[i] = relative ? scale[i] + value[i] : value[i];
        if (scale[i] < 0.0f)
            scale[i] = 0.0f;
    }
    return runtime.Scale(manager, scale, 0u);
}

}  // namespace pal4
