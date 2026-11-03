#pragma once

#include "pal4/combat_ending_task_sequence.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct CombatEndingTaskFactoryBinding {
    std::uint32_t entry;
    CombatEndingTaskType type;
    std::uint32_t vtable;
};

class ICombatEndingTaskFactoryRuntime {
public:
    virtual ~ICombatEndingTaskFactoryRuntime() = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class CombatEndingTaskFactories final {
public:
    // IDA 0x547b50..0x547bc0, eight consecutive vtable setters.
    static constexpr std::array<CombatEndingTaskFactoryBinding, 8>
        kBindings{{
            {0x547b50u, CombatEndingTaskType::ScoreText, 0x8453acu},
            {0x547b60u, CombatEndingTaskType::LevelText, 0x8453b0u},
            {0x547b70u, CombatEndingTaskType::Window, 0x8453b4u},
            {0x547b80u, CombatEndingTaskType::MoveDownWindow, 0x8453b8u},
            {0x547b90u, CombatEndingTaskType::UpdateExperience, 0x8453bcu},
            {0x547ba0u, CombatEndingTaskType::ShowStep, 0x8453c0u},
            {0x547bb0u, CombatEndingTaskType::PrizeProperty, 0x8453c4u},
            {0x547bc0u, CombatEndingTaskType::CloseSystem, 0x8453c8u}}};

    [[nodiscard]] static std::uint32_t Bind(
        CombatEndingTaskType type, std::uint32_t factory,
        ICombatEndingTaskFactoryRuntime& runtime);
};

}  // namespace pal4
