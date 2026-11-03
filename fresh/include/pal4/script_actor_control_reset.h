#pragma once

#include <cstdint>

namespace pal4 {

class IScriptActorControlResetRuntime {
public:
    virtual ~IScriptActorControlResetRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    virtual void ResetPlayerToJumpStart(std::uint32_t control,
                                        std::uint32_t caller_context) = 0;
};

class ScriptActorControlReset final {
public:
    // IDA 0x5d8140, giResetPlayerToJumpStart; caller context is in EBP.
    static void ToJumpStart(std::uint32_t caller_context,
                            IScriptActorControlResetRuntime& runtime);
};

}  // namespace pal4
