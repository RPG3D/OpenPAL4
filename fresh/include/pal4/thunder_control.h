#pragma once

#include "pal4/thunder_object.h"

#include <cstdint>

namespace pal4 {

class IThunderResetRuntime {
public:
    virtual ~IThunderResetRuntime() = default;
    virtual void RefreshTarget(ThunderObjectState& state,
                               std::uint32_t owner) = 0;
    virtual void UpdateFrame(ThunderObjectState& state) = 0;
};

class IThunderStopRuntime {
public:
    virtual ~IThunderStopRuntime() = default;
    virtual void ResetVisual(ThunderObjectState& state) = 0;
    virtual void ClearTargetName(ThunderObjectState& state) = 0;
    virtual void ProcessEffectEvent(std::uint32_t system,
                                    ThunderObjectState& state) = 0;
    [[nodiscard]] virtual std::uint32_t DeleteObject(
        ThunderObjectState& state, std::uint32_t flags) = 0;
};

class ThunderControl final {
public:
    // IDA 0x61c1b0.
    [[nodiscard]] static std::uint8_t Reset(
        ThunderObjectState& state, IThunderResetRuntime& runtime);
    // IDA 0x61c280.
    [[nodiscard]] static std::uint32_t Stop(
        ThunderObjectState& state, IThunderStopRuntime& runtime);
};

}  // namespace pal4
