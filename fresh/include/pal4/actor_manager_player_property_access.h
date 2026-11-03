#pragma once

#include <cstdint>

namespace pal4 {

class IActorManagerPlayerPropertyAccessRuntime {
public:
    virtual ~IActorManagerPlayerPropertyAccessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void ActorSlot52(std::uint32_t actor, std::uint32_t property) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectSlot168(
        std::uint32_t object, std::uint32_t actor, std::uint32_t property,
        std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ForwardPropertySlot168(
        std::uint32_t actor, std::uint32_t property,
        std::uint32_t value) = 0;
};

class ActorManagerPlayerPropertyAccess final {
public:
    [[nodiscard]] static std::uint32_t OwnedChild(
        std::uint32_t actor, IActorManagerPlayerPropertyAccessRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardProperty(
        std::uint32_t actor, std::uint32_t property, std::uint32_t value,
        IActorManagerPlayerPropertyAccessRuntime& runtime);
    [[nodiscard]] static std::uint32_t IndexedValue(
        std::uint32_t actor, std::uint32_t index,
        IActorManagerPlayerPropertyAccessRuntime& runtime);
};

}  // namespace pal4
