#include "pal4/rtk_prt_std_atomic_task_facets.h"

#include <bit>

namespace pal4 {
namespace {

std::uint8_t Active(const RtkPrtStdAtomicObjectState& state) {
    return static_cast<std::uint8_t>(state.words[1u]);
}

}  // namespace

std::uint8_t RtkPrtStdAtomicTaskFacets::SetPositionWords(
    const RtkPrtStdAtomicObjectState& state,
    const std::array<std::uint32_t, 3>& words,
    IRtkPrtStdAtomicTaskFacetRuntime& runtime) {
    if (Active(state) == 0u)
        return 0u;
    const auto resource = state.words[23u];
    for (std::uint32_t i = 0u; i != 3u; ++i)
        runtime.WriteWord(resource + 112u + i * 4u, words[i]);
    return runtime.UpdateFrame(resource);
}

std::uint8_t RtkPrtStdAtomicTaskFacets::DisableResourceFlag(
    const RtkPrtStdAtomicObjectState& state,
    IRtkPrtStdAtomicTaskFacetRuntime& runtime) {
    if (Active(state) == 0u)
        return 0u;
    const auto resource = state.words[23u];
    if (runtime.ReadByte(resource + 1u) != 0u)
        runtime.WriteByte(resource + 1u, 0u);
    return static_cast<std::uint8_t>(resource);
}

std::uint32_t RtkPrtStdAtomicTaskFacets::SetEmissionTime(
    const RtkPrtStdAtomicObjectState& state, const float interval,
    const std::uint32_t mode, const std::uint32_t entry_eax,
    IRtkPrtStdAtomicTaskFacetRuntime& runtime) {
    if (Active(state) == 0u)
        return entry_eax & 0xffffff00u;
    const auto resource = state.words[23u];
    if ((mode & 0xffu) != 0u) {
        const auto emitted = runtime.ReadWord(resource + 208u);
        const auto remaining = runtime.ReadWord(resource + 200u) - emitted;
        runtime.WriteByte(resource + 5u, 0u);
        runtime.WriteWord(resource + 8u, 0u);
        runtime.WriteByte(resource + 16u, 1u);
        runtime.WriteWord(resource + 24u, emitted);
        const auto count = runtime.FloatToInt(
            static_cast<double>(static_cast<std::int32_t>(remaining)) / interval);
        runtime.WriteWord(resource + 20u, static_cast<std::uint32_t>(count));
        runtime.WriteWord(resource + 32u, 0u);
        runtime.WriteWord(resource + 36u, 0u);
        return static_cast<std::uint32_t>(count);
    }
    const auto maximum = runtime.ReadWord(resource + 200u);
    runtime.WriteByte(resource + 16u, 0u);
    runtime.WriteWord(resource + 20u, 0u);
    runtime.WriteByte(resource + 5u, 1u);
    runtime.WriteWord(resource + 12u, maximum);
    const auto count = runtime.FloatToInt(
        static_cast<double>(static_cast<std::int32_t>(maximum)) / interval);
    runtime.WriteWord(resource + 8u, static_cast<std::uint32_t>(count));
    return static_cast<std::uint32_t>(count);
}

std::uint8_t RtkPrtStdAtomicTaskFacets::GetPositionWords(
    const RtkPrtStdAtomicObjectState& state,
    std::array<std::uint32_t, 3>& output,
    IRtkPrtStdAtomicTaskFacetRuntime& runtime) {
    if (Active(state) == 0u)
        return 0u;
    const auto resource = state.words[23u];
    for (std::uint32_t i = 0u; i != 3u; ++i)
        output[i] = runtime.ReadWord(resource + 112u + i * 4u);
    return static_cast<std::uint8_t>(output[2u]);
}

std::uint8_t RtkPrtStdAtomicTaskFacets::SetTargetWords(
    RtkPrtStdAtomicObjectState& state,
    const std::array<std::uint32_t, 3>& words) {
    if (Active(state) == 0u)
        return 0u;
    for (std::uint32_t i = 0u; i != 3u; ++i)
        state.words[15u + i] = words[i];
    return static_cast<std::uint8_t>(words[1u]);
}

std::uint8_t RtkPrtStdAtomicTaskFacets::SetPosition(
    const RtkPrtStdAtomicObjectState& state,
    std::array<float, 3> values, const bool relative,
    IRtkPrtStdAtomicTaskFacetRuntime& runtime) {
    if (Active(state) == 0u)
        return 0u;
    const auto resource = state.words[23u];
    if (relative) {
        for (std::uint32_t i = 0u; i != 3u; ++i)
            values[i] += std::bit_cast<float>(
                runtime.ReadWord(resource + 112u + i * 4u));
    }
    for (std::uint32_t i = 0u; i != 3u; ++i)
        runtime.WriteWord(resource + 112u + i * 4u,
                          std::bit_cast<std::uint32_t>(values[i]));
    return runtime.UpdateFrame(resource);
}

}  // namespace pal4
