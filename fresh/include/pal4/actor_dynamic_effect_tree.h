#pragma once

#include "pal4/actor_effect_record.h"

#include <cstdint>

namespace pal4 {

class IActorDynamicEffectTreeRuntime : public IActorEffectRecordRuntime {
public:
    virtual ~IActorDynamicEffectTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    [[nodiscard]] virtual std::uint32_t SharedReferences() = 0;
    virtual void SetSharedNil(std::uint32_t value) = 0;
    virtual void SetSharedReferences(std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
};

class ActorDynamicEffectTree final {
public:
    static void Destroy(std::uint32_t tree,
                        IActorDynamicEffectTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Erase(
        std::uint32_t tree, std::uint32_t result, std::uint32_t node,
        IActorDynamicEffectTreeRuntime& runtime);
    static void EraseRange(std::uint32_t tree, std::uint32_t result,
                           std::uint32_t first, std::uint32_t last,
                           IActorDynamicEffectTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Successor(
        std::uint32_t node, IActorDynamicEffectTreeRuntime& runtime);
    static void ClearSubtree(std::uint32_t root,
                             IActorDynamicEffectTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t InsertAt(
        std::uint32_t tree, std::uint32_t cursor, std::uint32_t parent,
        std::uint32_t record, IActorDynamicEffectTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Leftmost(
        std::uint32_t node, IActorDynamicEffectTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t LowerBound(
        std::uint32_t tree, std::int32_t key,
        IActorDynamicEffectTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Predecessor(
        std::uint32_t node, IActorDynamicEffectTreeRuntime& runtime);
};

}  // namespace pal4
