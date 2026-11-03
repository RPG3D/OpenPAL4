#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace pal4 {

enum class CombatEndingTaskType {
    ScoreText,
    LevelText,
    Window,
    MoveDownWindow,
    UpdateExperience,
    ShowStep,
    PrizeProperty,
    CloseSystem,
};

struct CombatEndingAttributeRecord {
    std::uint32_t image = 0u;
    std::int32_t limit = 0;
};

class ICombatEndingTaskSequenceRuntime {
public:
    virtual ~ICombatEndingTaskSequenceRuntime() = default;
    // Native owners may compose the same parent constructor in their allocator.
    [[nodiscard]] virtual std::uint32_t CreateSequence();
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void InitializeContainer(std::uint32_t address) = 0;
    virtual void InitializeName(std::uint32_t address,
                                const char* text) = 0;
    [[nodiscard]] virtual bool CombatInactive() = 0;
    [[nodiscard]] virtual std::string GlobalName(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CreateTask(
        CombatEndingTaskType type) = 0;
    virtual void AddTask(std::uint32_t container,
                         std::uint32_t task, const std::string& name,
                         std::int32_t mode, float time,
                         std::int32_t reserved) = 0;
    [[nodiscard]] virtual std::uint32_t BeginActors(
        std::uint32_t query) = 0;
    [[nodiscard]] virtual bool ActorsDone(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentActor(
        std::uint32_t iterator) = 0;
    virtual void AdvanceActor(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual int ReleaseActors(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::int32_t ActorStat(
        std::uint32_t actor, std::uint32_t stat) = 0;
    [[nodiscard]] virtual std::int32_t RequiredExperience(
        std::uint32_t player_key, std::int32_t next_level) = 0;
    // Player virtual256 takes no arguments. The prior XP stays on the stack
    // for the following4E9BF0 database call (547626/547627/54762D).
    [[nodiscard]] virtual std::uint32_t ActorRoleKey(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::optional<CombatEndingAttributeRecord>
    FindAttribute(std::uint32_t id, std::int32_t previous_experience) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void ResetFlags(std::uint32_t address) = 0;
    virtual void ResetActorManagerFlags() = 0;
    virtual void RegisterSequence(std::uint32_t router,
                                  std::uint32_t container) = 0;
    [[nodiscard]] virtual bool ActorStateNotThree(
        std::uint32_t actor) = 0;
    virtual void ResetObjectProperties(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t CombatWinAction(
        std::uint32_t actor) = 0;
    virtual void RegisterActorEvent(std::uint32_t value) = 0;
};

class CombatEndingTaskSequence final {
public:
    static constexpr std::uint32_t kContainerVtable = 0x845390u;

    // IDA ui_showEndTaskScore at 0x547270.
    [[nodiscard]] static int Start(
        std::uint32_t dialog, ICombatEndingTaskSequenceRuntime& runtime);
};

}  // namespace pal4
