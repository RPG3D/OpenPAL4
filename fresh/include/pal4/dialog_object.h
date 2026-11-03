#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace pal4 {

struct DialogObjectState final {
    std::array<std::uint32_t, 55u> words{};
};
static_assert(sizeof(DialogObjectState) == 0xdcu);

class IDialogObjectTransformRuntime {
public:
    virtual ~IDialogObjectTransformRuntime() = default;
    [[nodiscard]] virtual int RefreshTransform(DialogObjectState& state) = 0;
    [[nodiscard]] virtual std::uint32_t FrameParent(std::uint32_t frame) = 0;
    [[nodiscard]] virtual int RemoveFrameChild(std::uint32_t frame) = 0;
    [[nodiscard]] virtual int AddFrameChild(std::uint32_t parent,
                                           std::uint32_t frame) = 0;
};

class IDialogObjectRuntime : public IDialogObjectTransformRuntime {
public:
    virtual ~IDialogObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void WriteHeapWord(std::uint32_t address, std::uint32_t offset,
                               std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadHeapWord(
        std::uint32_t address, std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeTextManager(
        std::uint32_t address) = 0;
    virtual void DestroyTextManager(std::uint32_t address) = 0;
    virtual void InitializeXmlDocument(DialogObjectState& state) = 0;
    virtual void DestroyXmlDocument(DialogObjectState& state) = 0;
    [[nodiscard]] virtual std::uint8_t ConstructorStackByte() = 0;
    virtual void DestroyFirstTreeBranch(std::uint32_t root) = 0;
    virtual void DestroySecondTreeBranch(std::uint32_t root) = 0;
    [[nodiscard]] virtual std::uint32_t NextFirstTreeNode(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t NextSecondTreeNode(
        std::uint32_t node) = 0;
    virtual void EraseFirstTreeNode(std::uint32_t node) = 0;
    virtual void EraseSecondTreeNode(std::uint32_t node) = 0;
    virtual void DestroyStringRecord(std::uint32_t address) = 0;
    virtual void CleanupBeforeDelete(std::uint32_t object,
                                     std::uint32_t first,
                                     std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t VirtualDelete(
        std::uint32_t object, std::uint8_t flags) = 0;
};

class IDialogObjectPropertyRuntime {
public:
    virtual ~IDialogObjectPropertyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FirstTreeCandidate(
        const DialogObjectState& state, std::string_view key) = 0;
    [[nodiscard]] virtual std::uint32_t SecondTreeCandidate(
        const DialogObjectState& state, std::string_view key) = 0;
    [[nodiscard]] virtual int CompareKeyToNode(
        std::string_view key, std::uint32_t node) = 0;
    [[nodiscard]] virtual std::array<float, 3u> ReadStoredVector(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t ReadFacetHandle(
        std::uint32_t node) = 0;
    virtual void ReadFacetVector(std::uint32_t facet,
                                 std::array<float, 3u>& output) = 0;
    virtual void WriteFacetHandle(std::uint32_t node,
                                  std::uint32_t facet) = 0;
    virtual void InsertFacet(DialogObjectState& state, std::string_view key,
                             std::uint32_t facet) = 0;
};

class IDialogObjectFacetCleanupRuntime {
public:
    virtual ~IDialogObjectFacetCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SharedFirstTreeSentinel() = 0;
    [[nodiscard]] virtual std::uint32_t ReadNodeWord(
        std::uint32_t node, std::uint32_t offset) = 0;
    virtual void WriteNodeWord(std::uint32_t node, std::uint32_t offset,
                               std::uint32_t value) = 0;
    virtual void DeleteFirstTreeSubtree(std::uint32_t root) = 0;
    virtual void ReleaseNodeText(std::uint32_t address) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t NextFirstTreeNode(
        std::uint32_t node) = 0;
    virtual void EraseFirstTreeNode(std::uint32_t node) = 0;
};

class IDialogObjectEffectCleanupRuntime {
public:
    virtual ~IDialogObjectEffectCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address,
                                                  std::uint32_t offset) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t offset,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual bool EffectsEnabled() = 0;
    [[nodiscard]] virtual bool EffectIsActive(std::uint32_t effect) = 0;
    [[nodiscard]] virtual std::uint32_t EffectFrame(
        std::uint32_t effect) = 0;
    [[nodiscard]] virtual std::uint32_t FrameParent(
        std::uint32_t frame) = 0;
    virtual void RemoveFrameChild(std::uint32_t frame) = 0;
    virtual void ProcessEffectEvent(std::uint32_t owner,
                                    std::uint32_t effect) = 0;
    virtual void DestroyEffect(std::uint32_t effect) = 0;
};

class IDialogObjectPreDeleteRuntime : public IDialogObjectEffectCleanupRuntime {
public:
    ~IDialogObjectPreDeleteRuntime() override = default;
    virtual void ResetProperties(DialogObjectState& state) = 0;
    virtual void DestroyXmlNode(std::uint32_t node) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void ReleaseEffectReferences(std::uint32_t effect) = 0;
    virtual void ReleaseTextRecord(std::uint32_t address) = 0;
    virtual void DestroyFrame(std::uint32_t frame) = 0;
    virtual void CleanupXmlDocument(DialogObjectState& state) = 0;
    virtual void ClearTextElements(std::uint32_t text_manager) = 0;
};

class IDialogObjectConfigRuntime {
public:
    virtual ~IDialogObjectConfigRuntime() = default;
    [[nodiscard]] virtual std::string_view NodeName(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t FindChild(
        std::uint32_t parent, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t NextChild(
        std::uint32_t parent, std::uint32_t current,
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t ChildCount(
        std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t ChildAt(
        std::uint32_t parent, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::optional<std::string_view> StringAttribute(
        std::uint32_t node, std::string_view name) = 0;
    [[nodiscard]] virtual std::optional<int> IntAttribute(
        std::uint32_t node, std::string_view name) = 0;
    [[nodiscard]] virtual std::optional<float> FloatAttribute(
        std::uint32_t node, std::string_view name) = 0;
    virtual void AppendObjectId(DialogObjectState& state,
                                std::string_view id) = 0;
    virtual void AppendTaskId(DialogObjectState& state, int id) = 0;
    [[nodiscard]] virtual std::uint32_t CopyXmlNode(std::uint32_t node) = 0;
    virtual void AppendTaskParameter(DialogObjectState& state,
                                     std::uint32_t node) = 0;
    [[nodiscard]] virtual bool LoadText(std::uint32_t text_manager,
                                        std::uint32_t oxn_list) = 0;
    virtual void InsertTarget(DialogObjectState& state, std::string_view name,
                              const std::array<float, 3u>& position) = 0;
};

class IDialogObjectActivationRuntime {
public:
    virtual ~IDialogObjectActivationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address, std::uint32_t offset) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t offset,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string_view ReadText(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveTextProperty(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t HandleEffectEvent(
        std::uint32_t property, std::uint32_t owner,
        std::uint32_t parent) = 0;
    [[nodiscard]] virtual bool EffectsEnabled() = 0;
    [[nodiscard]] virtual bool EffectIsActive(std::uint32_t effect) = 0;
    virtual void ProcessEffectEvent(std::uint32_t parent,
                                    std::uint32_t effect) = 0;
    [[nodiscard]] virtual std::uint32_t EffectFrame(
        std::uint32_t effect) = 0;
    virtual void AddFrameChild(std::uint32_t parent,
                               std::uint32_t child) = 0;
};

struct DialogTaskTiming final {
    int wait_mode = 0;
    float wait_time = 0.0f;
    float expected_time = 0.0f;
};
static_assert(sizeof(DialogTaskTiming) == 12u);

struct DialogTaskFactoryAdapterState final {
    std::array<std::uint32_t, 2u> words{};
};
static_assert(sizeof(DialogTaskFactoryAdapterState) == 8u);

class DialogTaskFactoryAdapter final {
public:
    static constexpr std::uint32_t kVtable = 0x84b198u;
    static DialogTaskFactoryAdapterState& Construct(
        DialogTaskFactoryAdapterState& state,
        const DialogTaskFactoryAdapterState& source);
};

enum class DialogTaskInitError {
    MissingManager,
    EmptyClassName,
    CreateTaskFailed,
    EffectPlaybackFailed,
};

class DialogTaskInitException final : public std::runtime_error {
public:
    DialogTaskInitException(DialogTaskInitError code, std::string_view name = {});
    [[nodiscard]] DialogTaskInitError code() const noexcept { return code_; }
private:
    DialogTaskInitError code_;
};

class IDialogObjectTaskRuntime {
public:
    virtual ~IDialogObjectTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address, std::uint32_t offset) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t offset,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual bool EffectsEnabled() = 0;
    [[nodiscard]] virtual bool EffectIsActive(std::uint32_t effect) = 0;
    [[nodiscard]] virtual std::optional<int> IntAttribute(
        std::uint32_t xml, std::string_view name, bool ordered_tree) = 0;
    [[nodiscard]] virtual std::optional<float> FloatAttribute(
        std::uint32_t xml, std::string_view name) = 0;
    [[nodiscard]] virtual std::optional<std::string_view> StringAttribute(
        std::uint32_t xml, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t CreateTask(std::uint32_t xml) = 0;
    virtual void AddTaskAndInitialize(
        std::uint32_t manager, std::uint32_t task, std::string_view name,
        const DialogTaskTiming& timing) = 0;
    virtual void BindDialogObject(std::uint32_t task,
                                  std::uint32_t object) = 0;
    virtual void PrepareTask(std::uint32_t task) = 0;
    virtual void StartTask(std::uint32_t task) = 0;
};

class IDialogObjectPlaybackRuntime {
public:
    virtual ~IDialogObjectPlaybackRuntime() = default;
    virtual void ResetBeforePlayback(DialogObjectState& state) = 0;
    virtual void DestroyFrame(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrame() = 0;
    virtual void RefreshFrame(DialogObjectState& state) = 0;
    virtual void DestroyTaskContainer(std::uint32_t container) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeTaskContainer(
        std::uint32_t memory) = 0;
    [[nodiscard]] virtual bool ActivateEffects(DialogObjectState& state) = 0;
    virtual void StopEffects(DialogObjectState& state) = 0;
    [[nodiscard]] virtual bool InitializeTasks(DialogObjectState& state) = 0;
    virtual void ResetTaskSlots(DialogObjectState& state) = 0;
};

class IDialogObjectResumeRuntime : public IDialogObjectPlaybackRuntime {
public:
    ~IDialogObjectResumeRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t ReadEffectHandle(
        std::uint32_t record) = 0;
    virtual void SetEffectMode(std::uint32_t effect, std::uint32_t mode) = 0;
    [[nodiscard]] virtual bool EffectsEnabled() = 0;
    [[nodiscard]] virtual bool EffectIsActive(std::uint32_t effect) = 0;
    virtual void WriteEffectWord(std::uint32_t effect, std::uint32_t offset,
                                 std::uint32_t value) = 0;
    [[nodiscard]] virtual bool RestartEffect(std::uint32_t effect) = 0;
    virtual void ProcessEffectEvent(std::uint32_t owner,
                                    std::uint32_t effect) = 0;
    [[nodiscard]] virtual std::uint32_t EffectFrame(
        std::uint32_t effect) = 0;
    virtual void AddFrameChild(std::uint32_t parent,
                               std::uint32_t child) = 0;
};

class IDialogObjectStartRuntime {
public:
    virtual ~IDialogObjectStartRuntime() = default;
    [[nodiscard]] virtual int Random15() = 0;
    [[nodiscard]] virtual bool ResumePlayback(DialogObjectState& state) = 0;
    [[nodiscard]] virtual bool PreparePlayback(DialogObjectState& state) = 0;
};

class IDialogObjectPauseRuntime {
public:
    virtual ~IDialogObjectPauseRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadEffectHandle(
        std::uint32_t record) = 0;
    virtual void PauseEffect(std::uint32_t effect) = 0;
    virtual void RefreshFrame(DialogObjectState& state) = 0;
    virtual void BeginWaiting(std::uint32_t container) = 0;
};

class IDialogObjectResetRuntime {
public:
    virtual ~IDialogObjectResetRuntime() = default;
    virtual void CleanupObjectCollection(std::uint32_t container) = 0;
    virtual void DestroyTaskContainer(std::uint32_t container) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address, std::uint32_t offset) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t offset,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual bool EffectsEnabled() = 0;
    [[nodiscard]] virtual bool EffectIsActive(std::uint32_t effect) = 0;
    virtual void SetEffectMode(std::uint32_t effect, std::uint32_t mode) = 0;
    virtual void ReleaseEffectReferences(std::uint32_t effect) = 0;
    virtual void ProcessEffectEvent(std::uint32_t owner,
                                    std::uint32_t effect) = 0;
};

class IDialogObjectStopRuntime {
public:
    virtual ~IDialogObjectStopRuntime() = default;
    virtual void CleanupObjectCollection(std::uint32_t container) = 0;
    virtual void DestroyTaskContainer(std::uint32_t container) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FrameParent(
        std::uint32_t frame) = 0;
    virtual void RemoveFrameChild(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address, std::uint32_t offset) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t offset,
                           std::uint32_t value) = 0;
    virtual void InitializeEffectSystem() = 0;
    virtual void SetEffectMode(std::uint32_t effect,
                               std::uint32_t mode) = 0;
    [[nodiscard]] virtual double EffectValue(std::uint32_t effect) = 0;
};

class IDialogObjectUpdateRuntime {
public:
    virtual ~IDialogObjectUpdateRuntime() = default;
    [[nodiscard]] virtual int UpdateTaskContainer(
        std::uint32_t container, float elapsed) = 0;
    [[nodiscard]] virtual bool TaskContainerReady(
        std::uint32_t container) = 0;
    [[nodiscard]] virtual int ResetRuntimeState(DialogObjectState& state) = 0;
    [[nodiscard]] virtual std::array<float, 3u> ObjectPosition(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual int SetAbsolutePosition(
        DialogObjectState& state, const std::array<float, 3u>& position) = 0;
};

class IDialogObjectFrameRuntime {
public:
    virtual ~IDialogObjectFrameRuntime() = default;
    virtual void SetIdentity(std::uint32_t frame) = 0;
    virtual void Scale(std::uint32_t frame,
                       const std::array<float, 3u>& value,
                       std::uint32_t combine) = 0;
    virtual void Rotate(std::uint32_t frame,
                        const std::array<float, 3u>& axis,
                        float angle, std::uint32_t combine) = 0;
    virtual void Translate(std::uint32_t frame,
                           const std::array<float, 3u>& value,
                           std::uint32_t combine) = 0;
    [[nodiscard]] virtual int UpdateObjects(std::uint32_t frame) = 0;
};

class DialogObject final {
public:
    static constexpr std::uint32_t kVtable = 0x84b194u;
    [[nodiscard]] static std::uint32_t CreateInstance(
        DialogObjectState& state, IDialogObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyInstance(
        std::uint32_t object, std::uint32_t first, std::uint32_t second,
        IDialogObjectRuntime& runtime);
    static DialogObjectState& Construct(DialogObjectState& state,
                                        IDialogObjectRuntime& runtime);
    static DialogObjectState& Destroy(DialogObjectState& state,
                                      IDialogObjectRuntime& runtime);
    static void ClearFirstTree(DialogObjectState& state,
                               IDialogObjectRuntime& runtime);
    static void ClearSecondTree(DialogObjectState& state,
                                IDialogObjectRuntime& runtime);
    [[nodiscard]] static int SetPosition(DialogObjectState& state,
        const std::array<float, 3u>& value, bool additive,
        IDialogObjectTransformRuntime& runtime);
    [[nodiscard]] static int SetRotation(DialogObjectState& state,
        const std::array<float, 3u>& value, bool additive,
        IDialogObjectTransformRuntime& runtime);
    [[nodiscard]] static int SetScale(DialogObjectState& state,
        const std::array<float, 3u>& value, bool additive,
        IDialogObjectTransformRuntime& runtime);
    [[nodiscard]] static int SetParentFrame(DialogObjectState& state,
        std::uint32_t parent, IDialogObjectTransformRuntime& runtime);
    [[nodiscard]] static int SetVisibility(DialogObjectState& state,
                                            std::uint32_t value);
    [[nodiscard]] static int RemoveFromParentFrame(
        DialogObjectState& state, IDialogObjectTransformRuntime& runtime);
    [[nodiscard]] static int ResetVisibility(DialogObjectState& state);
    [[nodiscard]] static bool GetStoredVector(
        const DialogObjectState& state, std::string_view key,
        std::array<float, 3u>& output,
        IDialogObjectPropertyRuntime& runtime);
    [[nodiscard]] static bool GetVector(
        const DialogObjectState& state, std::string_view key,
        std::array<float, 3u>& output,
        IDialogObjectPropertyRuntime& runtime);
    static void SetFacet(DialogObjectState& state, std::string_view key,
                         std::uint32_t facet,
                         IDialogObjectPropertyRuntime& runtime);
    static void ClearFacets(DialogObjectState& state,
                            IDialogObjectFacetCleanupRuntime& runtime);
    static void PrepareForDelete(DialogObjectState& state,
                                 IDialogObjectPreDeleteRuntime& runtime,
                                 IDialogObjectFacetCleanupRuntime& facet_runtime);
    [[nodiscard]] static bool LoadConfig(DialogObjectState& state,
                                         std::uint32_t root,
                                         IDialogObjectConfigRuntime& runtime);
    [[nodiscard]] static bool ActivateEffects(
        DialogObjectState& state, std::uint32_t object,
        IDialogObjectActivationRuntime& runtime);
    [[nodiscard]] static bool InitializeTasks(
        DialogObjectState& state, std::uint32_t object,
        IDialogObjectTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetTaskSlots(
        DialogObjectState& state, IDialogObjectTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t StopEffects(
        DialogObjectState& state,
        IDialogObjectEffectCleanupRuntime& runtime);
    [[nodiscard]] static bool PreparePlayback(
        DialogObjectState& state, IDialogObjectPlaybackRuntime& runtime);
    [[nodiscard]] static bool ResumePlayback(
        DialogObjectState& state, std::uint32_t object,
        IDialogObjectResumeRuntime& runtime);
    [[nodiscard]] static bool StartPlayback(
        DialogObjectState& state, IDialogObjectStartRuntime& runtime);
    [[nodiscard]] static bool PausePlayback(
        DialogObjectState& state, IDialogObjectPauseRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetRuntimeState(
        DialogObjectState& state, IDialogObjectResetRuntime& runtime);
    [[nodiscard]] static double StopPlayback(
        DialogObjectState& state, bool detach_frame,
        IDialogObjectStopRuntime& runtime);
    [[nodiscard]] static int UpdatePlayback(
        DialogObjectState& state, float elapsed,
        IDialogObjectUpdateRuntime& runtime);
    [[nodiscard]] static int RefreshTransform(
        const DialogObjectState& state, IDialogObjectFrameRuntime& runtime);
};

}  // namespace pal4
