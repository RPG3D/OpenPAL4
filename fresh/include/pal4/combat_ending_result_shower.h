#pragma once

#include "pal4/combat_ending_task_sequence.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class ICombatEndingResultShowRuntime {
public:
    virtual ~ICombatEndingResultShowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CreateSequence();
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void CleanupCombatConsole() = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void InitializeContainer(std::uint32_t address) = 0;
    virtual void InitializeName(std::uint32_t address,
                                const char* text) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetYPosition(std::uint32_t window, float y) = 0;
    virtual void SetText(std::uint32_t window,
                         std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t BeginActors() = 0;
    [[nodiscard]] virtual bool ActorsDone(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentActor(
        std::uint32_t iterator) = 0;
    virtual void AdvanceActor(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual int ReleaseActors(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::int32_t ActorStat(
        std::uint32_t actor, std::uint32_t stat) = 0;
    [[nodiscard]] virtual std::int32_t RequiredExperience(
        std::uint32_t player_key, std::int32_t next_level) = 0;
    virtual void HandleLevelUp(std::uint32_t row,
                               std::int32_t level,
                               std::uint32_t actor,
                               std::uint32_t hint,
                               std::uint32_t record) = 0;
    [[nodiscard]] virtual std::string GlobalName(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CreateTask(
        CombatEndingTaskType type) = 0;
    virtual void AddTask(std::uint32_t container,
                         std::uint32_t task,
                         const std::string& name,
                         std::int32_t mode, float time,
                         std::int32_t reserved) = 0;
    virtual void ResetFlags(std::uint32_t address) = 0;
    [[nodiscard]] virtual int RegisterSequence(
        std::uint32_t router, std::uint32_t container) = 0;
};

class CombatEndingResultShower final {
public:
    // IDA CombatEndingResult_Show at 0x547bd0.
    [[nodiscard]] static int Show(std::uint32_t dialog,
                                  ICombatEndingResultShowRuntime& runtime);
    // IDA ui_hideCombatResult at 0x5481d0.
    [[nodiscard]] static int Hide(std::uint32_t dialog,
                                  ICombatEndingResultShowRuntime& runtime);
};

}  // namespace pal4
