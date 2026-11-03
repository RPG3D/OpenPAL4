#pragma once

#include "pal4/monster_behavior_manager.h"

#include <functional>

namespace pal4 {

class NativeBaseObjectMemory;
class NativeMonsterManagerOwner;
class NativeMonsterObjectOwner;
class NativeMonsterProximityRuntime;
class NativeMonsterPursuitPathRuntime;
class NativeMonsterActionDispatchRuntime;
class NativeMonsterArenaSkillFrame;
class IGameResourceManagerRuntime;

// Actual encounter implementation must supply these two original calls.
class IMonsterEncounterRuntime {
public:
    virtual ~IMonsterEncounterRuntime() = default;
    virtual void DispatchCombatPositions(std::uint32_t object, float elapsed) = 0;
    virtual void InitializeCombat(std::uint32_t object, std::int32_t argument) = 0;
};

// Scene-owned composition of the existing 0x433850 pursuit state machine.
// Interaction scripts and combat are mandatory caller-owned dependencies.
class NativeMonsterPursuitFrame final : public IMonsterPursuitRuntime {
public:
    NativeMonsterPursuitFrame(
        NativeBaseObjectMemory& memory, NativeMonsterManagerOwner& manager,
        NativeMonsterProximityRuntime& proximity,
        NativeMonsterPursuitPathRuntime& paths,
        NativeMonsterActionDispatchRuntime& group,
        NativeMonsterArenaSkillFrame& skills,
        IGameResourceManagerRuntime& transforms,
        IMonsterInteractionRuntime& interaction, IMonsterEncounterRuntime& encounter,
        std::function<std::uint32_t()> arena_skill_manager);
    [[nodiscard]] bool Update(std::uint32_t object, float elapsed);

    std::string_view AnimationName(std::uint32_t animation) override;
    std::uint32_t SelectAnimation(std::uint32_t animation, const char* name) override;
    IScriptEventProcessorRuntime& ProcessorRuntime() override;
    void CleanupScriptObjectCollection(std::uint32_t address) override;
    void LoadFuzzyBehaviorConfig(std::uint32_t object, std::uint32_t child) override;
    void DispatchComponentField(std::uint32_t object, std::uint32_t slot,
                                std::uint32_t field, std::uint32_t argument) override;
    bool CheckProximity(MonsterBehaviorManagerState&, std::uint32_t, float) override;
    bool PlayerControlBusy() override;
    bool CheckDistanceAndFlag(const MonsterBehaviorManagerState&,
                              std::uint32_t, float) override;
    bool CheckTargetReached(const MonsterBehaviorManagerState&,
                            std::uint32_t, float) override;
    void ProcessCollisionAndConfig(MonsterBehaviorManagerState&,
                                   std::uint32_t, float) override;
    bool ProcessPursuitCollisionAndSpeed(const MonsterBehaviorManagerState&,
        std::uint32_t, float, std::uint32_t) override;
    bool ProcessObjectInteraction(MonsterBehaviorManagerState&,
        std::uint32_t, std::uint32_t, float) override;
    void ProcessConfigAndEvents(std::uint32_t object, float elapsed) override;
    void InitializeCombatSystem(std::uint32_t object, std::int32_t argument) override;
    std::uint32_t ResetPursuit(MonsterBehaviorManagerState&,
                               std::uint32_t object) override;
    void SetRecordErrorFlag(std::uint32_t record, std::uint8_t flag) override;
    void ProcessActionWithScript(std::uint32_t object, float elapsed) override;
    void VisitArenaObjects(std::uint32_t object) override;

private:
    void RequireObject(std::uint32_t object) const;
    NativeBaseObjectMemory& memory_;
    NativeMonsterManagerOwner& manager_;
    NativeMonsterProximityRuntime& proximity_;
    NativeMonsterPursuitPathRuntime& paths_;
    NativeMonsterActionDispatchRuntime& group_;
    NativeMonsterArenaSkillFrame& skills_;
    IGameResourceManagerRuntime& transforms_;
    IMonsterInteractionRuntime& interaction_;
    IMonsterEncounterRuntime& encounter_;
    std::function<std::uint32_t()> arena_skill_manager_;
    NativeMonsterObjectOwner* current_ = nullptr;
};

}  // namespace pal4
