#include "pal4/combat_result_audio.h"

namespace pal4 {

std::uint8_t CombatResultAudio::PlayTransition(
    ICombatResultAudioRuntime& runtime) {
    (void)runtime.PlaySound("WA45.wav", 1u);
    return 1u;
}

std::uint8_t CombatResultAudio::PlayReward(
    const std::uint32_t element, ICombatResultAudioRuntime& runtime) {
    const auto handle = runtime.PlaySound("WA47.wav", 0u);
    const auto destination = runtime.ReadWord(element + 92u);
    runtime.WriteWord(destination, handle);
    return 1u;
}

std::uint8_t CombatResultAudio::StopStored(
    const std::uint32_t element, ICombatResultAudioRuntime& runtime) {
    const auto handle = runtime.ReadWord(element + 92u);
    runtime.StopStoredHandle(handle);
    return 1u;
}

}  // namespace pal4
