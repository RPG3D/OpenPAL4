#pragma once

#include <cstdint>

namespace pal4 {

class ICombatActorStatusRuntime {
public:
    virtual ~ICombatActorStatusRuntime() = default;
    [[nodiscard]] virtual std::int32_t Query(
        std::uint32_t actor, std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual std::int32_t QueryIndexed(
        std::uint32_t actor, std::uint32_t vtable_offset,
        std::uint32_t index) = 0;
    [[nodiscard]] virtual std::int32_t Set(
        std::uint32_t actor, std::uint32_t vtable_offset,
        std::int32_t value) = 0;
};

class CombatActorStatus final {
public:
    // IDA 0x585350..0x585440.
    [[nodiscard]] static bool IsStateThree(
        std::uint32_t actor, ICombatActorStatusRuntime& runtime);
    [[nodiscard]] static bool IsNotStateThree(
        std::uint32_t actor, ICombatActorStatusRuntime& runtime);
    [[nodiscard]] static std::int32_t SetSecondaryStateOne(
        std::uint32_t actor, ICombatActorStatusRuntime& runtime);
    [[nodiscard]] static std::int32_t SetSecondaryStateZero(
        std::uint32_t actor, ICombatActorStatusRuntime& runtime);
    [[nodiscard]] static bool IsStatusOne(
        std::uint32_t actor, ICombatActorStatusRuntime& runtime);
    [[nodiscard]] static std::int32_t ResetStatus(
        std::uint32_t actor, ICombatActorStatusRuntime& runtime);
    [[nodiscard]] static std::int32_t QueryConditionalStatus(
        std::uint32_t actor, std::int32_t mode,
        ICombatActorStatusRuntime& runtime);
    [[nodiscard]] static std::int32_t UpdateHealthStatus(
        std::uint32_t actor, ICombatActorStatusRuntime& runtime);
};

}  // namespace pal4
