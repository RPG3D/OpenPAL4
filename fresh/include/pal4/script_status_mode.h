#pragma once

#include <cstdint>

namespace pal4 {

class IScriptStatusModeRuntime {
public:
    virtual ~IScriptStatusModeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    [[nodiscard]] virtual std::uint32_t ActivateStatusMode(
        std::uint32_t actor_control, std::int32_t mode) = 0;
};

class ScriptStatusMode final {
public:
    // IDA 0x5d8b40, giEnableSTS.
    [[nodiscard]] static std::uint32_t Enable(
        std::int32_t mode, IScriptStatusModeRuntime& runtime);
};

}  // namespace pal4
