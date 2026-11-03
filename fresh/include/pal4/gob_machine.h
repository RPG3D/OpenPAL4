#pragma once

#include "pal4/gob_resource_cleanup.h"

#include <cstdint>
#include <array>
#include <string_view>

namespace pal4 {

class IGobMachineTaskRuntime;
class IGobMachineControlRuntime;
class IGobMachineConfigRuntime;
class IGobMachineVisibilityRuntime;
class IGobMachineInventoryRuntime;
class IGobMachineSnapshotRuntime;

class IGobMachineConstructRuntime {
public:
    virtual ~IGobMachineConstructRuntime() = default;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void DestructBase(std::uint32_t object) = 0;
    virtual void InitializeAnimationState(std::uint32_t state) = 0;
    virtual void CleanupAnimationState(std::uint32_t state) = 0;
    virtual void InitializeTaskContainer(std::uint32_t container) = 0;
    virtual void DestroyTaskContainer(std::uint32_t container) = 0;
    virtual void InitializeTaskArrayRecord(std::uint32_t record) = 0;
    virtual void InitializeStringRecord(std::uint32_t record) = 0;
    virtual void ReleaseStringRecord(std::uint32_t record) = 0;
    virtual void FreeTaskArray(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

// Per-frame composition is independent of construction and package loading.
class IGobMachineFrameRuntime {
public:
    virtual ~IGobMachineFrameRuntime() = default;
    virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,std::uint32_t value) = 0;
    virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void UpdateBaseClump(float elapsed) = 0;
    virtual void UpdateAnimation(std::uint32_t state, float elapsed) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateTaskContainer(
        std::uint32_t container, float elapsed) = 0;
    [[nodiscard]] virtual std::uint32_t TriggerTaskRun(
        std::uint32_t object, const std::array<float, 3>& position) = 0;
    virtual void ReaddActiveSceneObject(std::uint32_t object,
                                        float weight) = 0;
    [[nodiscard]] virtual std::uint32_t RefreshMinimap() = 0;
};

class IGobMachineRuntime : public IGobMachineConstructRuntime, public IGobMachineCleanupRuntime, public IGobMachineFrameRuntime {
public:
    ~IGobMachineRuntime() override = default;
    virtual std::uint32_t ReadWord(std::uint32_t) override = 0;
    virtual void WriteWord(std::uint32_t,std::uint32_t) override = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CleanupAnimationData(std::uint32_t state) = 0;
    virtual void ClearTaskContainer(std::uint32_t container) = 0;
    virtual void CleanupBaseResources(std::uint32_t object) = 0;
    virtual void CleanupBeforeSceneLoad(std::uint32_t object) = 0;
    [[nodiscard]] virtual bool UsesObjectPackage(std::string_view path) = 0;
    [[nodiscard]] virtual bool UsesWeaponPackage(std::string_view path) = 0;
    virtual void LoadObjectPackage() = 0;
    virtual void LoadWeaponPackage() = 0;
    [[nodiscard]] virtual std::uint32_t CurrentScenePackage() = 0;
    virtual void LoadScenePackage(std::uint32_t package) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeBaseModel(
        std::uint32_t object, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth) = 0;
    virtual void HideHookUserDataAtomics(std::uint32_t clump) = 0;
    virtual void LoadAnimationData(std::uint32_t state,
                                   std::uint32_t clump,
                                   std::string_view path,
                                   std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t FindXmlChild(
        std::uint32_t parent, std::string_view name) = 0;
    virtual void RemoveXmlChild(std::uint32_t parent,
                                std::uint32_t child) = 0;
    [[nodiscard]] virtual std::uint32_t CreateXmlChild(
        std::uint32_t parent, std::string_view name) = 0;
    virtual void ClearAnimationFlag(std::uint32_t state,
                                    std::uint32_t flag) = 0;
    virtual void SetAnimationFlag(std::uint32_t state,
                                  std::uint32_t flag) = 0;
    virtual void CleanupScriptPackages() = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
};

class GobMachine final {
public:
    static constexpr std::uint32_t kSize = 0x2ccu;
    static constexpr std::uint32_t kVtable = 0x848f58u;
    static constexpr std::uint32_t kStateVtable = 0x848f4cu;
    static constexpr std::uint32_t kStateOffset = 0x1dcu;
    static constexpr std::uint32_t kPrimaryTasksOffset = 0x258u;
    static constexpr std::uint32_t kContextTasksOffset = 0x268u;

    [[nodiscard]] static std::uint32_t Create(IGobMachineRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IGobMachineConstructRuntime& runtime);
    [[nodiscard]] static std::uint32_t TypeKey();
    static void Destruct(std::uint32_t object,
                         IGobMachineConstructRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupResources(
        std::uint32_t object, IGobMachineCleanupRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateRuntimeAndTasks(
        std::uint32_t object, float elapsed,
        IGobMachineFrameRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeSceneAndConfig(
        std::uint32_t object, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth, IGobMachineRuntime& runtime);
    [[nodiscard]] static std::uint32_t BuildAndStartTaskRun(
        std::uint32_t object, IGobMachineTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t StartAndSaveTransform(
        std::uint32_t object, IGobMachineControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetTriggeredRuntime(
        std::uint32_t object, IGobMachineControlRuntime& runtime);
    [[nodiscard]] static bool AreTaskContainersReady(
        std::uint32_t object, IGobMachineControlRuntime& runtime);
    [[nodiscard]] static std::uint8_t InvokeTaskSlot28Range(
        std::uint32_t object, IGobMachineControlRuntime& runtime);
    [[nodiscard]] static std::uint8_t InvokeTaskSlot32Range(
        std::uint32_t object, IGobMachineControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetMachineConfigNode(
        std::uint32_t object, IGobMachineConfigRuntime& runtime);
    [[nodiscard]] static std::uint32_t LoadMainConfigAndTaskFlags(
        std::uint32_t object, IGobMachineConfigRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyVisibleFlagAndRepeatTrigger(
        std::uint32_t object, std::uint32_t visible,
        IGobMachineVisibilityRuntime& runtime);
    [[nodiscard]] static std::uint32_t CheckAndConsumeInventoryConditions(
        std::uint32_t object, IGobMachineInventoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateStateSnapshot(
        std::uint32_t object, IGobMachineSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyStateSnapshotEvent(
        std::uint32_t object, std::uint32_t snapshot,
        IGobMachineSnapshotRuntime& runtime);
};

}  // namespace pal4
