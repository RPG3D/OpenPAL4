#pragma once

#include "pal4/monster_behavior_manager.h"
#include "pal4/native_base_object_member_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_monster_behavior_tree_runtime.h"
#include "pal4/native_monster_encounter_storage.h"
#include "pal4/native_script_object_runtime.h"

#include <span>
#include <functional>
#include <string_view>

namespace pal4 {

class BinaryXmlConfig;
class IBaseObjectActivityRuntime;
class IGameResourceManagerRuntime;

// Binds the original monster constructor/destructor to owned native members.
class NativeMonsterBehaviorRuntime final :
    public MonsterBehaviorManagerRuntimeStub {
public:
    NativeMonsterBehaviorRuntime(NativeBaseObjectMemory& memory,
                                 IRwDictSchemaRuntime& schema,
                                 IGameObjectEmbeddedAnimationRuntime& animation)
        : memory_(memory), base_(memory, schema, animation, memory,
                                  memory.Path()), document_(memory),
          script_(memory), tree_(memory) {}

    using ComponentFieldDispatch = std::function<void(std::uint32_t,
        std::uint32_t, std::uint32_t)>;
    // Scoped virtual +4/+8 dispatch for shared script component operations.
    std::uint32_t WithComponentFieldDispatch(const ComponentFieldDispatch& dispatch,
        const std::function<std::uint32_t()>& operation);
    IScriptEventProcessorRuntime& ProcessorRuntime() override {
        return document_;
    }
    IScriptObjectRuntime& ScriptRuntime() override { return script_; }
    void ConstructBase(MonsterBehaviorManagerState& state,
                       std::uint32_t address) override;
    void DestructBase(MonsterBehaviorManagerState& state,
                      std::uint32_t address) override;
    std::uint32_t ResetActivity(MonsterBehaviorManagerState& state,std::uint32_t address,
        IBaseObjectActivityRuntime& activity,IGameResourceManagerRuntime& transforms);
    std::uint32_t ResetBaseObject(MonsterBehaviorManagerState& state,std::uint32_t address) override;
    void InitializeTree(MonsterBehaviorManagerState& state,
                        std::uint32_t address) override;
    void CleanupTree(MonsterBehaviorManagerState& state,
                     std::uint32_t address) override;
    void DestroyAuxiliary(MonsterBehaviorManagerState& state,
                          std::uint32_t address) override;
    void InsertBehavior(std::uint32_t tree_address,
                        std::uint32_t class_id,
                        std::uint32_t factory_address) override;
    void FreeMemory(std::uint32_t address) override;
    bool ReleaseNativeHistoryRecord(std::uint32_t record) override {
        return encounters_.ReleaseHistoryRecord(record);
    }
    [[nodiscard]] NativeMonsterEncounterStorage& Encounters() noexcept { return encounters_; }
    void SetRecordErrorFlag(std::uint32_t address, std::uint8_t flag) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void FreeStringStorage(std::uint32_t record, std::uint32_t allocation,
                            std::uint32_t bytes) override;
    std::uint32_t TreeFirst(std::uint32_t tree_address) override;
    std::uint32_t TreeNext(std::uint32_t node_address) override;
    std::uint32_t TreeNilAddress() override;
    void TreeEraseSubtree(std::uint32_t tree_address,
                          std::uint32_t root_address) override;
    void TreeRemoveNode(std::uint32_t tree_address,
                        std::uint32_t node_address) override;
    void ReleaseSharedTreeNil() override;
    void BindBehaviorDocument(std::uint32_t address,
                              const BinaryXmlConfig& document);
    std::uint32_t XmlChildByIndex(std::uint32_t parent,
                                 std::uint32_t index) override;
    void LoadFuzzyBehaviorConfig(std::uint32_t manager_address,
                                 std::uint32_t child_address) override;
    void CleanupScriptObjectCollection(std::uint32_t address) override;
    void DispatchComponentField(std::uint32_t manager_address,
                                std::uint32_t virtual_slot,
                                std::uint32_t field_address) override;
    std::uint32_t ProcessManagerState(MonsterBehaviorManagerState& state,
                                      std::uint32_t address,
                                      float elapsed) override;
    void CheckStateAndProcessCollision(MonsterBehaviorManagerState& state,
                                       std::uint32_t address, float elapsed) override;
    [[nodiscard]] NativeEventDocumentRuntime& EventDocuments() noexcept { return document_; }
    [[nodiscard]] NativeScriptObjectRuntime& Tasks() noexcept { return script_; }
    std::uint32_t UpdatePosition(std::uint32_t address, float elapsed,
                                 std::uint32_t argument) override;
    [[nodiscard]] std::uint32_t AddScriptEvent(
        MonsterBehaviorManagerState& state, const char* name);
    void SetEventAtomicNodeNames(
        std::uint32_t event, std::span<const std::string_view> names) {
        document_.SetAtomicNodeNames(event, names);
    }
    void AddEventAtomicIntAttribute(std::uint32_t event, std::uint32_t index,
                                    std::string_view name, std::int32_t value) {
        document_.AddAtomicIntAttribute(event, index, name, value);
    }
    void AddEventAtomicFloatAttribute(std::uint32_t event, std::uint32_t index,
                                      std::string_view name, float value) {
        document_.AddAtomicFloatAttribute(event, index, name, value);
    }

    [[nodiscard]] std::uint32_t DocumentCount() const noexcept {
        return document_.DocumentCount();
    }
    [[nodiscard]] std::uint32_t DocumentNil() const noexcept {
        return document_.SharedNil();
    }
    [[nodiscard]] std::uint32_t BehaviorTreeNil() { return tree_.SharedNil(); }
    [[nodiscard]] std::uint32_t BehaviorTreeReferences() {
        return tree_.SharedReferenceCount();
    }

private:
    const ComponentFieldDispatch* component_dispatch_{};
    NativeBaseObjectMemory& memory_;
    NativeBaseObjectMemberRuntime base_;
    NativeEventDocumentRuntime document_;
    NativeScriptObjectRuntime script_;
    NativeMonsterBehaviorTreeRuntime tree_;
    NativeMonsterEncounterStorage encounters_;
    std::uint32_t behavior_document_address_ = 0u;
    const BinaryXmlConfig* behavior_document_ = nullptr;
    IBaseObjectActivityRuntime* reset_activity_ = nullptr;
    IGameResourceManagerRuntime* reset_transforms_ = nullptr;
};

}  // namespace pal4
