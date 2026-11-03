#pragma once

#include "pal4/actor_manager_owned_slot_array.h"

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>

namespace pal4 {

// Game policy separated from the historical owned-slot container layout.
// Native adapters use STL ownership; the existing image adapter remains usable.
class IActorManagerModelPoolRuntime {
public:
    virtual ~IActorManagerModelPoolRuntime() = default;
    virtual bool CombatInactive() = 0;
    virtual std::size_t CachedCount() = 0;
    virtual std::string CachedName(std::size_t index) = 0;
    virtual std::uint32_t ExtractCached(std::size_t index) = 0;
    virtual std::uint32_t CreateModel(std::string_view name) = 0;
    virtual void DeactivateModel(std::uint32_t actor, std::uint32_t context) = 0;
    virtual void EvictOldest() = 0;
    virtual void AppendModel(std::uint32_t actor) = 0;
};

class IActorManagerActorPoolRuntime
    : public IActorManagerOwnedSlotArrayRuntime {
public:
    virtual ~IActorManagerActorPoolRuntime() = default;
    [[nodiscard]] virtual bool CombatInactive() = 0;
    [[nodiscard]] virtual std::string ActorName(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructActor(
        std::uint32_t storage) = 0;
    [[nodiscard]] virtual bool InitializeActor(
        std::uint32_t actor, std::string_view first,
        std::string_view second) = 0;
    virtual void DeactivateActor(std::uint32_t actor,
                                 std::uint32_t context) = 0;
    [[nodiscard]] virtual std::uint32_t CreateTemporarySlot(
        std::uint32_t actor) = 0;
};

class ActorManagerActorPool final {
public:
    [[nodiscard]] static std::uint32_t TakeOrCreate(
        std::string_view name, IActorManagerModelPoolRuntime& runtime);
    static void Return(std::uint32_t limit, std::uint32_t actor,
                       std::uint32_t context, IActorManagerModelPoolRuntime& runtime);
    [[nodiscard]] static std::uint32_t TakeOrCreate(
        std::uint32_t pool, std::string_view name,
        IActorManagerActorPoolRuntime& runtime);
    static void Return(std::uint32_t pool, std::uint32_t limit,
                       std::uint32_t actor, std::uint32_t context,
                       IActorManagerActorPoolRuntime& runtime);
};

}  // namespace pal4
