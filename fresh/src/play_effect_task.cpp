#include "pal4/play_effect_task.h"

namespace pal4 {
namespace {

void WriteByte(PlayEffectTaskState& state, const std::size_t offset,
               const std::uint8_t value) {
    const auto index = offset / 4u;
    const auto shift = static_cast<unsigned>((offset % 4u) * 8u);
    const auto mask = 0xffu << shift;
    state.words[index] = (state.words[index] & ~mask) |
                         (static_cast<std::uint32_t>(value) << shift);
}

std::uint8_t ReadByte(const PlayEffectTaskState& state,
                      const std::size_t offset) {
    return static_cast<std::uint8_t>(state.words[offset / 4u] >>
                                     ((offset % 4u) * 8u));
}

}  // namespace

PlayEffectTaskState& PlayEffectTask::Construct(
    PlayEffectTaskState& state, IPlayEffectTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[19u] = 0u;
    state.words[0u] = kVtable;
    return state;
}

PlayEffectTaskState& PlayEffectTask::Destroy(
    PlayEffectTaskState& state, IPlayEffectTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

std::string_view PlayEffectTask::ClassName() {
    return "PlayEffectTask";
}

bool PlayEffectTask::LoadConfig(
    PlayEffectTaskState& state, const std::uint32_t config,
    IPlayEffectConfigRuntime& runtime) {
    if (config == 0u)
        return false;
    const auto loop = runtime.ReadInt(config, "Attr_Task_PlayEffect_Loop", 0);
    WriteByte(state, 72u, static_cast<std::uint8_t>(loop > 0));
    const auto inherit = runtime.ReadInt(config, "Attr_Inherit_Pre_Task", 0);
    WriteByte(state, 61u, static_cast<std::uint8_t>(inherit > 0));
    return true;
}

std::uint8_t PlayEffectTask::Start(
    PlayEffectTaskState& state, IPlayEffectTaskRuntime& runtime) {
    const auto facet = runtime.ResolveFacet(state.words[16u], ClassName());
    state.words[19u] = facet;
    runtime.SetLoop(facet, ReadByte(state, 72u));
    return 1u;
}

std::uint8_t PlayEffectTask::Reset(PlayEffectTaskState& state) {
    state.words[19u] = 0u;
    return 1u;
}

}  // namespace pal4
