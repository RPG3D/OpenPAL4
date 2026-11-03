#pragma once

#include <cstdint>

namespace pal4 {

class ICombatFloatComparisonEventRuntime {
public:
    virtual ~ICombatFloatComparisonEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(float value) = 0;
    [[nodiscard]] virtual std::uint32_t LookupEvent(std::int32_t id) = 0;
    [[nodiscard]] virtual double ReadEventValue(
        std::uint32_t event, std::uint32_t context_address) = 0;
    virtual void OnEqualZero(std::uint32_t context,
                             std::uint32_t target) = 0;
    virtual void Dispatch(std::uint32_t target, std::int32_t id,
                          std::uint32_t context) = 0;
};

class CombatFloatComparisonEvent final {
public:
    // IDA 0x581430; the second native argument is unused.
    static void Process(std::uint32_t action, std::uint32_t target,
                        std::uint32_t context,
                        ICombatFloatComparisonEventRuntime& runtime);
};

}  // namespace pal4
