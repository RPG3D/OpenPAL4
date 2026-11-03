#pragma once

#include <cstdint>

namespace pal4 {

class ICombatEventPropertyDispatchRuntime {
public:
    virtual ~ICombatEventPropertyDispatchRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(float value) = 0;
    [[nodiscard]] virtual std::uint32_t LookupEvent(std::int32_t id) = 0;
    [[nodiscard]] virtual std::int32_t ApplyEvent(
        std::uint32_t event, std::uint32_t target,
        std::uint32_t scale_bits) = 0;
};

class CombatEventPropertyDispatch final {
public:
    // IDA 0x581140; the second and third native arguments are unused.
    [[nodiscard]] static std::int32_t Process(
        std::uint32_t action, std::uint32_t target,
        ICombatEventPropertyDispatchRuntime& runtime);
    // IDA 0x581180; the second and third native arguments are unused.
    [[nodiscard]] static std::int32_t Clear(
        std::uint32_t action, std::uint32_t target,
        ICombatEventPropertyDispatchRuntime& runtime);
};

}  // namespace pal4
