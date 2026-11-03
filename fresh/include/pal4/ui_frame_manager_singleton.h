#pragma once

#include "pal4/renderware_image.h"

#include <array>
#include <cstdint>
#include "pal4/string_buffer.h"

namespace pal4 {

class IUiFrameManagerSingletonRuntime {
public:
    virtual ~IUiFrameManagerSingletonRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, const char* name,
        std::uint32_t singleton_id,
        std::uint32_t factory) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(
        std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IUiFrameManagerDestructRuntime {
public:
    virtual ~IUiFrameManagerDestructRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t NilNode() = 0;
    virtual void DestroyCursor(std::uint32_t cursor) = 0;
    [[nodiscard]] virtual std::uint32_t FindMaxNode(
        std::uint32_t node) = 0;
    virtual void FreeNodes(std::uint32_t manager,
                           std::uint32_t root) = 0;
    virtual void FirstElement(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t NextNode(
        std::uint32_t node) = 0;
    virtual void DeleteNode(std::uint32_t manager,
                            std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint8_t SetErrorFlag(
        std::uint32_t record, std::uint32_t flag) = 0;
};

class IUiFrameManagerInitializeRuntime {
public:
    virtual ~IUiFrameManagerInitializeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializeLogger() = 0;
    virtual void LogCursorLoading(std::uint32_t logger,
                                  const char* file,
                                  std::uint32_t line,
                                  const char* message) = 0;
    [[nodiscard]] virtual std::uint32_t PkManager() = 0;
    virtual void ProcessFile3(std::uint32_t manager) = 0;
    virtual void LoadCursor(std::uint32_t ui_manager,
                            const char* name) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessFile4(
        std::uint32_t manager) = 0;
};

struct UiCursorRecord {
    StringBufferState name{};
    std::uint32_t image{};
};

class IUiFrameManagerLoadCursorRuntime {
public:
    virtual ~IUiFrameManagerLoadCursorRuntime() = default;
    [[nodiscard]] virtual std::uint8_t SetErrorFlag(
        StringBufferState& text, std::uint8_t flag) = 0;
    virtual void AssignCString(StringBufferState& text,
                               const char* value) = 0;
    virtual void AppendCString(StringBufferState& text,
                               const char* value) = 0;
    virtual void AssignString(StringBufferState& destination,
                              const StringBufferState& source) = 0;
    [[nodiscard]] virtual std::uint32_t TreeEnd(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t Find(
        std::uint32_t manager,
        const StringBufferState& name) = 0;
    virtual void LogMissingCursor(const char* file,
                                  std::uint32_t line,
                                  const char* format,
                                  const char* name) = 0;
    [[nodiscard]] virtual std::uint32_t LoadImage(
        const StringBufferState& path,
        std::uint32_t type,
        std::uint32_t width,
        std::uint32_t height,
        std::uint32_t flags) = 0;
    virtual void InsertOrFind(std::uint32_t manager,
                              const UiCursorRecord& cursor) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(
        std::uint32_t address) = 0;
};

class IUiFrameManagerSetCursorRuntime {
public:
    virtual ~IUiFrameManagerSetCursorRuntime() = default;
    [[nodiscard]] virtual std::uint8_t SetErrorFlag(
        StringBufferState& text, std::uint8_t flag) = 0;
    virtual void AssignCString(StringBufferState& text,
                               const char* value) = 0;
    [[nodiscard]] virtual std::uint32_t TreeEnd(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t FindNode(
        std::uint32_t manager,
        const StringBufferState& name) = 0;
    [[nodiscard]] virtual std::int32_t CompareNodeName(
        const StringBufferState& name,
        std::uint32_t node_name_address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void SetCursor(std::uint32_t cursor) = 0;
    [[nodiscard]] virtual StringBufferState& CurrentName(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void Reallocate(StringBufferState& text,
                            std::uint32_t length) = 0;
    virtual void CopyCString(StringBufferState& text,
                             const char* value,
                             std::uint32_t length) = 0;
};

class IUiFrameManagerConstructRuntime {
public:
    virtual ~IUiFrameManagerConstructRuntime() = default;
    virtual void ConstructEventTree(std::uint32_t address) = 0;
    virtual void ConstructTree(std::uint32_t address,
                               std::uint8_t seed) = 0;
    virtual void SetErrorFlag(std::uint32_t address,
                              std::uint8_t flag) = 0;
    virtual void InitializeEffects(std::uint32_t manager) = 0;
    virtual void InitializeXml(std::uint32_t address) = 0;
    virtual void InitializeStruct(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructCollection(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateAndInitializeNode(
        std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeSounds(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void CopyDwordIfDestinationNonNull(
        std::uint32_t destination,
        std::uint32_t source) = 0;
    virtual void FinishRange(std::uint32_t begin,
                             std::uint32_t end) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CollectionSize(
        std::uint32_t address) = 0;
};

class IUiFrameManagerTreeDestructRuntime {
public:
    virtual ~IUiFrameManagerTreeDestructRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(
        std::uint32_t address) = 0;
    virtual void FreeNodes(std::uint32_t tree,
                           std::uint32_t root) = 0;
    [[nodiscard]] virtual std::uint32_t NextNode(
        std::uint32_t& iterator) = 0;
    virtual void DeleteNode(std::uint32_t tree,
                            std::uint32_t node) = 0;
    virtual void FirstElement(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    virtual void SetSharedNil(std::uint32_t nil) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNilUsers() = 0;
    virtual void SetSharedNilUsers(std::uint32_t count) = 0;
};

class IUiFrameManagerTreeConstructRuntime {
public:
    virtual ~IUiFrameManagerTreeConstructRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void ConstructRedBlackTree(std::uint32_t tree) = 0;
};

class IUiFrameManagerFullDestructRuntime
    : public IUiFrameManagerTreeDestructRuntime {
public:
    virtual void DestructSound(std::uint32_t address) = 0;
    virtual void DestructLateStruct(std::uint32_t address) = 0;
    virtual void DestructXml(std::uint32_t address) = 0;
    virtual void DestructEffects(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t GetFirstElement(
        std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t DestructEventTree(
        std::uint32_t tree) = 0;
};

class IUiFrameManagerRegisterWindowsRuntime {
public:
    virtual ~IUiFrameManagerRegisterWindowsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t RegisterFrame(
        std::uint32_t manager, const char* name) = 0;
};

class IUiFrameManagerMessageRuntime {
public:
    virtual ~IUiFrameManagerMessageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ProcessUiEvent(
        std::uint32_t system, std::uint32_t hwnd,
        std::uint32_t message, std::uint32_t wparam,
        std::uint32_t lparam) = 0;
    virtual void SimulateKeyPressAndRelease(
        std::uint32_t manager, std::uint32_t key) = 0;
};

class IUiFrameManagerUpdateRuntime {
public:
    virtual ~IUiFrameManagerUpdateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool IsVisible(
        std::uint32_t frame) = 0;
    virtual void InjectTimePulse(std::uint32_t system,
                                 float elapsed) = 0;
    virtual void RotateGameObjects(std::uint32_t collection,
                                   double elapsed) = 0;
    virtual void UpdateJumpHint(std::uint32_t frame,
                                float elapsed) = 0;
    virtual void UpdateFrame(std::uint32_t frame,
                             double elapsed) = 0;
    virtual void UpdateTimedEffects(std::uint32_t effects,
                                    float elapsed) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateAnimations(
        std::uint32_t list, float elapsed) = 0;
};

class IUiFrameManagerRenderRuntime {
public:
    virtual ~IUiFrameManagerRenderRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void RenderGui(std::uint32_t system) = 0;
    [[nodiscard]] virtual std::uint32_t RenderAnimations(
        std::uint32_t list, std::uint32_t context) = 0;
    virtual void GetGameObjectCount(
        std::uint32_t collection, std::uint32_t context) = 0;
    virtual void DrawEffects(std::uint32_t manager,
                             std::uint32_t context) = 0;
    virtual void GetGameObject(std::uint32_t collection,
                               std::uint32_t context) = 0;
};

class IUiFrameManagerPortraitRuntime {
public:
    virtual ~IUiFrameManagerPortraitRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, const char* name) = 0;
    virtual void SetFlagAndUpdateVisibility(
        std::uint32_t frame, std::uint8_t flag) = 0;
};

class IUiFrameManagerCleanupRuntime {
public:
    virtual ~IUiFrameManagerCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void DisableEffects(std::uint32_t manager) = 0;
    virtual void DestroyFrame(std::uint32_t frame, std::uint32_t delete_flag) = 0;
    virtual void CleanupCollectionElements(std::uint32_t collection) = 0;
    virtual void CleanupGameObjects(std::uint32_t collection) = 0;
    virtual void DestructCollection(std::uint32_t collection) = 0;
    virtual void CleanupTimedEffects(std::uint32_t effects) = 0;
    virtual void DestructEmbeddedTree(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t ClearSpinnerPool() = 0;
    virtual void DestructGui(std::uint32_t gui) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class IUiFrameManagerFrameEventsRuntime {
public:
    virtual ~IUiFrameManagerFrameEventsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void CleanupEventNode(std::uint32_t list,
                                  std::uint32_t node) = 0;
    virtual void FreeNodePayload(std::uint32_t payload,
                                 std::uint32_t flag) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IUiFrameManagerAccessRuntime {
public:
    virtual ~IUiFrameManagerAccessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructMenu(std::uint32_t memory) = 0;
    virtual void FreeAfterConstructionFailure(std::uint32_t memory) = 0;
};

enum class UiCachedFrameKind {
    Common, YesNo, Select, Scenario, ProgressBar,
    Loading, Hint, CombatHint, CombatHint2
};

class IUiFrameManagerCachedFrameRuntime {
public:
    virtual ~IUiFrameManagerCachedFrameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(
        UiCachedFrameKind kind, std::uint32_t memory,
        const char* name) = 0;
    virtual void FreeAfterConstructionFailure(std::uint32_t memory) = 0;
};

class IUiFrameManagerDialogRuntime : public IUiFrameManagerCachedFrameRuntime {
public:
    ~IUiFrameManagerDialogRuntime() override = default;
    virtual void ReleaseCommonResources(std::uint32_t dialog) = 0;
    virtual void CleanupSelectCallback(std::uint32_t dialog) = 0;
};

class IUiFrameManagerPoetryRuntime {
public:
    virtual ~IUiFrameManagerPoetryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void DestroyPoetry(std::uint32_t poetry,
                               std::uint32_t delete_flag) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeName() = 0;
    [[nodiscard]] virtual std::uint32_t FormatName(
        std::uint32_t name_buffer, const char* format,
        std::int32_t index) = 0;
    virtual void CleanupName(std::uint32_t name_buffer) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructPoetry(
        std::uint32_t memory, std::uint32_t name) = 0;
    virtual void FreeAfterConstructionFailure(std::uint32_t memory) = 0;
    [[nodiscard]] virtual std::uint32_t HidePoetry(
        std::uint32_t poetry) = 0;
};

struct UiFrameTimerText {
    std::uint32_t control{};
    std::uint32_t data{};
    std::uint32_t length{};
    std::uint32_t capacity{};
};

class IUiFrameManagerTimerRuntime {
public:
    virtual ~IUiFrameManagerTimerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, const char* name) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyText() = 0;
    [[nodiscard]] virtual std::uint8_t ReadReference(
        std::uint32_t data) = 0;
    virtual void WriteReference(std::uint32_t data,
                                std::uint8_t value) = 0;
    virtual void SetErrorFlag(UiFrameTimerText& text,
                              std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateText(
        UiFrameTimerText& text, std::uint32_t length,
        std::uint8_t flag) = 0;
    virtual void CopyText(std::uint32_t destination,
                          std::uint32_t source,
                          std::uint32_t length) = 0;
    virtual void SetTextSize(UiFrameTimerText& text,
                             std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeTimer(
        std::uint32_t frame, std::uint32_t period,
        const UiFrameTimerText& text) = 0;
    [[nodiscard]] virtual std::uint32_t Free(
        std::uint32_t address) = 0;
};

class IUiFrameManagerTimerControlRuntime {
public:
    virtual ~IUiFrameManagerTimerControlRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, const char* name) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ClearTimerAndText(std::uint32_t frame) = 0;
};

struct UiFrameEventPayload {
    std::uint32_t type{};
    std::uint32_t first{};
    std::uint32_t second{};
};

class IUiFrameManagerEventRuntime {
public:
    virtual ~IUiFrameManagerEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ProcessEvent(
        std::uint32_t event_tree, std::uint32_t key,
        const UiFrameEventPayload& payload) = 0;
    [[nodiscard]] virtual std::uint8_t ProcessUiEvent(
        std::uint32_t system, std::uint32_t hwnd,
        std::uint32_t message, std::uint32_t wparam,
        std::uint32_t lparam) = 0;
};

class IUiFrameManagerCoordinatesRuntime {
public:
    virtual ~IUiFrameManagerCoordinatesRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void TransformMouse(std::uint32_t system,
                                std::uint32_t output,
                                std::uint32_t input) = 0;
    virtual void TransformCegui(std::uint32_t system,
                                std::uint32_t output,
                                std::uint32_t input) = 0;
};

class IUiFrameManagerResourceRuntime {
public:
    virtual ~IUiFrameManagerResourceRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t AddElement(
        std::uint32_t collection, const char* prefix,
        const char* name) = 0;
};

class IUiFrameManagerCameraRuntime {
public:
    virtual ~IUiFrameManagerCameraRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializeCameraSubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCameraObject(
        std::uint32_t subsystem) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class IUiFrameManagerStateEventRuntime {
public:
    virtual ~IUiFrameManagerStateEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t QueryGameState(
        std::uint32_t controller) = 0;
    [[nodiscard]] virtual std::uint8_t ProcessEvent(
        std::uint32_t tree, std::uint32_t key,
        std::uint32_t type, std::uint32_t zero) = 0;
};

class IUiFrameManagerVisibilityRuntime {
public:
    virtual ~IUiFrameManagerVisibilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, const char* name) = 0;
    [[nodiscard]] virtual bool IsVisible(std::uint32_t frame) = 0;
};

class IUiFrameManagerEdgeWindowsRuntime {
public:
    virtual ~IUiFrameManagerEdgeWindowsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadConfigWord(
        std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::array<float, 2> TransformMouseCoordinates(
        std::uint32_t system, std::array<float, 2> input) = 0;
    [[nodiscard]] virtual std::uint32_t SystemSingleton() = 0;
    virtual void AddChildWindow(std::uint32_t parent,
                                std::uint32_t child) = 0;
    virtual void RemoveChildWindow(std::uint32_t parent,
                                   std::uint32_t child) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window, bool value) = 0;
    virtual void SetClippedByParent(std::uint32_t window, bool value) = 0;
    virtual void SetPosition(std::uint32_t window, std::uint32_t mode,
                             std::array<float, 2> position) = 0;
    virtual void SetWidth(std::uint32_t window, std::uint32_t mode,
                          std::uint32_t float_bits) = 0;
    virtual void SetHeight(std::uint32_t window, std::uint32_t mode,
                           std::uint32_t float_bits) = 0;
};

class IUiFrameManagerTutorialRuntime {
public:
    virtual ~IUiFrameManagerTutorialRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializeGlobalData() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool GetDialogFlag(
        std::uint32_t data, std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, const char* name) = 0;
    [[nodiscard]] virtual std::uint32_t ManagerSingleton() = 0;
    virtual void RegisterFrame(std::uint32_t manager,
                               const char* name) = 0;
    virtual void SetDialogFlagAndSave(
        std::uint32_t data, std::uint32_t key) = 0;
    virtual void UpdateVisibility(
        std::uint32_t frame, std::uint32_t visible,
        std::uint32_t key, std::uint32_t zero) = 0;
};

class IUiFrameManagerTimerFlagRuntime {
public:
    virtual ~IUiFrameManagerTimerFlagRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, const char* name) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
};

class IUiFrameManagerWorldClumpRuntime {
public:
    virtual ~IUiFrameManagerWorldClumpRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t AddClump(
        std::uint32_t collection, std::uint32_t clump) = 0;
};

class UiFrameManagerSingleton final {
public:
    static constexpr std::uint32_t kFactoryAddress = 0x4bb690u;

    // IDA 0x4bb650.
    [[nodiscard]] static std::uint32_t GetInstance(
        std::uint8_t& guard, std::uint32_t& cached,
        IUiFrameManagerSingletonRuntime& runtime);

    // IDA 0x4bb690.
    [[nodiscard]] static std::uint32_t CreateInstance(
        IUiFrameManagerSingletonRuntime& runtime);

    // IDA 0x4bb6f0.
    [[nodiscard]] static std::uint8_t Destruct(
        std::uint32_t manager,
        IUiFrameManagerDestructRuntime& runtime);

    // IDA 0x4bb8a0.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t manager,
        IUiFrameManagerInitializeRuntime& runtime);

    // IDA 0x4bb910. The byte returned by the original is a cleanup residue.
    [[nodiscard]] static std::uint8_t LoadCursor(
        std::uint32_t manager, const char* name,
        IUiFrameManagerLoadCursorRuntime& runtime);

    // IDA 0x4bbb70.
    [[nodiscard]] static std::uint8_t SetCursor(
        std::uint32_t manager, const char* name,
        IUiFrameManagerSetCursorRuntime& runtime);

    // IDA 0x4bbd00. The caller provides the observed uninitialized stack byte.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t manager, std::uint8_t seed,
        IUiFrameManagerConstructRuntime& runtime);

    // IDA 0x4bbfd0; tree is the embedded object at manager+0x114.
    [[nodiscard]] static std::uint32_t DestructEmbeddedTree(
        std::uint32_t tree,
        IUiFrameManagerTreeDestructRuntime& runtime);

    // IDA 0x4bc0b0; event tree is at manager+0x104.
    [[nodiscard]] static std::uint32_t DestructEventTree(
        std::uint32_t tree,
        IUiFrameManagerTreeDestructRuntime& runtime);

    // IDA 0x4c0380; auxiliary tree at the manager's +0x1a4 object.
    [[nodiscard]] static std::uint32_t DestructAuxiliaryTree(
        std::uint32_t tree,
        IUiFrameManagerTreeDestructRuntime& runtime);

    // IDA 0x4c0430; called on manager+0x114 during manager construction.
    [[nodiscard]] static std::uint32_t ConstructEmbeddedTree(
        std::uint32_t tree, std::uint32_t first_source,
        std::uint8_t flag, std::uint32_t third_source,
        IUiFrameManagerTreeConstructRuntime& runtime);

    // IDA 0x4bc170. Returns the final event-tree destructor result.
    [[nodiscard]] static std::uint32_t DestructManager(
        std::uint32_t manager,
        IUiFrameManagerFullDestructRuntime& runtime);

    // IDA 0x4bd320.
    [[nodiscard]] static std::uint32_t RegisterWindows(
        std::uint32_t manager,
        IUiFrameManagerRegisterWindowsRuntime& runtime);

    // IDA 0x4bf0e0.
    [[nodiscard]] static std::uint8_t HandleMessage(
        std::uint32_t manager, std::uint32_t hwnd,
        std::uint32_t message, std::uint32_t wparam,
        std::uint32_t lparam,
        IUiFrameManagerMessageRuntime& runtime);

    // IDA 0x4bf150.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t manager, double elapsed,
        IUiFrameManagerUpdateRuntime& runtime);

    // IDA 0x4bf240.
    [[nodiscard]] static std::uint32_t Render(
        std::uint32_t manager, std::uint32_t context_record,
        IUiFrameManagerRenderRuntime& runtime);

    // IDA 0x4bf270; Hex-Rays fails here, so the sequence follows assembly.
    static void PreRender(
        std::uint32_t manager, std::uint32_t context_record,
        IUiFrameManagerRenderRuntime& runtime);

    // IDA 0x4bf2b0.
    static void SetPortraitFlag(
        std::uint32_t manager, std::uint8_t flag,
        IUiFrameManagerPortraitRuntime& runtime);

    // IDA 0x4bf2e0. Returns the last pool-clear or CEGUI-free result.
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t manager, IUiFrameManagerCleanupRuntime& runtime);

    // IDA 0x4bf4c0; the container is the manager's +0x1a0 object.
    [[nodiscard]] static std::uint32_t CleanupFrameEvents(
        std::uint32_t container, IUiFrameManagerFrameEventsRuntime& runtime);

    // IDA 0x4bf590.
    [[nodiscard]] static std::uint8_t SetMessageFlag(
        std::uint32_t manager, std::uint8_t flag,
        IUiFrameManagerAccessRuntime& runtime);

    // IDA 0x4bf5a0; the cached object is a MenuWindow.
    [[nodiscard]] static std::uint32_t GetMenuWindow(
        std::uint32_t manager, IUiFrameManagerAccessRuntime& runtime);

    // IDA 0x4bf610; releases resources on every call, including a null result.
    [[nodiscard]] static std::uint32_t CreateCommonDialog(
        std::uint32_t manager, IUiFrameManagerDialogRuntime& runtime);

    // IDA 0x4bf690.
    [[nodiscard]] static std::uint32_t CreateYesNoDialog(
        std::uint32_t manager, IUiFrameManagerDialogRuntime& runtime);

    // IDA 0x4bf700; clears the pending callback on every call.
    [[nodiscard]] static std::uint32_t CreateSelectDialog(
        std::uint32_t manager, IUiFrameManagerDialogRuntime& runtime);

    // IDA 0x4bfa60.
    [[nodiscard]] static std::uint32_t CreateScenarioDialog(
        std::uint32_t manager, IUiFrameManagerCachedFrameRuntime& runtime);

    // IDA 0x4bfad0.
    [[nodiscard]] static std::uint32_t CreateProgressBar(
        std::uint32_t manager, IUiFrameManagerCachedFrameRuntime& runtime);

    // IDA 0x4bfb40; constructs a LoadingWindow.
    [[nodiscard]] static std::uint32_t CreateLoadingWindow(
        std::uint32_t manager, IUiFrameManagerCachedFrameRuntime& runtime);

    // IDA 0x4bfbd0.
    [[nodiscard]] static std::uint32_t CreateHintWindow(
        std::uint32_t manager, IUiFrameManagerCachedFrameRuntime& runtime);

    // IDA 0x4bfc40.
    [[nodiscard]] static std::uint32_t CreateCombatHintWindow(
        std::uint32_t manager, IUiFrameManagerCachedFrameRuntime& runtime);

    // IDA 0x4bfcb0.
    [[nodiscard]] static std::uint32_t CreateCombatHintWindow2(
        std::uint32_t manager, IUiFrameManagerCachedFrameRuntime& runtime);

    // IDA 0x4bf780.
    [[nodiscard]] static std::uint32_t CreatePoetryWindow(
        std::uint32_t manager, std::int32_t index,
        IUiFrameManagerPoetryRuntime& runtime);

    // IDA 0x4bf830.
    [[nodiscard]] static std::uint32_t CreateAndHidePoetryWindow(
        std::uint32_t manager, std::int32_t index,
        IUiFrameManagerPoetryRuntime& runtime);

    // IDA 0x4bf850.
    [[nodiscard]] static std::uint32_t GetPoetryWindow(
        std::uint32_t manager, IUiFrameManagerPoetryRuntime& runtime);

    // IDA 0x4bf860. The input text pointer owns one reference.
    [[nodiscard]] static std::uint32_t InitializePortraitTimer(
        std::uint32_t manager, std::uint32_t period,
        std::uint8_t text_control, std::uint32_t text_data,
        std::uint32_t text_length, std::uint32_t text_capacity,
        IUiFrameManagerTimerRuntime& runtime);

    // IDA 0x4bf9c0; the body continues in a tail chunk at 0x47c870.
    [[nodiscard]] static std::int32_t SetPortraitTimerDuration(
        std::uint32_t manager, std::int32_t duration,
        IUiFrameManagerTimerControlRuntime& runtime);

    // IDA 0x4bf9e0.
    static void ClearPortraitTimerText(
        std::uint32_t manager,
        IUiFrameManagerTimerControlRuntime& runtime);

    // IDA 0x4bfa40.
    [[nodiscard]] static std::int32_t GetPortraitTimerDuration(
        std::uint32_t manager,
        IUiFrameManagerTimerControlRuntime& runtime);

    // IDA 0x4bfd20.
    [[nodiscard]] static std::uint8_t ProcessUiEvents(
        std::uint32_t manager, std::uint32_t type,
        std::uint32_t arguments,
        IUiFrameManagerEventRuntime& runtime);

    // IDA 0x4bfd70.
    [[nodiscard]] static std::uint8_t SimulateKeyPressAndRelease(
        std::uint32_t manager, std::uint32_t key,
        IUiFrameManagerEventRuntime& runtime);

    // IDA 0x4bfdb0.
    [[nodiscard]] static std::uint32_t ConvertUiFrameCoordinates(
        std::uint32_t manager, std::uint32_t output,
        std::uint32_t input,
        IUiFrameManagerCoordinatesRuntime& runtime);

    // IDA 0x4bfdd0.
    [[nodiscard]] static std::uint32_t TransformCeguiCoordinates(
        std::uint32_t manager, std::uint32_t output,
        std::uint32_t input,
        IUiFrameManagerCoordinatesRuntime& runtime);

    // IDA 0x4bfdf0.
    [[nodiscard]] static std::uint32_t AddUiResource(
        std::uint32_t manager, const char* name,
        IUiFrameManagerResourceRuntime& runtime);

    // IDA 0x4bfe30. RenderWare operations stay behind an injectable API.
    [[nodiscard]] static std::uint32_t CopyActiveCameraRaster(
        std::uint32_t manager, IUiFrameManagerCameraRuntime& runtime,
        IRenderWareImageApi& renderware);

    // IDA 0x4bfed0; inactive branches expose an AL register residue.
    [[nodiscard]] static std::uint8_t ProcessStateEvent(
        std::uint32_t manager, std::uint32_t type,
        IUiFrameManagerStateEventRuntime& runtime);

    // IDA 0x4bff30.
    [[nodiscard]] static bool IsPortraitTimerVisible(
        std::uint32_t manager,
        IUiFrameManagerVisibilityRuntime& runtime);

    // IDA 0x4c0090.
    static void ToggleEdgeWindows(
        std::uint32_t manager, bool enabled,
        IUiFrameManagerEdgeWindowsRuntime& runtime);

    // IDA 0x4c0260.
    static void ShowTutorialDialog(
        std::uint32_t manager, std::uint32_t key,
        IUiFrameManagerTutorialRuntime& runtime);

    // IDA 0x4c02e0; its final read is in the 0x478fc0 tail chunk.
    [[nodiscard]] static std::uint8_t GetPortraitTimerFlag(
        std::uint32_t manager, IUiFrameManagerTimerFlagRuntime& runtime);

    // IDA 0x4c0300.
    [[nodiscard]] static bool AddWorldClump(
        std::uint32_t manager, std::uint32_t clump,
        IUiFrameManagerWorldClumpRuntime& runtime);
};

}  // namespace pal4
