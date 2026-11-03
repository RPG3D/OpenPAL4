#pragma once

#include "pal4/combat_action_factory.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IWoundedActionFactoryRuntime : public ICombatActionFactoryRuntime {
public:
    virtual ~IWoundedActionFactoryRuntime() = default;
    [[nodiscard]] virtual bool ActorInTerminalState(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ActorActionKind(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ActorWord(
        std::uint32_t actor, std::uint32_t offset) = 0;
    virtual void InitializeModeName(
        std::uint32_t header, std::string_view name,
        std::uint8_t mode) = 0;
    virtual void SetExceptionMessage(
        std::uint32_t header, std::string_view name) = 0;
};

class CombatWoundedActionFactory final {
public:
    // IDA 0x567bf0: choose QuickWounded, FinalHit or WoundedTask.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t source, std::uint32_t target,
        IWoundedActionFactoryRuntime& runtime);
};

}  // namespace pal4
