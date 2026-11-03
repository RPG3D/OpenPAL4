#include "pal4/dynamic_strip_object_update.h"

#include <bit>

namespace pal4 {

namespace {

std::uint8_t ReadByte(const StripVisualBlockView& state,
                      std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

void WriteByte(StripVisualBlockView state,
               std::uint32_t offset, std::uint8_t value) {
    auto& word = state.words[offset / 4u];
    const auto shift = 8u * (offset % 4u);
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

float ReadFloat(const StripVisualBlockView& state,
                std::uint32_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}

void WriteFloat(StripVisualBlockView state,
                std::uint32_t offset, float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}

std::array<float, 3u> ReadPosition(
    const StripVisualBlockView& state, std::uint32_t offset) {
    return {ReadFloat(state, offset), ReadFloat(state, offset + 4u),
            ReadFloat(state, offset + 8u)};
}

void WritePosition(StripVisualBlockView state,
                   std::uint32_t offset,
                   const std::array<float, 3u>& position) {
    for (std::uint32_t i = 0u; i < 3u; ++i)
        WriteFloat(state, offset + i * 4u, position[i]);
}

bool Moved(const std::array<float, 3u>& current,
           const std::array<float, 3u>& previous) {
    constexpr double epsilon =
        static_cast<double>(std::bit_cast<float>(0x38d1b717u));
    for (std::uint32_t i = 0u; i < 3u; ++i) {
        if (static_cast<double>(current[i]) + epsilon < previous[i] ||
            static_cast<double>(current[i]) - epsilon >= previous[i])
            return true;
    }
    return false;
}

}  // namespace

std::uint8_t DynamicStripObjectUpdater::UpdateVisual(
    StripVisualBlockView state, double frame_time,
    std::uint8_t mode, IDynamicStripVisualUpdateRuntime& runtime) {
    if (ReadByte(state, 0u) == 0u) return 0u;

    const auto sentinel = state.words[28u];
    if (ReadByte(state, 2u) != 0u) {
        WriteByte(state, 2u, 0u);
        const auto first = runtime.FramePosition(state.words[25u]);
        const auto second = runtime.FramePosition(state.words[26u]);
        WritePosition(state, 76u, first);
        WritePosition(state, 88u, second);
        runtime.InsertFront(sentinel, {first, second,
                                       ReadFloat(state, 60u)});
        ++state.words[29u];
    }

    for (auto node = runtime.FirstNode(sentinel); node != sentinel;) {
        const auto next = runtime.NextNode(node);
        const double remaining =
            static_cast<double>(runtime.RemainingLife(node)) - frame_time;
        runtime.SetRemainingLife(node, static_cast<float>(remaining));
        if (remaining <= 0.0) {
            runtime.RemoveNode(node);
            --state.words[29u];
        }
        node = next;
    }

    const auto interval = ReadFloat(state, 8u);
    if (interval > 0.0f) {
        const double elapsed =
            static_cast<double>(ReadFloat(state, 12u)) + frame_time;
        WriteFloat(state, 12u, static_cast<float>(elapsed));
        if (elapsed < interval) return 0u;
        WriteFloat(state, 12u, 0.0f);
    }
    if (mode != 0u) return 0u;

    const auto first = runtime.FramePosition(state.words[25u]);
    const auto second = runtime.FramePosition(state.words[26u]);
    if (!Moved(first, ReadPosition(state, 76u)) &&
        !Moved(second, ReadPosition(state, 88u)))
        return 0u;
    runtime.InsertFront(sentinel, {first, second,
                                   ReadFloat(state, 60u)});
    ++state.words[29u];
    WritePosition(state, 76u, first);
    WritePosition(state, 88u, second);
    return 1u;
}

std::uint8_t DynamicStripObjectUpdater::UpdateVisual(
    DynamicStripObjectState& state, double frame_time, std::uint8_t mode,
    IDynamicStripVisualUpdateRuntime& runtime) {
    return UpdateVisual(StripVisualBlockView{std::span{state.words}.first<34u>()},
                        frame_time, mode, runtime);
}

}  // namespace pal4
