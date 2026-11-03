#pragma once

#include "pal4/script_event_processor.h"
#include "pal4/monster_behavior_factory_registration.h"
#include "pal4/script_object.h"
#include "pal4/monster_fuzzy_behavior_loader.h"
#include "pal4/monster_proximity_checker.h"
#include "pal4/monster_interaction_processor.h"
#include "pal4/monster_pursuit_collision_speed.h"
#include "pal4/monster_action_dispatcher.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {
class IMonsterBehaviorConstructionRuntime;
class IMonsterBehaviorComponentRuntime;

struct MonsterBehaviorManagerState {
    std::array<std::uint32_t, 463> words{};
};
static_assert(sizeof(MonsterBehaviorManagerState) == 0x73cu);

struct MonsterBehaviorTreeBytes {
    std::uint8_t first{};
    std::uint8_t second{};
};

class IMonsterPursuitEntryRuntime {
public:
    virtual ~IMonsterPursuitEntryRuntime() = default;
    virtual IScriptEventProcessorRuntime& ProcessorRuntime() = 0;
    virtual void CleanupScriptObjectCollection(std::uint32_t address) = 0;
    virtual std::uint32_t SelectAnimation(std::uint32_t animation,
                                         const char* name) = 0;
    virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    // Native host strings publish borrowed views without legacy refcount headers.
    // Return true only when the creating owner has released this exact record.
    virtual bool ReleaseNativeHistoryRecord(std::uint32_t) { return false; }
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void FreeStringStorage(std::uint32_t record,
                                   std::uint32_t allocation,
                                   std::uint32_t bytes) = 0;
};

class IMonsterBehaviorFrameRuntime {
public:
    virtual ~IMonsterBehaviorFrameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ProcessManagerState(
        MonsterBehaviorManagerState& state, std::uint32_t address, float elapsed) = 0;
    virtual void CheckStateAndProcessCollision(MonsterBehaviorManagerState& state,
                                               std::uint32_t address, float elapsed) = 0;
    [[nodiscard]] virtual IScriptEventSchedulerRuntime& SchedulerRuntime() = 0;
    [[nodiscard]] virtual std::uint32_t UpdatePosition(
        std::uint32_t address, float elapsed, std::uint32_t argument) = 0;
};

class IMonsterStopPursuitRuntime {
public:
    virtual ~IMonsterStopPursuitRuntime() = default;
    virtual std::uint32_t ProcessCollisionAndConfig(
        std::uint32_t manager_address, float incoming_st0, float constant_one) = 0;
};

