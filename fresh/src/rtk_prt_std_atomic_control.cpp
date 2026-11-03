#include "pal4/rtk_prt_std_atomic_control.h"
#include "pal4/effect_color_math.h"

#include <stdexcept>
#include <utility>

namespace pal4 {
namespace {

std::uint8_t Byte(const RtkPrtStdAtomicObjectState& state,
                  const std::uint32_t offset) {
    return static_cast<std::uint8_t>(state.words[offset / 4u] >>
                                     ((offset % 4u) * 8u));
}

void StoreByte(RtkPrtStdAtomicObjectState& state,
               const std::uint32_t offset, const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

}  // namespace

std::uint8_t RtkPrtStdAtomicControl::Reset(
    RtkPrtStdAtomicObjectState& state, const std::uint32_t owner,
    IRtkPrtStdAtomicResetRuntime& runtime) {
    if (Byte(state, 4u) == 0u)
        return 0u;
    runtime.ResetPlaybackFacet(owner + 20u);
    const auto resource = state.words[23u];
    for (const auto& [resource_offset, cache_offset] : {
             std::pair{100u, 112u}, std::pair{88u, 100u},
             std::pair{112u, 124u}}) {
        for (std::uint32_t i = 0u; i != 3u; ++i)
            runtime.WriteWord(resource + resource_offset + i * 4u,
                              state.words[(cache_offset + i * 4u) / 4u]);
        runtime.UpdateFrame(resource);
    }
    const auto packed = state.words[24u];
    for (std::uint32_t i = 0u; i != 4u; ++i)
        runtime.WriteFloat(resource + 212u + i * 4u,
                           NormalizeEffectColorByte(static_cast<std::uint8_t>(packed >> (i * 8u))));
    runtime.DeactivateResource(resource);
    StoreByte(state, 11u, 0u);
    return 1u;
}

void RtkPrtStdAtomicControl::ReleasePlayback(
    RtkPrtStdAtomicObjectState& state, const std::uint32_t owner,
    IRtkPrtStdAtomicReleaseRuntime& runtime) {
    if (Byte(state, 4u) != 0u) {
        StoreByte(state, 4u, 0u);
        StoreByte(state, 6u, 0u);
        StoreByte(state, 7u, 0u);
        const auto resource = state.words[23u];
        state.words[15u] = 0u;
        state.words[16u] = 0x3f800000u;
        state.words[17u] = 0x3f800000u;
        state.words[18u] = 0u;
        state.words[19u] = 0u;
        state.words[20u] = 0u;
        StoreByte(state, 84u, 0u);
        state.words[22u] = 0u;
        if (resource != 0u) {
            runtime.DestroyResource(resource);
            runtime.FreeResource(resource);
            state.words[23u] = 0u;
        }
        runtime.ProcessEvent(state.words[4u], owner);
    }
}

std::uint32_t RtkPrtStdAtomicControl::Stop(
    RtkPrtStdAtomicObjectState& state, const std::uint32_t owner,
    IRtkPrtStdAtomicControlRuntime& runtime) {
    ReleasePlayback(state, owner, runtime);
    return runtime.DeleteObject(owner, 1u);
}

std::uint8_t RtkPrtStdAtomicControl::Advance(
    const RtkPrtStdAtomicObjectState& state, const double elapsed,
    IRtkPrtStdAtomicUpdateRuntime& runtime) {
    const auto active = Byte(state, 4u);
    if (active == 0u)
        return active;
    const auto running = Byte(state, 6u);
    if (running == 0u)
        return running;
    return runtime.UpdateState(
        state.words[23u], elapsed, runtime.ReadWord(state.words[4u] + 48u), Byte(state, 11u));
}

std::uint8_t RtkPrtStdAtomicControl::ActivatePending(
    const RtkPrtStdAtomicObjectState& state, const std::uint32_t context,
    IRtkPrtStdAtomicDrawRuntime& runtime) {
    const auto active = Byte(state, 4u);
    if (active == 0u)
        return active;
    const auto pending = Byte(state, 7u);
    if (pending == 0u)
        return pending;
    const auto child = state.words[3u];
    const auto resource = state.words[23u];
    if (child == 0u || runtime.ReadByte(child + 83u) == 0u)
        return runtime.HandleStateChange(resource, context);
    const auto object = runtime.ReadWord(resource + 44u);
    const auto transform = runtime.GetObjectTransform(object) + 48u;
    const auto manager = runtime.EffectManager();
    if (manager == 0u) {
        throw std::runtime_error(
            "\xb5\xa5\xbc\xfe\xb6\xd4\xcf\xf3\xce\xb4\xb1\xbb"
            "\xb4\xb4\xbd\xa8\xa3\xac\xc7\xeb\xd4\xda\xca\xb9"
            "\xd3\xc3\xd6\xae\xc7\xb0\xb4\xb4\xbd\xa8\xa3\xa1");
    }
    const auto valid = runtime.ValidateTransform(manager, context, transform);
    if (valid != 0u)
        return runtime.HandleStateChange(resource, context);
    return valid;
}

std::uint8_t RtkPrtStdAtomicControl::EnablePlayback(
    RtkPrtStdAtomicObjectState& state) {
    return EnablePlaybackStatus(state.words[1u]);
}

std::uint8_t RtkPrtStdAtomicControl::EnablePlaybackStatus(std::uint32_t& status) {
    const auto active = static_cast<std::uint8_t>(status);
    if (active == 0u)
        return active;
    const auto running = static_cast<std::uint8_t>(status >> 16u);
    if (running != 0u)
        return running;
    status = (status & 0x0000ffffu) | 0x01010000u;
    return 1u;
}

std::uint8_t RtkPrtStdAtomicControl::DisablePlayback(
    RtkPrtStdAtomicObjectState& state) {
    if (Byte(state, 4u) != 0u && Byte(state, 6u) != 0u) {
        StoreByte(state, 6u, 0u);
        StoreByte(state, 7u, 0u);
    }
    return 0u;
}

}  // namespace pal4
