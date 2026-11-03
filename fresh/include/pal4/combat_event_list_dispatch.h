#pragma once

#include <cstdint>

namespace pal4 {

class ICombatEventListDispatchRuntime {
public:
    virtual ~ICombatEventListDispatchRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(float value) = 0;
    [[nodiscard]] virtual std::int32_t DispatchId(
        std::uint32_t target, std::int32_t id) = 0;
};

class CombatEventListDispatch final {
public:
    // IDA 0x581210; the second and third native arguments are unused.
    [[nodiscard]] static std::int32_t Process(
        std::uint32_t action, std::uint32_t target,
        ICombatEventListDispatchRuntime& runtime);
    // IDA 0x5813f0; the second native argument is unused.
    [[nodiscard]] static std::int32_t ProcessSecond(
        std::uint32_t action, std::uint32_t target,
        ICombatEventListDispatchRuntime& runtime);
};

}  // namespace pal4
