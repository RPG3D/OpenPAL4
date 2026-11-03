#include "pal4/actor_manager_player_state_records.h"

namespace pal4 {

std::uint32_t ActorManagerPlayerStateRecords::Update(
    const std::uint32_t actor,
    IActorManagerPlayerStateUpdateRuntime& runtime) {
    const auto saved = runtime.ReadWord(actor + 1192u);
    const auto state = runtime.ActorSlot52(actor, 4u);
    return runtime.UpdateStateAndProcess(actor, state, saved);
}

std::uint32_t ActorManagerPlayerStateRecords::BuildMatching(
    const std::uint32_t actor, const std::uint32_t output,
    IActorManagerPlayerStateLearningRuntime& runtime) {
    const auto saved = runtime.ReadWord(actor + 1192u);
    const auto state = runtime.ActorSlot52(actor, 4u);
    runtime.BuildMatchingRecords(actor + 920u, output, state, saved);
    return output;
}

}  // namespace pal4
