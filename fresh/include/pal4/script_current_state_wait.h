#pragma once

#include <cstdint>

namespace pal4 {

class IScriptCurrentStateWaitRuntime {
public:
    virtual ~IScriptCurrentStateWaitRuntime() = default;
    virtual std::uint32_t CurrentState() = 0;
    virtual std::int32_t CurrentStateId(std::uint32_t state) = 0;
};

// IDA 0x5e6ef0 is shared by trade (849600) and puzzle (84960c).
// Preserve both current-state reads and the absent-state short circuit.
class ScriptCurrentStateWait final {
public:
    static bool Complete(IScriptCurrentStateWaitRuntime& runtime) {
        return runtime.CurrentState() == 0u ||
               runtime.CurrentStateId(runtime.CurrentState()) == 1;
    }
};

} // namespace pal4
