#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace pal4 {

struct ActorPropertyForwardSpec {
    std::uint32_t entry;
    std::uint32_t slot;
    std::uint8_t argument_count;
};

class IActorManagerPropertyForwardRuntime {
public:
    virtual ~IActorManagerPropertyForwardRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeProperty(
        std::uint32_t property, std::uint32_t actor,
        std::uint32_t slot, std::span<const std::uint32_t> arguments) = 0;
    [[nodiscard]] virtual std::array<std::uint32_t, 3> ChildPosition(
        std::uint32_t actor) = 0;
};

class ActorManagerPropertyForwarders final {
public:
    [[nodiscard]] static std::span<const ActorPropertyForwardSpec> Specs();
    [[nodiscard]] static std::uint32_t Forward(
        std::uint32_t entry, std::uint32_t actor,
        std::span<const std::uint32_t> arguments,
        IActorManagerPropertyForwardRuntime& runtime);
    [[nodiscard]] static std::array<std::uint32_t, 3> CopyChildPosition(
        std::uint32_t embedded_member,
        IActorManagerPropertyForwardRuntime& runtime);
};

}  // namespace pal4
