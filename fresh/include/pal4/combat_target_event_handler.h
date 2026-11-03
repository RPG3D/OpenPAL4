#pragma once

#include <cstdint>

namespace pal4 {

class ICombatTargetEventRuntime {
public:
    virtual ~ICombatTargetEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(float value) = 0;
    [[nodiscard]] virtual std::uint32_t TargetType(std::uint32_t target) = 0;
    [[nodiscard]] virtual std::uint8_t IsStateThree(
        std::uint32_t target) = 0;
    virtual void SetEventData(std::uint32_t slot, std::uint32_t target,
                              std::int32_t first, float scale,
                              std::int32_t second) = 0;
    [[nodiscard]] virtual double PropertyValue(
        std::uint32_t target, std::uint32_t property) = 0;
    virtual void ApplyProperty(std::uint32_t slot, float value) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessObject(
        std::uint32_t slot, std::uint32_t target) = 0;
};

class CombatTargetEventHandler final {
public:
    // IDA 0x580a70.
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t action, std::uint32_t target,
        ICombatTargetEventRuntime& runtime);
};

}  // namespace pal4
