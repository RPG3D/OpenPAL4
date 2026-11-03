#pragma once

#include "pal4/combat_action_factory.h"

#include <cstdint>

namespace pal4 {

class IWoundedDeathFactoryRuntime : public ICombatActionFactoryRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool ActorCheckStateThree(
        std::uint32_t actor) = 0;
};

}  // namespace pal4
