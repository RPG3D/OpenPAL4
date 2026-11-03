#pragma once

#include <cstdint>

namespace pal4 {

class ICombatConditionalEventDispatchRuntime {
public:
    virtual ~ICombatConditionalEventDispatchRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(float value) = 0;
    [[nodiscard]] virtual bool ElementExists(std::uint32_t context,
                                              std::int32_t id) = 0;
    virtual void Dispatch(std::uint32_t target, std::int32_t id,
                          std::uint32_t context) = 0;
};

class CombatConditionalEventDispatch final {
public:
    // IDA 0x581380; the second native argument is unused.
    static void Process(std::uint32_t action, std::uint32_t target,
                        std::uint32_t context,
                        ICombatConditionalEventDispatchRuntime& runtime);
};

}  // namespace pal4