class IMonsterBehaviorManagerRuntime : public IMonsterPursuitEntryRuntime,
                                      public IMonsterBehaviorFrameRuntime, public IMonsterStopPursuitRuntime, public IMonsterBehaviorFactoryRegistrationRuntime {
public:
    virtual ~IMonsterBehaviorManagerRuntime() = default;
    [[nodiscard]] virtual IScriptObjectRuntime& ScriptRuntime() = 0;
    [[nodiscard]] virtual std::uint8_t ProcessorIncomingByte() = 0;
    [[nodiscard]] virtual MonsterBehaviorTreeBytes TreeInputBytes() = 0;
    virtual void ConstructBase(MonsterBehaviorManagerState& state,
                               std::uint32_t address) = 0;
    virtual void DestructBase(MonsterBehaviorManagerState& state,
                              std::uint32_t address) = 0;
    virtual void InitializeTree(MonsterBehaviorManagerState& state,
                                std::uint32_t address) = 0;
    virtual void CleanupTree(MonsterBehaviorManagerState& state,
                             std::uint32_t address) = 0;
    virtual void DestroyAuxiliary(MonsterBehaviorManagerState& state,
                                  std::uint32_t address) = 0;
    virtual void CleanupWordArray(MonsterBehaviorManagerState& state,
                                  std::uint32_t address) = 0;
    virtual void CleanupHistory(MonsterBehaviorManagerState& state,
                                std::uint32_t address) = 0;
    virtual void SetRecordErrorFlag(std::uint32_t record_address,
                                    std::uint8_t value) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t TreeFirst(
        std::uint32_t tree_address) = 0;
    [[nodiscard]] virtual std::uint32_t TreeNext(
        std::uint32_t node_address) = 0;
    [[nodiscard]] virtual std::uint32_t TreeNilAddress() = 0;
    virtual void TreeEraseSubtree(std::uint32_t tree_address,
                                  std::uint32_t root_address) = 0;
    virtual void TreeRemoveNode(std::uint32_t tree_address,
                                std::uint32_t node_address) = 0;
    virtual void ReleaseSharedTreeNil() = 0;
    [[nodiscard]] virtual std::uint32_t ResetBaseObject(
        MonsterBehaviorManagerState& state, std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t XmlChildByIndex(
        std::uint32_t parent, std::uint32_t index) = 0;
    virtual void LoadFuzzyBehaviorConfig(std::uint32_t manager_address,
                                         std::uint32_t child_address) = 0;
    virtual void DispatchComponentField(std::uint32_t manager_address,
                                        std::uint32_t virtual_slot,
                                        std::uint32_t field_address) = 0;
    [[nodiscard]] virtual std::uint32_t GetConfigAttribute(
        std::uint32_t config_address, const char* name) = 0;
    [[nodiscard]] virtual std::int32_t ParseConfigInt(
        std::uint32_t attribute_address, std::int32_t fallback) = 0;
    [[nodiscard]] virtual std::uint32_t CallActionSlot4(
        std::uint32_t action_address) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessCollisionAndConfig(
        std::uint32_t manager_address, float incoming_st0,
        float constant_one) = 0;
};

class MonsterBehaviorManagerRuntimeStub
    : public IMonsterBehaviorManagerRuntime {
public:
    IScriptEventProcessorRuntime& ProcessorRuntime() override;
    IScriptObjectRuntime& ScriptRuntime() override;
    std::uint8_t ProcessorIncomingByte() override;
    MonsterBehaviorTreeBytes TreeInputBytes() override;
    void ConstructBase(MonsterBehaviorManagerState& state,
                       std::uint32_t address) override;
    void DestructBase(MonsterBehaviorManagerState& state,
                      std::uint32_t address) override;
    void InitializeTree(MonsterBehaviorManagerState& state,
                        std::uint32_t address) override;
    void CleanupTree(MonsterBehaviorManagerState& state,
                     std::uint32_t address) override;
    void DestroyAuxiliary(MonsterBehaviorManagerState& state,
                          std::uint32_t address) override;
    void CleanupWordArray(MonsterBehaviorManagerState& state,
                          std::uint32_t address) override;
    void CleanupHistory(MonsterBehaviorManagerState& state,
                        std::uint32_t address) override;
    void InsertBehavior(std::uint32_t tree_address,
                        std::uint32_t class_id,
                        std::uint32_t factory_address) override;
    void SetRecordErrorFlag(std::uint32_t record_address,
                            std::uint8_t value) override;
    void FreeMemory(std::uint32_t address) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override;
    std::uint32_t TreeFirst(std::uint32_t tree_address) override;
    std::uint32_t TreeNext(std::uint32_t node_address) override;
    std::uint32_t TreeNilAddress() override;
    void TreeEraseSubtree(std::uint32_t tree_address,
                          std::uint32_t root_address) override;
    void TreeRemoveNode(std::uint32_t tree_address,
                        std::uint32_t node_address) override;
    void ReleaseSharedTreeNil() override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void FreeStringStorage(std::uint32_t record_address,
                           std::uint32_t allocation_address,
                           std::uint32_t bytes) override;
    std::uint32_t ResetBaseObject(MonsterBehaviorManagerState& state,
                                  std::uint32_t address) override;
    std::uint32_t ProcessManagerState(
        MonsterBehaviorManagerState& state, std::uint32_t address,
        float elapsed) override;
    void CheckStateAndProcessCollision(MonsterBehaviorManagerState& state,
                                       std::uint32_t address, float elapsed) override;
    IScriptEventSchedulerRuntime& SchedulerRuntime() override;
    std::uint32_t UpdatePosition(std::uint32_t address, float elapsed,
                                 std::uint32_t argument) override;
    void CleanupScriptObjectCollection(std::uint32_t address) override;
    std::uint32_t XmlChildByIndex(std::uint32_t parent,
                                  std::uint32_t index) override;
    void LoadFuzzyBehaviorConfig(std::uint32_t manager_address,
                                 std::uint32_t child_address) override;
    void DispatchComponentField(std::uint32_t manager_address,
                                std::uint32_t virtual_slot,
                                std::uint32_t field_address) override;
    std::uint32_t GetConfigAttribute(std::uint32_t config_address,
                                     const char* name) override;
    std::int32_t ParseConfigInt(std::uint32_t attribute_address,
                                std::int32_t fallback) override;
    std::uint32_t CallActionSlot4(std::uint32_t action_address) override;
    std::uint32_t SelectAnimation(std::uint32_t animation_address,
                                  const char* name) override;
    std::uint32_t ProcessCollisionAndConfig(
        std::uint32_t manager_address, float incoming_st0,
        float constant_one) override;

private:
    ScriptEventProcessorRuntimeStub processor_{};
    ScriptEventSchedulerRuntimeStub scheduler_{};
    ScriptObjectRuntimeStub script_{};
    MonsterFuzzyBehaviorRuntimeStub fuzzy_{};
};

class IMonsterPursuitResetRuntime {
public:
    virtual ~IMonsterPursuitResetRuntime() = default;
    [[nodiscard]] virtual std::string_view AnimationName(
        std::uint32_t animation_address) = 0;
    [[nodiscard]] virtual std::uint32_t SelectAnimation(
        std::uint32_t animation_address, const char* name) = 0;
    [[nodiscard]] virtual IScriptEventProcessorRuntime& ProcessorRuntime() = 0;
    virtual void CleanupScriptObjectCollection(std::uint32_t address) = 0;
    virtual void LoadFuzzyBehaviorConfig(
        std::uint32_t manager_address, std::uint32_t child_address) = 0;
    virtual void DispatchComponentField(
        std::uint32_t manager_address, std::uint32_t virtual_slot,
        std::uint32_t field_address, std::uint32_t argument) = 0;
};

class IMonsterPursuitRuntime : public IMonsterPursuitResetRuntime {
public:
    virtual ~IMonsterPursuitRuntime() = default;
    [[nodiscard]] virtual bool CheckProximity(
        MonsterBehaviorManagerState& state, std::uint32_t manager_address,
        float elapsed) = 0;
    [[nodiscard]] virtual bool PlayerControlBusy() = 0;
    [[nodiscard]] virtual bool CheckDistanceAndFlag(
        const MonsterBehaviorManagerState& state,
        std::uint32_t manager_address, float elapsed) = 0;
    [[nodiscard]] virtual bool CheckTargetReached(
        const MonsterBehaviorManagerState& state,
        std::uint32_t manager_address, float elapsed) = 0;
    virtual void ProcessCollisionAndConfig(
        MonsterBehaviorManagerState& state,
        std::uint32_t manager_address, float elapsed) = 0;
    [[nodiscard]] virtual bool ProcessPursuitCollisionAndSpeed(
        const MonsterBehaviorManagerState& state,
        std::uint32_t manager_address, float elapsed,
        std::uint32_t target_word) = 0;
    [[nodiscard]] virtual bool ProcessObjectInteraction(
        MonsterBehaviorManagerState& state, std::uint32_t manager_address,
        std::uint32_t target_word, float elapsed) = 0;
    virtual void ProcessConfigAndEvents(
        std::uint32_t manager_address, float elapsed) = 0;
    virtual void InitializeCombatSystem(
        std::uint32_t manager_address, std::int32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t ResetPursuit(
        MonsterBehaviorManagerState& state, std::uint32_t manager_address) = 0;
    virtual void SetRecordErrorFlag(
        std::uint32_t record_address, std::uint8_t flag) = 0;
    virtual void ProcessActionWithScript(
        std::uint32_t manager_address, float elapsed) = 0;
    virtual void VisitArenaObjects(std::uint32_t manager_address) = 0;
};

class MonsterPursuitRuntimeStub final : public IMonsterPursuitRuntime {
public:
    std::string_view AnimationName(std::uint32_t animation_address) override;
    std::uint32_t SelectAnimation(std::uint32_t animation_address,
                                  const char* name) override;
    bool CheckProximity(MonsterBehaviorManagerState& state,
                        std::uint32_t manager_address, float elapsed) override;
    bool PlayerControlBusy() override;
    bool CheckDistanceAndFlag(const MonsterBehaviorManagerState& state,
                              std::uint32_t manager_address,
                              float elapsed) override;
    bool CheckTargetReached(const MonsterBehaviorManagerState& state,
                            std::uint32_t manager_address,
                            float elapsed) override;
    void ProcessCollisionAndConfig(MonsterBehaviorManagerState& state,
                                   std::uint32_t manager_address,
                                   float elapsed) override;
    bool ProcessPursuitCollisionAndSpeed(
        const MonsterBehaviorManagerState& state,
        std::uint32_t manager_address, float elapsed,
        std::uint32_t target_word) override;
    bool ProcessObjectInteraction(MonsterBehaviorManagerState& state,
                                  std::uint32_t manager_address,
                                  std::uint32_t target_word,
                                  float elapsed) override;
    void ProcessConfigAndEvents(std::uint32_t manager_address,
                                float elapsed) override;
    void InitializeCombatSystem(std::uint32_t manager_address,
                                std::int32_t argument) override;
    std::uint32_t ResetPursuit(MonsterBehaviorManagerState& state,
                               std::uint32_t manager_address) override;
    IScriptEventProcessorRuntime& ProcessorRuntime() override;
    void CleanupScriptObjectCollection(std::uint32_t address) override;
    void LoadFuzzyBehaviorConfig(std::uint32_t manager_address,
                                 std::uint32_t child_address) override;
    void DispatchComponentField(std::uint32_t manager_address,
                                std::uint32_t virtual_slot,
                                std::uint32_t field_address,
                                std::uint32_t argument) override;
    void SetRecordErrorFlag(std::uint32_t record_address,
                            std::uint8_t flag) override;
    void ProcessActionWithScript(std::uint32_t manager_address,
                                 float elapsed) override;
    void VisitArenaObjects(std::uint32_t manager_address) override;

private:
    ScriptEventProcessorRuntimeStub processor_{};
    MonsterProximityRuntimeStub proximity_{};
    MonsterInteractionRuntimeStub interaction_{};
    MonsterPursuitSpeedRuntimeStub speed_{};
    MonsterFuzzyBehaviorRuntimeStub fuzzy_{};
    MonsterActionDispatchRuntimeStub action_{};
};

class MonsterBehaviorManager final {
public:
    static constexpr std::uint32_t kBytes = 0x73cu;
    static constexpr std::uint32_t kProcessorOffset = 0x658u;
    static constexpr std::uint32_t kScriptOffset = 0x688u;
    static constexpr std::uint32_t kTreeOffset = 0x69cu;
    static constexpr std::uint32_t kAuxiliaryOffset = 0x6d4u;
    static constexpr std::array<std::uint32_t, 4> kFactories{
        0x429c70u, 0x429a30u, 0x42a130u, 0x42a360u};

    // Existing legacy history-image boundary shared by entry and frame cleanup.
    static void ClearHistoryRecord(std::uint32_t record,
                                    IMonsterPursuitEntryRuntime& runtime);

    [[nodiscard]] static std::uint32_t Construct(
        MonsterBehaviorManagerState& state, std::uint32_t address,
        IMonsterBehaviorManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructMembers(
        MonsterBehaviorManagerState&,std::uint32_t,IMonsterBehaviorConstructionRuntime&);
    static std::uint32_t SetComponentMembers(MonsterBehaviorManagerState&,std::uint32_t manager,
        std::uint32_t source,bool dispatch_fields,bool replace,IMonsterBehaviorComponentRuntime&);
    static void Destroy(MonsterBehaviorManagerState& state,
                        std::uint32_t address,
                        IMonsterBehaviorManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t Reset(
        MonsterBehaviorManagerState& state, std::uint32_t address,
        IMonsterBehaviorManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupBuffers(
        MonsterBehaviorManagerState& state, std::uint32_t address,
        bool notify, IMonsterBehaviorManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t Update(
        MonsterBehaviorManagerState& state, std::uint32_t address,
        float elapsed, std::uint32_t argument,
        IMonsterBehaviorFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeComponent(
        MonsterBehaviorManagerState& state, std::uint32_t address,
        std::uint32_t source_address, bool dispatch_fields,
        IMonsterBehaviorManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReplaceComponent(
        MonsterBehaviorManagerState& state, std::uint32_t address,
        std::uint32_t source_address, bool dispatch_fields,
        IMonsterBehaviorManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessStorageAction(
        MonsterBehaviorManagerState& state, std::uint32_t action_address,
        IMonsterBehaviorManagerRuntime& runtime);
    // IDA 0x433680: enters state 1 and resets transient script data.
    [[nodiscard]] static std::uint32_t SetStateAndCleanup(
        MonsterBehaviorManagerState& state, std::uint32_t address,
        std::uint32_t argument, IMonsterPursuitEntryRuntime& runtime);
    // IDA 0x433830 and shared tail 0x433800.
    [[nodiscard]] static std::uint32_t SetStopPursuitFlag(
        MonsterBehaviorManagerState& state, std::uint32_t address,
        float incoming_st0, IMonsterStopPursuitRuntime& runtime);
    // IDA 0x433840.
    static void ClearStopPursuitFlag(MonsterBehaviorManagerState& state);
    // IDA 0x433850: three pursuit states and unconditional arena traversal.
    [[nodiscard]] static bool UpdatePursuitState(
        MonsterBehaviorManagerState& state, std::uint32_t address,
        float elapsed, IMonsterPursuitRuntime& runtime);
    // IDA 0x435300: update the target only during active pursuit.
    [[nodiscard]] static std::uint32_t SetPursuitTargetIfActive(
        MonsterBehaviorManagerState& state, std::uint32_t target);
};

class MonsterPursuitReset final {
public:
    // IDA 0x4344f0: reload behavior and restore the idle animation.
    [[nodiscard]] static std::uint32_t Reset(
        MonsterBehaviorManagerState& state, std::uint32_t address,
        IMonsterPursuitResetRuntime& runtime);
};

}  // namespace pal4
