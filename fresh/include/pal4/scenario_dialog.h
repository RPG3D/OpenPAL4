#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace pal4 {

class IScenarioDialogRuntime {
public:
    virtual ~IScenarioDialogRuntime() = default;
    virtual void LoadLayout(std::uint32_t object, const char* layout) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint8_t IndeterminateTreeByte() = 0;
    virtual void InitializeTree(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::string ReadLayoutPrefix(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::uint32_t object, const std::string& name) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window, bool enabled) = 0;
    virtual void RegisterEditboxEvent(std::uint32_t object) = 0;
    virtual void SetTextAndImagePositions(std::uint32_t object,
                                          std::uint32_t option) = 0;
    virtual void DestructTree(std::uint32_t tree) = 0;
    virtual void DestructBase(std::uint32_t object) = 0;
};

class IScenarioDialogLifecycleRuntime {
public:
    virtual ~IScenarioDialogLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void FreeNodes(std::uint32_t tree,
                           std::uint32_t root) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    virtual void FirstElement(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t Successor(
        std::uint32_t node) = 0;
    virtual void RemoveNode(std::uint32_t tree,
                            std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t Free(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DecrementSharedRef() = 0;
    [[nodiscard]] virtual std::uint32_t TakeSharedNil() = 0;
    [[nodiscard]] virtual std::uint32_t DestructBase(
        std::uint32_t object) = 0;
};

struct ScenarioPortraitCacheEntry {
    std::string imageset;
    std::string image;
};

class IScenarioPortraitRuntime {
public:
    virtual ~IScenarioPortraitRuntime() = default;
    virtual void BeginResources() = 0;
    [[nodiscard]] virtual std::uint32_t EndResources() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void SetImage(std::uint32_t window,
                          std::uint32_t image) = 0;
    [[nodiscard]] virtual std::optional<ScenarioPortraitCacheEntry>
        FindCache(std::uint32_t tree, const std::string& name) = 0;
    virtual void InsertCache(
        std::uint32_t tree, const std::string& name,
        const ScenarioPortraitCacheEntry& entry) = 0;
    [[nodiscard]] virtual bool ImagesetPresent(
        const std::string& name) = 0;
    virtual void CreateImageset(const std::string& file) = 0;
    [[nodiscard]] virtual bool PortraitFileExists(
        const std::string& file) = 0;
    [[nodiscard]] virtual bool ImageDefined(
        const std::string& imageset,
        const std::string& name) = 0;
    [[nodiscard]] virtual std::uint32_t GetImage(
        const std::string& imageset,
        const std::string& name) = 0;
    virtual void SetTextAndImagePositions(
        std::uint32_t object, std::uint32_t option) = 0;
};

class IScenarioTextRuntime {
public:
    virtual ~IScenarioTextRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string ReadCString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        const std::string& text) = 0;
    virtual void SetUtf8Text(std::uint32_t window,
                             const std::string& text) = 0;
    virtual void Show(std::uint32_t object,
                      std::uint32_t visible,
                      std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t InsertHandler(
        std::uint32_t array, std::uint32_t handler) = 0;
};

class IScenarioInputEventRuntime {
public:
    virtual ~IScenarioInputEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void HandleTextDisplay(std::uint32_t object) = 0;
};

class IScenarioEventRuntime : public IScenarioInputEventRuntime {
public:
    virtual ~IScenarioEventRuntime() = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t SubscribeMouseUp(
        std::uint32_t window, std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyOwner(
        std::uint32_t owner, std::uint32_t flags) = 0;
};

struct ScenarioRect {
    float left;
    float top;
    float right;
    float bottom;
};

class IScenarioLayoutRuntime {
public:
    virtual ~IScenarioLayoutRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float FontExtentAA(
        std::uint32_t editbox) = 0;
    virtual void SetVisible(std::uint32_t window,
                            bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t SetRect(
        std::uint32_t window, const ScenarioRect& rectangle) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void RequestRedraw(std::uint32_t window) = 0;
};

class IScenarioVisibilityRuntime {
public:
    virtual ~IScenarioVisibilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ScriptInstance() = 0;
    [[nodiscard]] virtual bool AudioActive(
        std::uint32_t audio) = 0;
    virtual void PlayAudio(std::uint32_t audio) = 0;
    [[nodiscard]] virtual std::string ReadGlobalString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void SetText(std::uint32_t window,
                         const std::string& text) = 0;
    virtual void SetImage(std::uint32_t window,
                          std::uint32_t image) = 0;
    virtual void Layout(std::uint32_t object,
                        std::uint32_t option) = 0;
    virtual void Close(std::uint32_t object,
                       std::uint32_t visible,
                       std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t RemoveHandler(
        std::uint32_t array, std::uint32_t handler) = 0;
};

class IScenarioTextDisplayRuntime {
public:
    virtual ~IScenarioTextDisplayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool IsTypeEnd(
        std::uint32_t editbox) = 0;
    [[nodiscard]] virtual bool NeedChangePage(
        std::uint32_t editbox) = 0;
    virtual void ChangePage(std::uint32_t editbox) = 0;
    virtual void ChangeLine(std::uint32_t editbox) = 0;
    virtual void RequestRedraw(std::uint32_t editbox) = 0;
    virtual void HideAndClear(std::uint32_t object) = 0;
};

class ScenarioDialog final {
public:
    static constexpr std::uint32_t kBaseTreeVtable = 0x8435a4u;
    static constexpr std::uint32_t kVtable = 0x84431cu;
    static constexpr std::uint32_t kTreeVtable = 0x844304u;

    // IDA 0x4b6e60.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IScenarioDialogRuntime& runtime);
    // IDA 0x4b7110.
    [[nodiscard]] static std::uint32_t DeletingDestruct(
        std::uint32_t object, std::uint8_t flags,
        IScenarioDialogLifecycleRuntime& runtime);
    // IDA 0x4b7130: tree subobject at object+64.
    [[nodiscard]] static std::uint32_t DestructTree(
        std::uint32_t tree, IScenarioDialogLifecycleRuntime& runtime);
    // IDA 0x4b84c0.
    [[nodiscard]] static std::uint32_t GetFirstElement(
        std::uint32_t tree, std::uint32_t output,
        IScenarioDialogLifecycleRuntime& runtime);
    // IDA 0x4b71f0.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, IScenarioDialogLifecycleRuntime& runtime);
    // IDA 0x4b7300.
    [[nodiscard]] static std::uint32_t SetPortrait(
        std::uint32_t object, const std::string& name,
        std::uint32_t position_option,
        IScenarioPortraitRuntime& runtime);
    // IDA 0x4b7e00.
    [[nodiscard]] static std::uint32_t SetText(
        std::uint32_t object, std::uint32_t source,
        std::uint32_t editbox_value,
        IScenarioTextRuntime& runtime);
    // IDA 0x4b7fb0.
    [[nodiscard]] static std::uint8_t HandleEditboxMouseUp(
        std::uint32_t object, std::uint32_t event,
        IScenarioInputEventRuntime& runtime);
    // IDA 0x4b7fd0. The handler context points to object+40.
    [[nodiscard]] static std::uint8_t HandleEditboxKey(
        std::uint32_t context, std::uint32_t event,
        IScenarioInputEventRuntime& runtime);
    // IDA 0x4b7ff0.
    [[nodiscard]] static std::uint8_t AlwaysTrue(
        std::uint32_t unused_event);
    // IDA 0x4b8000.
    [[nodiscard]] static std::uint32_t RegisterEditboxEvent(
        std::uint32_t object, IScenarioEventRuntime& runtime);
    // IDA 0x4b81c0: one stack argument (side); text bounds use
    // the DLL font measurement of "AA" in every portrait branch.
    [[nodiscard]] static std::uint32_t Layout(
        std::uint32_t object, std::uint8_t side,
        IScenarioLayoutRuntime& runtime);
    // IDA 0x4b8080.
    [[nodiscard]] static std::uint32_t HideAndClear(
        std::uint32_t object,
        IScenarioVisibilityRuntime& runtime);
    // IDA 0x4b8450.
    static void HandleTextDisplay(
        std::uint32_t object,
        IScenarioTextDisplayRuntime& runtime);
};

}  // namespace pal4
