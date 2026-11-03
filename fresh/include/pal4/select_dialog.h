#pragma once

#include <cstdint>
#include <string>
#include "pal4/ui_navigation_list.h"

namespace pal4 {

struct SelectPosition {
    std::uint32_t x{};
    std::uint32_t y{};
};

class ISelectDialogRuntime {
public:
    virtual ~ISelectDialogRuntime() = default;
    virtual void LoadLayout(std::uint32_t object,
                            const char* layout) = 0;
    virtual void ConstructNavigation(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t IndeterminateListByte() = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ReadLayoutPrefix(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::uint32_t object,
        const std::string& name) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(
        std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, const char* event,
        std::uint32_t callback) = 0;
    virtual void DestroyOwner(std::uint32_t owner,
                              std::uint32_t flags) = 0;
    [[nodiscard]] virtual SelectPosition GetPosition(
        std::uint32_t window) = 0;
    virtual void ResetAnswerCollection(
        std::uint32_t collection,
        std::uint32_t storage,
        std::uint32_t mode,
        std::uint32_t body_slot) = 0;
    virtual void RemoveChild(std::uint32_t parent,
                             std::uint32_t child) = 0;
    virtual void CleanupMemoryBlock(std::uint32_t address) = 0;
    virtual void CleanupNavigation(std::uint32_t address) = 0;
    virtual void CleanupBase(std::uint32_t object) = 0;
};

class ISelectDialogLifecycleRuntime {
public:
    virtual ~ISelectDialogLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void CleanupCallbacks(std::uint32_t object) = 0;
    virtual void SetErrorFlag(std::uint32_t record,
                              std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestructBase(
        std::uint32_t object) = 0;
};

class ISelectDialogOptionRuntime {
public:
    virtual ~ISelectDialogOptionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t size) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void CopyBuffer(std::uint32_t first,
                            std::uint32_t last,
                            std::uint32_t destination) = 0;
    virtual void CopyMultipleBuffers(std::uint32_t destination,
                                     std::uint32_t count,
                                     std::uint32_t source) = 0;
    virtual void RemoveAndReplaceBuffer(
        std::uint32_t destination,
        std::uint32_t end,
        std::uint32_t source) = 0;
    virtual void CleanupStrings(std::uint32_t begin,
                                std::uint32_t end) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ISelectDialogTextRuntime {
public:
    virtual ~ISelectDialogTextRuntime() = default;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        const std::string& source) = 0;
    virtual void InitializeList(std::uint32_t object,
                                const std::u32string& text,
                                std::uint32_t option) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyCeguiString() = 0;
};

class ISelectDialogListRuntime {
public:
    virtual ~ISelectDialogListRuntime() = default;
    virtual void CleanupCallback(std::uint32_t object) = 0;
    virtual void ClearNavigation(std::uint32_t navigation) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string ReadCString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        const std::string& source) = 0;
    [[nodiscard]] virtual std::uint32_t CreateButton(
        std::uint32_t object) = 0;
    virtual void SetText(std::uint32_t window,
                         const std::u32string& text) = 0;
    virtual void SetupMouseHandlers(std::uint32_t navigation,
                                    std::uint32_t window) = 0;
    virtual void AppendActiveButton(std::uint32_t collection,
                                    std::uint32_t storage,
                                    std::uint32_t button) = 0;
    [[nodiscard]] virtual float GetWidth(std::uint32_t window) = 0;
    [[nodiscard]] virtual float GetHeight(std::uint32_t window) = 0;
    virtual void AddChild(std::uint32_t parent,
                          std::uint32_t child) = 0;
    virtual void SetPosition(std::uint32_t window,
                             float x, float y) = 0;
    virtual void SetWidth(std::uint32_t window, float width) = 0;
    virtual void SetHeight(std::uint32_t window, float height) = 0;
    [[nodiscard]] virtual std::uint32_t DesktopWindow() = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window,
                                bool enabled) = 0;
    virtual void SetModalState(std::uint32_t window,
                               bool enabled) = 0;
    virtual void SetPositionWithMetrics(std::uint32_t window,
                                        std::uint32_t mode,
                                        float x, float y) = 0;
    [[nodiscard]] virtual std::uint32_t RefreshNavigation(
        std::uint32_t navigation) = 0;
};

class ISelectDialogCloseRuntime {
public:
    virtual ~ISelectDialogCloseRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void SetModalState(std::uint32_t window,
                               bool enabled) = 0;
    virtual void RemoveChild(std::uint32_t parent,
                             std::uint32_t child) = 0;
    virtual void AppendAvailableButton(std::uint32_t collection,
                                       std::uint32_t storage,
                                       std::uint32_t button) = 0;
    virtual void FreeString(std::uint32_t record,
                            std::uint32_t allocation,
                            std::uint32_t size) = 0;
};

