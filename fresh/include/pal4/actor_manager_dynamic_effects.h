#pragma once

#include "pal4/actor_render_state.h"

#include <cstdint>

namespace pal4 {

class IActorManagerDynamicEffectsRuntime {
public:
    virtual ~IActorManagerDynamicEffectsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t NilNode() = 0;
    [[nodiscard]] virtual std::uint32_t LowerBound(
        std::uint32_t tree, std::int32_t key) = 0;
    virtual void NotifyEffectEnd(std::uint32_t handle_address,
                                 std::uint32_t context) = 0;
    virtual void EraseNode(std::uint32_t tree, std::uint32_t node) = 0;
    [[nodiscard]] virtual ActorEffectHandle CreateEffect(
        std::uint32_t actor, std::uint32_t first_argument,
        std::uint32_t second_argument) = 0;
    [[nodiscard]] virtual bool IsValid(ActorEffectHandle handle) = 0;
    [[nodiscard]] virtual ActorEffectHandle CopyEffect(
        ActorEffectHandle handle) = 0;
    [[nodiscard]] virtual std::uint32_t FirstNode(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t Predecessor(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t InsertNode(
        std::uint32_t tree, std::uint32_t cursor,
        std::uint32_t parent, std::int32_t key,
        ActorEffectHandle handle) = 0;
    virtual void RecordResult(std::uint32_t node, bool inserted) = 0;
    virtual void ResetTemporary(ActorEffectHandle handle) = 0;
};

class ActorManagerDynamicEffects final {
public:
    static void Add(std::uint32_t actor, std::uint32_t context,
                    std::uint32_t record,
                    IActorManagerDynamicEffectsRuntime& runtime);
    static void RemoveMatching(std::uint32_t actor, std::uint32_t context,
                               std::uint32_t record,
                               IActorManagerDynamicEffectsRuntime& runtime);
};

}  // namespace pal4
