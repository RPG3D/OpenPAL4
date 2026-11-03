#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatResultAudioRuntime {
public:
    virtual ~ICombatResultAudioRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlaySound(
        std::string_view name, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void StopStoredHandle(std::uint32_t handle_address) = 0;
};

class CombatResultAudio final {
public:
    // IDA CombatResultAudio_PlayTransition at 0x548a30.
    [[nodiscard]] static std::uint8_t PlayTransition(
        ICombatResultAudioRuntime& runtime);

    // IDA CombatResultAudio_PlayReward at 0x548be0.
    [[nodiscard]] static std::uint8_t PlayReward(
        std::uint32_t element, ICombatResultAudioRuntime& runtime);

    // IDA CombatResultAudio_StopStored at 0x548cd0.
    [[nodiscard]] static std::uint8_t StopStored(
        std::uint32_t element, ICombatResultAudioRuntime& runtime);
};

}  // namespace pal4
