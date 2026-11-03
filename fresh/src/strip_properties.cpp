#include "pal4/strip_properties.h"

#include <bit>

namespace pal4 {
namespace {

std::uint8_t Active(const StripObjectState& state) {
    return static_cast<std::uint8_t>(state.words[1u] & 0xffu);
}

std::uint8_t Get(const std::span<const std::uint32_t> words,
                 const std::uint32_t offset,
                 std::array<std::uint32_t, 3>& output) {
    const auto active = static_cast<std::uint8_t>(words[1u]);
    if (active == 0u)
        return active;
    for (std::uint32_t i = 0u; i < 3u; ++i)
        output[i] = words[offset / 4u + i];
    return static_cast<std::uint8_t>(output[2u]);
}

std::uint32_t Set(StripObjectState& state, const std::uint32_t offset,
                  const StripTransformKind kind,
                  const std::array<float, 3>& values, const bool relative,
                  IStripPropertyRuntime& runtime) {
    const auto active = Active(state);
    if (active == 0u)
        return active;
    std::array<float, 3> result{};
    for (std::uint32_t i = 0u; i < 3u; ++i) {
        const auto previous = std::bit_cast<float>(state.words[offset / 4u + i]);
        result[i] = relative ? previous + values[i] : values[i];
        state.words[offset / 4u + i] = std::bit_cast<std::uint32_t>(result[i]);
    }
    if (((state.words[35u] & 0xffu) != 0u))
        return runtime.UpdateStraight(state.words[38u], kind, result);
    return runtime.UpdateFrame(state);
}

}  // namespace

std::uint8_t StripProperties::GetRotation(
    const StripObjectState& state, std::array<std::uint32_t, 3>& output) {
    return GetRotation(std::span(state.words).first<22u>(),output);
}
std::uint8_t StripProperties::GetRotation(
    const std::span<const std::uint32_t,22u> prefix,
    std::array<std::uint32_t,3>& output) {
    return Get(prefix,76u,output);
}

std::uint8_t StripProperties::GetScale(
    const StripObjectState& state, std::array<std::uint32_t, 3>& output) {
    return GetScale(std::span(state.words).first<19u>(),output);
}
std::uint8_t StripProperties::GetScale(
    const std::span<const std::uint32_t,19u> prefix,
    std::array<std::uint32_t,3>& output) {
    return Get(prefix,64u,output);
}

std::uint8_t StripProperties::GetPosition(
    const StripObjectState& state, std::array<std::uint32_t, 3>& output) {
    return Get(state.words, 88u, output);
}

std::uint32_t StripProperties::SetScale(
    StripObjectState& state, const std::array<float, 3>& values,
    const bool relative, IStripPropertyRuntime& runtime) {
    return Set(state, 64u, StripTransformKind::Scale,
               values, relative, runtime);
}

std::uint32_t StripProperties::SetRotation(
    StripObjectState& state, const std::array<float, 3>& values,
    const bool relative, IStripPropertyRuntime& runtime) {
    return Set(state, 76u, StripTransformKind::Rotation,
               values, relative, runtime);
}

std::uint32_t StripProperties::SetPosition(
    StripObjectState& state, const std::array<float, 3>& values,
    const bool relative, IStripPropertyRuntime& runtime) {
    return Set(state, 88u, StripTransformKind::Position,
               values, relative, runtime);
}

}  // namespace pal4
