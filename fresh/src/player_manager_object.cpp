#include "pal4/player_manager_object.h"

namespace pal4 {

void PlayerManagerVectorRuntimeStub::ConstructVector(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t) {}
void PlayerManagerVectorRuntimeStub::DestroyVector(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t) {}
void PlayerManagerCallbackRuntimeStub::Invoke(
    std::uint32_t, std::uint32_t, std::uint32_t) {}

std::uint32_t PlayerManagerObject::Construct(
    PlayerManagerObjectState& state, std::uint32_t object,
    IPlayerManagerVectorRuntime& runtime) {
    runtime.ConstructVector(object, kElementBytes, kElementCount,
                            kConstructor, kDestructor);
    state.Word(0x2c50u / 4u) = 0u;
    return object;
}

void PlayerManagerObject::Destroy(
    std::uint32_t object, IPlayerManagerVectorRuntime& runtime) {
    runtime.DestroyVector(object, kElementBytes,
                          kElementCount, kDestructor);
}

std::uint32_t PlayerManagerObject::GetPlayerByIndex(
    const std::uint32_t object, const std::int32_t index) noexcept {
    if (index >= static_cast<std::int32_t>(kElementCount)) return 0u;
    return object + static_cast<std::uint32_t>(index) * kElementBytes;
}

std::uint32_t PlayerManagerObject::GetActivePlayerCount(
    const PlayerManagerObjectState& state) noexcept {
    std::uint32_t count = 0u;
    for (std::uint32_t index = 0u; index < kElementCount; ++index)
        count += state.Word((index * kElementBytes + 0xb08u) / 4u) != 0u;
    return count;
}

std::uint32_t PlayerManagerObject::SetPlayerState(
    PlayerManagerObjectState& state, const std::uint32_t value) noexcept {
    for (std::uint32_t index = 0u; index < kElementCount; ++index)
        state.Word((index * kElementBytes + 0xb0cu) / 4u) = 0u;
    const auto word = state.Word(0x2c50u / 4u) * (kElementBytes / 4u);
    state.Word(word + 0xb0cu / 4u) = value;
    return word;
}

void PlayerManagerObject::ForAllPlayers(
    const std::uint32_t manager, const std::uint32_t callback,
    const std::uint32_t context, IPlayerManagerCallbackRuntime& runtime) {
    for (std::uint32_t index = 0u; index < kElementCount; ++index)
        runtime.Invoke(callback, manager + index * kElementBytes, context);
}

}  // namespace pal4
