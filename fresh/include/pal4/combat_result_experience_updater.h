#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatResultExperienceRuntime {
public:
    virtual ~ICombatResultExperienceRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteInt(std::uint32_t address, std::int32_t value) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::int32_t ActorMetric(
        std::uint32_t actor, std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t ActorId(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t NextLevelExperience(
        std::uint32_t actor_id, std::int32_t level) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(double value) = 0;
    virtual void SetWindowText(std::uint32_t window,
                               std::string_view text) = 0;
    virtual void ApplyLevelUp(std::uint32_t row, std::int32_t level,
                              std::uint32_t actor, std::uint32_t hint,
                              std::uint32_t experience_record) = 0;
};

class CombatResultExperienceUpdater final {
public:
    // IDA CombatResultExperience_Update at 0x548ce0.
    [[nodiscard]] static std::uint8_t Update(
        std::uint32_t element, float elapsed,
        ICombatResultExperienceRuntime& runtime);

    // IDA CombatResultExperience_Complete at 0x548fb0.
    [[nodiscard]] static std::uint8_t Complete(
        std::uint32_t element, ICombatResultExperienceRuntime& runtime);
};

}  // namespace pal4
