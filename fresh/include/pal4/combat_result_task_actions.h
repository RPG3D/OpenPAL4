#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatResultTaskRuntime {
public:
    virtual ~ICombatResultTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void PlaySound(std::string_view name,
                           std::uint32_t mode) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void EndCombatSequence(std::uint32_t combat_result) = 0;
};

class CombatResultTaskActions final {
public:
    // IDA CombatResultTask_SetState at 0x549270.
    [[nodiscard]] static std::uint8_t SetState(
        std::uint32_t element, ICombatResultTaskRuntime& runtime);
    // IDA CombatResultTask_ShowPanel at 0x5492d0.
    [[nodiscard]] static std::uint8_t ShowPanel(
        std::uint32_t element, ICombatResultTaskRuntime& runtime);
    // IDA CombatResultTask_FinishCombat at 0x549420.
    [[nodiscard]] static std::uint8_t FinishCombat(
        std::uint32_t element, ICombatResultTaskRuntime& runtime);
};

}  // namespace pal4
