#pragma once

#include "pal4/pal_actor_control.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

struct ActorControlEffectHandle {
    std::uint32_t object{};
    std::uint32_t token{};
};
static_assert(sizeof(ActorControlEffectHandle) == 8u);

class IActorControlFlashRuntime {
public:
    virtual ~IActorControlFlashRuntime() = default;
    [[nodiscard]] virtual bool NeedsUpdate(std::uint32_t* handle);
    virtual void HandleEvent(std::uint32_t* handle, std::uint32_t event);
    [[nodiscard]] virtual ActorControlEffectHandle CreateEffect(
        std::string_view name, std::uint32_t parameter_address,
        std::uint32_t enabled, std::uint32_t extra);
    virtual void ResetTemporary(ActorControlEffectHandle& temporary);
    [[nodiscard]] virtual std::uint32_t ReadEffectProperty(
        std::uint32_t context_address);
    virtual void UpdateProperties(std::uint32_t* handle,
                                  std::uint32_t property);
};

class PalActorControlFlashEffect final {
public:
    static constexpr std::uint32_t kParameterAddress = 0x84b1dcu;

    // IDA 0x426330. context_address is the original non-null a3 pointer.
    [[nodiscard]] static bool Update(PalActorControlState& state,
                                     std::uint32_t context_address,
                                     IActorControlFlashRuntime& runtime);
};

}  // namespace pal4