class ISelectDialogCallbackRuntime {
public:
    virtual ~ISelectDialogCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void DestroyCallback(std::uint32_t callback,
                                 std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t Free(
        std::uint32_t allocation) = 0;
};

class ISelectDialogEventRuntime {
public:
    virtual ~ISelectDialogEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ForwardNavigationKey(std::uint32_t navigation,
                                      std::uint32_t event) = 0;
    virtual void HideAndClearList(std::uint32_t object) = 0;
    virtual void InvokeSelectionCallback(std::uint32_t callback,
                                         std::uint32_t index) = 0;
    virtual void CleanupCallback(std::uint32_t object) = 0;
};

enum class SelectButtonState {
    Normal,
    Hover,
    Pushed,
    Disabled,
};

class ISelectDialogButtonRuntime {
public:
    virtual ~ISelectDialogButtonRuntime() = default;
    [[nodiscard]] virtual std::uint32_t NextButtonId() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CreateWindow(
        const char* type, const std::string& name) = 0;
    virtual void CopyTextColour(std::uint32_t button,
                                std::uint32_t template_window,
                                SelectButtonState state) = 0;
    [[nodiscard]] virtual std::uint32_t GetStateImage(
        std::uint32_t template_window,
        SelectButtonState state) = 0;
    virtual void SetStateImage(std::uint32_t button,
                               SelectButtonState state,
                               std::uint32_t image) = 0;
    virtual void SetPanelImage(std::uint32_t panel,
                               std::uint32_t image) = 0;
    virtual void AddChild(std::uint32_t parent,
                          std::uint32_t child) = 0;
    [[nodiscard]] virtual SelectPosition GetMetricPosition(
        std::uint32_t window, std::uint32_t mode) = 0;
    virtual void SetMetricPosition(std::uint32_t window,
                                   std::uint32_t mode,
                                   SelectPosition position) = 0;
    [[nodiscard]] virtual float GetMetricHeight(
        std::uint32_t window, std::uint32_t mode) = 0;
    [[nodiscard]] virtual float GetMetricWidth(
        std::uint32_t window, std::uint32_t mode) = 0;
    virtual void SetMetricHeight(std::uint32_t window,
                                 std::uint32_t mode,
                                 float height) = 0;
    virtual void SetMetricWidth(std::uint32_t window,
                                std::uint32_t mode,
                                float width) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(
        std::uint32_t size) = 0;
    virtual void InitializeClickCallback(std::uint32_t allocation,
                                         std::uint32_t owner,
                                         std::uint32_t handler) = 0;
    [[nodiscard]] virtual std::uint32_t SubscribeClicked(std::uint32_t button,
                                  std::uint32_t callback) = 0;
    virtual void DestroyWindowOwner(std::uint32_t window,
                                    std::uint32_t flags) = 0;
};

class SelectDialog final {
public:
    static constexpr std::uint32_t kNavigationBaseVtable = 0x843628u;
    static constexpr std::uint32_t kNavigationVtable = 0x844364u;
    static constexpr std::uint32_t kVtable = 0x84434cu;
    static constexpr std::uint32_t kCallbackVtable = 0x844344u;
    static constexpr std::uint32_t kKeyCallback = 0x4b9fa0u;
    static constexpr std::uint32_t kClickCallback = 0x4b9fc0u;

    // IDA 0x4b9240.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object,
        ISelectDialogRuntime& runtime);

    // IDA 0x4b9630.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object,
        ISelectDialogLifecycleRuntime& runtime);

    // IDA 0x4b9700.
    [[nodiscard]] static std::uint32_t AddButton(
        std::uint32_t object, std::uint32_t source,
        ISelectDialogOptionRuntime& runtime);

    // IDA 0x4b98b0.
    [[nodiscard]] static std::uint32_t SetQuestionAndOptions(
        std::uint32_t object, const std::string& source,
        std::uint32_t option,
        ISelectDialogTextRuntime& runtime);

    // IDA 0x4b9b00.
    [[nodiscard]] static std::uint32_t InitializeList(
        std::uint32_t object, const std::u32string& question,
        std::uint32_t option,
        ISelectDialogListRuntime& runtime);

    // IDA 0x4b9e00.
    [[nodiscard]] static std::uint32_t HideAndClearList(
        std::uint32_t object, ISelectDialogCloseRuntime& runtime);

    // IDA 0x4b9f20. The native return is undefined when +64 is null.
    [[nodiscard]] static std::uint32_t CleanupCallback(
        std::uint32_t object, ISelectDialogCallbackRuntime& runtime);

    // IDA 0x4b9fa0 / 0x4b9fc0.
    [[nodiscard]] static std::uint8_t HandleKeyDown(
        std::uint32_t object, std::uint32_t event,
        ISelectDialogEventRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleClick(
        std::uint32_t object, std::uint32_t event,
        ISelectDialogEventRuntime& runtime);
    // IDA 0x4b9f50: Esc returns +160; other keys use vertical navigation.
    [[nodiscard]] static std::uint8_t HandleNavigationKey(
        std::uint32_t object, std::uint32_t event,
        ISelectDialogEventRuntime& runtime, IUiNavigationInputRuntime& navigation);

    // IDA 0x4ba000.
    [[nodiscard]] static std::uint32_t CreateButton(
        std::uint32_t object, ISelectDialogButtonRuntime& runtime);

    // IDA 0x4bae40.
    [[nodiscard]] static std::uint32_t ConstructCallback(
        std::uint32_t allocation, std::uint32_t handler,
        std::uint32_t argument, std::uint32_t owner,
        ISelectDialogCallbackRuntime& runtime);
};

}  // namespace pal4
