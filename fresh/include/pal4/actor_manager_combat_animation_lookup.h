#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorManagerCombatAnimationLookupRuntime {
public:
    virtual ~IActorManagerCombatAnimationLookupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view ReadText(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual bool FindAnimation(
        std::uint32_t child, std::string_view name) = 0;
};

class ActorManagerCombatAnimationLookup final {
public:
    static constexpr std::uint32_t kMissingAnimation = 0x8fa38cu;

    [[nodiscard]] static std::uint32_t FromPrimaryName(
        std::uint32_t actor, std::uint32_t source,
        IActorManagerCombatAnimationLookupRuntime& runtime);
    [[nodiscard]] static std::uint32_t FromSecondaryName(
        std::uint32_t actor, std::uint32_t source,
        IActorManagerCombatAnimationLookupRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t Find(
        std::uint32_t actor, std::uint32_t source,
        std::uint32_t text_offset,
        IActorManagerCombatAnimationLookupRuntime& runtime);
};

}  // namespace pal4
