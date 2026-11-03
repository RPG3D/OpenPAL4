#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatResultSoundRuntime {
public:
    virtual ~ICombatResultSoundRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void PlaySound(std::string_view name,
                           std::uint32_t mode) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
};

class CombatResultSoundElement final {
public:
    // IDA ui_combatResultIcon_sound_show at 0x5488e0.
    [[nodiscard]] static std::uint8_t Show(
        std::uint32_t element, ICombatResultSoundRuntime& runtime);
};

}  // namespace pal4
