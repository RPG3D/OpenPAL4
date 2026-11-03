#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

// Narrow endpoints keep package lifecycle independent of actor/SDK services.
class IGobLocalFloatMemoryRuntime {
public:
    struct Vector3 {
        float x;
        float y;
        float z;
    };
    struct DebugVertex {
        Vector3 position;
        std::uint32_t color;
    };
    virtual ~IGobLocalFloatMemoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t memory) = 0;
    [[nodiscard]] virtual std::uint32_t FreePathBuffer(
        std::uint32_t memory) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
};

class IGobLocalFloatLifecycleRuntime : public virtual IGobLocalFloatMemoryRuntime {
public:
    virtual ~IGobLocalFloatLifecycleRuntime() = default;
    virtual void ConstructMachine(std::uint32_t object) = 0;
    virtual void DestructMachine(std::uint32_t object) = 0;
};

class IGobLocalFloatCleanupRuntime : public virtual IGobLocalFloatMemoryRuntime {
public:
    virtual ~IGobLocalFloatCleanupRuntime() = default;
    virtual void CleanupMachineRuntime(std::uint32_t object,
                                       std::uint32_t context) = 0;
};

class IGobLocalFloatFrameRuntime : public virtual IGobLocalFloatMemoryRuntime {
public:
    virtual ~IGobLocalFloatFrameRuntime() = default;
    virtual void UpdateMachineTasks(std::uint32_t object,
                                    float elapsed) = 0;
    virtual void StorePlatformDelta(const Vector3& translation,
                                    float rotation) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControlAccess() = 0;
    virtual void ApplyActorTranslation(std::uint32_t actor,
                                       const Vector3& translation,
                                       std::uint32_t mode) = 0;
    virtual void MatrixTranslate(std::uint32_t matrix,
                                 const Vector3& translation,
                                 std::uint32_t mode) = 0;
    virtual void MatrixRotateY(std::uint32_t matrix, float angle,
                               std::uint32_t mode) = 0;
    virtual void FrameUpdateObjects(std::uint32_t frame) = 0;
    virtual void MatrixQueryRotation(std::uint32_t matrix,
                                     float& axis_y,
                                     float& angle) = 0;
    virtual void SetActorPosition(std::uint32_t actor,
                                  const Vector3& position) = 0;
    virtual void SetActorTransform(std::uint32_t actor,
                                   const Vector3& transform,
                                   std::uint32_t mode) = 0;
    virtual void UpdateMinimapGrid(std::uint32_t actor_position,
                                   std::uint32_t actor_transform) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateFloorAttributes() = 0;
};

