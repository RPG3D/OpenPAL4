#pragma once

#include <cstdint>

namespace pal4 {

class IActorManagerPlayerConditionRuntime {
public:
    virtual ~IActorManagerPlayerConditionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ReadSignedWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool AlreadyUsed(std::uint32_t values,
                                            std::uint32_t key) = 0;
    [[nodiscard]] virtual bool FixedContains(std::uint32_t values,
                                              std::uint32_t key) = 0;
    [[nodiscard]] virtual bool IndexedContains(std::uint32_t values,
                                                std::uint32_t group,
                                                std::uint32_t key) = 0;
    [[nodiscard]] virtual std::int32_t ActorSlot52(
        std::uint32_t actor, std::uint32_t mode) = 0;
    [[nodiscard]] virtual bool ScriptElementExists(
        std::uint32_t actor, std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t Inventory() = 0;
    [[nodiscard]] virtual std::uint32_t GameDbPair(
        std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t FindInventoryElement(
        std::uint32_t inventory, std::uint32_t id,
        std::uint32_t variant) = 0;
};

class ActorManagerPlayerConditions final {
public:
    [[nodiscard]] static bool CheckPrimary(
        std::uint32_t actor, std::uint32_t requirement,
        IActorManagerPlayerConditionRuntime& runtime);
    [[nodiscard]] static bool CheckSecondary(
        std::uint32_t actor, std::uint32_t requirement,
        IActorManagerPlayerConditionRuntime& runtime);
};

}  // namespace pal4
