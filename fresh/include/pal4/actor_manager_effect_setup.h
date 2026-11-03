#pragma once

#include "pal4/actor_render_state.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorManagerEffectLoadRuntime {
public:
    virtual ~IActorManagerEffectLoadRuntime() = default;
    [[nodiscard]] virtual ActorEffectHandle LoadBottomEffect(
        std::uint32_t actor, std::uint32_t frame_address,
        std::string_view hook) = 0;
    [[nodiscard]] virtual ActorEffectHandle LoadNamedEffect(
        std::uint32_t actor, std::string_view name,
        std::string_view hook) = 0;
    virtual void StoreEffect(std::uint32_t handle_address,
                             ActorEffectHandle handle) = 0;
    virtual void ResetTemporary(ActorEffectHandle handle) = 0;
    [[nodiscard]] virtual std::uint32_t ModelId(std::uint32_t actor) = 0;
};

class IActorManagerEffectSetupRuntime : public IActorManagerEffectLoadRuntime {
public:
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void InvokeActorSlot(std::uint32_t actor, std::uint32_t slot,
                                 std::uint32_t argument) = 0;
    virtual void ResetEffect(std::uint32_t actor,
                             std::uint32_t handle_address) = 0;
};

class ActorManagerEffectSetup final {
public:
    static void RefreshAttachments(std::uint32_t actor,IActorManagerEffectSetupRuntime& runtime);
    static void Initialize(std::uint32_t actor, std::uint32_t context,
                           IActorManagerEffectSetupRuntime& runtime);
    static void LoadAttachments(std::uint32_t actor, std::uint32_t child,
                                IActorManagerEffectLoadRuntime& runtime);
};

}  // namespace pal4
