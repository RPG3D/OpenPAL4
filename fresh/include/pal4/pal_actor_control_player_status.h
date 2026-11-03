#pragma once

#include "pal4/pal_actor_control.h"

#include <cstdint>

namespace pal4 {

class IActorControlPlayerStatusRuntime {
public:
    virtual ~IActorControlPlayerStatusRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager();
    [[nodiscard]] virtual std::uint32_t FindPlayer(
        std::uint32_t manager, std::uint32_t index);
    [[nodiscard]] virtual std::uint32_t HandleDamageOrStatus(
        PalActorControlState& state, float value);
};

class PalActorControlPlayerStatus final {
public:
    // IDA 0x4266a0.
    [[nodiscard]] static std::uint32_t ApplyIfPlayerExists(
        PalActorControlState& state, float value,
        IActorControlPlayerStatusRuntime& runtime);
    // IDA 0x4266d0.
    [[nodiscard]] static std::uint32_t Start(
        PalActorControlState& state, std::uint32_t first,
        std::uint32_t second, float value,
        IActorControlPlayerStatusRuntime& runtime);
};

}  // namespace pal4
