#pragma once

#include "pal4/pal_actor_control_flash_effect.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

struct ActorControlEffectCreation {
    ActorControlEffectHandle handle{};
    std::uint32_t return_value{};
};

class IActorControlInvincibilityRuntime {
public:
    virtual ~IActorControlInvincibilityRuntime() = default;
    [[nodiscard]] virtual bool NeedsUpdate(std::uint32_t* handle);
    virtual void Update(std::uint32_t* handle);
    [[nodiscard]] virtual ActorControlEffectCreation CreateEffect(
        std::string_view name, std::uint32_t parameter_address,
        std::uint32_t first_flag, std::uint32_t second_flag);
    virtual void ResetTemporary(ActorControlEffectHandle& temporary);
};

class PalActorControlInvincibility final {
public:
    // IDA 0x4265f0. A successful call leaves the create result in EAX.
    [[nodiscard]] static std::uint32_t Enable(
        PalActorControlState& state, std::uint32_t value,
        IActorControlInvincibilityRuntime& runtime);
};

}  // namespace pal4