class IGobLocalFloatConfigRuntime : public virtual IGobLocalFloatMemoryRuntime {
public:
    virtual ~IGobLocalFloatConfigRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetMachineConfigNode(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t FindXmlChild(
        std::uint32_t parent, std::string_view name) = 0;
    [[nodiscard]] virtual std::int32_t XmlChildCount(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t XmlChildAt(
        std::uint32_t node, std::int32_t index) = 0;
    [[nodiscard]] virtual std::int32_t ReadXmlInt(
        std::uint32_t node, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual float ReadXmlFloat(
        std::uint32_t node, std::string_view key,
        float fallback) = 0;
    virtual void LoadMachineMainConfig(std::uint32_t object) = 0;
};

class IGobLocalFloatTouchRuntime : public virtual IGobLocalFloatMemoryRuntime {
public:
    virtual ~IGobLocalFloatTouchRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InvokeMachineTrigger(
        std::uint32_t object, const Vector3& value) = 0;
};

class IGobLocalFloatVisibilityRuntime : public virtual IGobLocalFloatTouchRuntime {
public:
    virtual ~IGobLocalFloatVisibilityRuntime() = default;
    virtual void ApplyMachineVisibleFlag(std::uint32_t object,
                                         std::uint32_t visible) = 0;
};

class IGobLocalFloatTriggerRuntime : public virtual IGobLocalFloatMemoryRuntime {
public:
    virtual ~IGobLocalFloatTriggerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t TryTriggerMachineTask(
        std::uint32_t object, std::uint32_t argument) = 0;
};

class IGobLocalFloatControlRuntime : public virtual IGobLocalFloatMemoryRuntime {
public:
    virtual ~IGobLocalFloatControlRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ForwardHandleEvent(
        std::uint32_t object, std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t ResetTriggeredMachine(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint8_t StopTaskSounds(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint8_t ResumeTaskSounds(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual bool AreTaskContainersReady(
        std::uint32_t object) = 0;
};

class IGobLocalFloatStateRuntime : public virtual IGobLocalFloatMemoryRuntime {
public:
    virtual ~IGobLocalFloatStateRuntime() = default;
    [[nodiscard]] virtual std::string_view ReadObjectName(
        std::uint32_t address) = 0;
    virtual void ConstructSnapshot(std::uint32_t snapshot) = 0;
    virtual void AssignSnapshotName(std::uint32_t snapshot,
                                    std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t SnapshotType(
        std::uint32_t snapshot) = 0;
    virtual void SetBaseVisible(std::uint32_t object,
                                std::uint32_t visible) = 0;
    virtual void SetBasePosition(std::uint32_t object,
                                 const Vector3& position,
                                 std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t SetBaseRotation(
        std::uint32_t object, const Vector3& rotation,
        std::uint32_t mode) = 0;
};

class IGobLocalFloatMovementRuntime : public virtual IGobLocalFloatMemoryRuntime {
public:
    virtual ~IGobLocalFloatMovementRuntime() = default;
    [[nodiscard]] virtual std::uint32_t MovementAdapterRoot() = 0;
    [[nodiscard]] virtual std::uint32_t MovementProperty(
        std::uint32_t object) = 0;
    virtual void SetClumpProperties(
        std::uint32_t clump_manager, std::uint32_t clump,
        std::uint32_t property, std::uint32_t first,
        std::uint32_t second, std::uint32_t value) = 0;
    virtual void SetClumpRenderCallback(
        std::uint32_t clump_manager,
        std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint32_t AttachMovementHelper(
        std::uint32_t adapter, std::uint32_t helper) = 0;
    [[nodiscard]] virtual std::uint32_t DetachMovementHelper(
        std::uint32_t adapter, std::uint32_t helper) = 0;
};

class IGobLocalFloatRuntime : public IGobLocalFloatLifecycleRuntime,
    public IGobLocalFloatCleanupRuntime,
    public IGobLocalFloatVisibilityRuntime,
    public IGobLocalFloatFrameRuntime,
    public IGobLocalFloatConfigRuntime,
    public IGobLocalFloatControlRuntime,
    public IGobLocalFloatTriggerRuntime,
    public IGobLocalFloatStateRuntime,
    public IGobLocalFloatMovementRuntime {
public:
    ~IGobLocalFloatRuntime() override = default;
    virtual void SetRenderState(std::uint32_t state,
                                std::uint32_t value) = 0;
    virtual void SampleCircle(float angle, float& x,
                              float& z) = 0;
    [[nodiscard]] virtual bool TransformDebugVertices(
        std::span<const DebugVertex> vertices,
        std::uint32_t mode) = 0;
    virtual void RenderDebugPrimitive(std::uint32_t type) = 0;
    virtual void EndDebugPrimitive() = 0;
    virtual void CleanupBeforeSceneLoad(std::uint32_t object) = 0;
    [[nodiscard]] virtual bool UsesObjectPackage(
        std::string_view path) = 0;
    [[nodiscard]] virtual bool UsesWeaponPackage(
        std::string_view path) = 0;
    virtual void LoadObjectPackage() = 0;
    virtual void LoadWeaponPackage() = 0;
    [[nodiscard]] virtual std::uint32_t CurrentScenePackage() = 0;
    virtual void LoadScenePackage(std::uint32_t package) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeMachineAndConfig(
        std::uint32_t object, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth) = 0;
    virtual void CleanupScriptPackages() = 0;
    virtual void RemoveXmlChild(std::uint32_t parent,
                                std::uint32_t child) = 0;
    [[nodiscard]] virtual std::uint32_t CreateXmlChild(
        std::uint32_t parent, std::string_view name) = 0;
};

class GobLocalFloat final {
public:
    static constexpr std::uint32_t kSize = 0x2e4u;
    static constexpr std::uint32_t kVtable = 0x848d08u;
    static constexpr std::uint32_t kMachineMemberVtable = 0x848cf8u;
    static constexpr std::uint32_t kHelperVtable = 0x848ce4u;
    static constexpr std::uint32_t kHelperBaseVtable = 0x848da8u;

    [[nodiscard]] static std::uint32_t Create(
        IGobLocalFloatLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IGobLocalFloatLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t PrimaryTaskContainer(
        std::uint32_t object);
    [[nodiscard]] static double InteractionThreshold();
    static void Destruct(std::uint32_t object,
                         IGobLocalFloatLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupAndReset(
        std::uint32_t object, std::uint32_t context,
        IGobLocalFloatCleanupRuntime& runtime);
    [[nodiscard]] static std::uint32_t HandleEvent(
        std::uint32_t object, std::uint32_t argument,
        IGobLocalFloatControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateMachineAndActor(
        std::uint32_t object, float elapsed,
        IGobLocalFloatFrameRuntime& runtime);
    static void RenderDebugPath(std::uint32_t object,
                                IGobLocalFloatRuntime& runtime);
    [[nodiscard]] static std::uint32_t RebuildMachineTaskPathPoints(
        std::uint32_t object, IGobLocalFloatConfigRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeSceneAndConfig(
        std::uint32_t object, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth, IGobLocalFloatRuntime& runtime);
    [[nodiscard]] static std::uint32_t AttachMovementAdapter(
        std::uint32_t object, IGobLocalFloatMovementRuntime& runtime);
    [[nodiscard]] static std::uint32_t DetachMovementAdapter(
        std::uint32_t object, IGobLocalFloatMovementRuntime& runtime);
    [[nodiscard]] static std::uint32_t TryTriggerByTouchTypeOrUseCount(
        std::uint32_t object, std::uint32_t argument,
        IGobLocalFloatTriggerRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetMachineAndTouchLatch(
        std::uint32_t object, IGobLocalFloatControlRuntime& runtime);
    [[nodiscard]] static std::uint8_t StopTaskSounds(
        std::uint32_t object, IGobLocalFloatControlRuntime& runtime);
    [[nodiscard]] static std::uint8_t ResumeTaskSounds(
        std::uint32_t object, IGobLocalFloatControlRuntime& runtime);
    [[nodiscard]] static bool AreTaskContainersReady(
        std::uint32_t object, IGobLocalFloatControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetConfigNode(
        std::uint32_t object, IGobLocalFloatConfigRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReadConfigAndMachineFlags(
        std::uint32_t object, IGobLocalFloatConfigRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyVisibleAndAutoTouch(
        std::uint32_t object, std::uint32_t visible,
        IGobLocalFloatVisibilityRuntime& runtime);
    [[nodiscard]] static std::uint32_t HelperClump(
        std::uint32_t object, IGobLocalFloatMemoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessHelperTouchState(
        std::uint32_t object, IGobLocalFloatTouchRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetHelperTouchState(
        std::uint32_t object, IGobLocalFloatMemoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateStateSnapshot(
        std::uint32_t object, IGobLocalFloatStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteHelper(
        std::uint32_t helper, std::uint32_t flags,
        IGobLocalFloatMemoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyStateSnapshot(
        std::uint32_t object, std::uint32_t snapshot,
        IGobLocalFloatStateRuntime& runtime);
};

}  // namespace pal4
