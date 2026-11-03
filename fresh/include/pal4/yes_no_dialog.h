#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include "pal4/dialog_event_handler.h"

namespace pal4 {

class IYesNoDialogCloseRuntime {
public:
    virtual ~IYesNoDialogCloseRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void SetModal(std::uint32_t root, bool enabled) = 0;
    virtual void RemoveChild(std::uint32_t parent, std::uint32_t child) = 0;
};

class IYesNoDialogRuntime : public IDialogEventHandlerMemory, public IYesNoDialogCloseRuntime {
public:
    virtual ~IYesNoDialogRuntime() = default;
    virtual void LoadLayout(std::uint32_t dialog,
                            std::string_view layout_name) = 0;
    virtual void InitializeSubobject(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(
        std::uint32_t dialog) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t BeginName(
        std::uint32_t source) = 0;
    virtual void ResetName(std::uint32_t buffer,
                           std::uint32_t source) = 0;
    virtual void AppendName(std::uint32_t buffer,
                            std::string_view suffix) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::uint32_t buffer) = 0;
    virtual void EndName(std::uint32_t buffer) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, std::uint32_t event,
        std::uint32_t handler) = 0;
    virtual void ReleaseConnection(std::uint32_t connection) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void ProcessEvent19(std::uint32_t range,
                                std::uint32_t end,
                                std::uint32_t count,
                                std::uint32_t control_field) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    virtual void PlaySound(std::string_view file,
                           std::uint32_t repeat) = 0;
    virtual void SetText(std::uint32_t window,
                         std::u32string_view text) = 0;
    virtual void AddToSheet(std::uint32_t root) = 0;
    virtual void SetPosition(std::uint32_t root,
                             std::uint32_t metrics,
                             float x, float y) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t root,
                                 bool enabled) = 0;
    virtual void ActivateSubobject(std::uint32_t address) = 0;
    virtual void SetModal(std::uint32_t root, bool enabled) = 0;
    virtual void RemoveChild(std::uint32_t parent,
                             std::uint32_t child) = 0;
};

class IYesNoDialogKeyRuntime {
public:
    virtual ~IYesNoDialogKeyRuntime() = default;
    virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void CloseWindow(std::uint32_t dialog) = 0;
    virtual std::uint8_t ForwardNavigationKey(std::uint32_t member,std::uint32_t event) = 0;
};

class YesNoDialog final {
public:
    static constexpr std::uint32_t kFrameVtable = 0x84406cu;
    static constexpr std::uint32_t kSubobjectVtable = 0x844044u;
    static constexpr std::uint32_t kKeyHandlerVtable = 0x84403cu;
    static constexpr std::uint32_t kClickedEvent = 0x8411b8u;
    static constexpr std::uint32_t kKeyDownEvent = 0x84103cu;
    static constexpr std::uint32_t kOnClick = 0x4ab630u;
    static constexpr std::uint32_t kOnKeyDown = 0x4b9fa0u;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t dialog, std::string_view layout_name,
        IYesNoDialogRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t dialog, IYesNoDialogRuntime& runtime);
    static void ConvertAndShow(std::uint32_t dialog,
                               std::string_view source,
                               IYesNoDialogRuntime& runtime);
    static void Show(std::uint32_t dialog, std::u32string_view text,
                     IYesNoDialogRuntime& runtime);
    // IDA4AB650: Esc stores No(0); all other keys use original navigation.
    static std::uint8_t HandleNavigationKey(std::uint32_t member,std::uint32_t event,
                                           IYesNoDialogKeyRuntime& runtime);
    static void Close(std::uint32_t dialog,
                      IYesNoDialogCloseRuntime& runtime);
    [[nodiscard]] static bool OnClick(std::uint32_t dialog,
                                      std::uint32_t event,
                                      IYesNoDialogRuntime& runtime);
};

}  // namespace pal4
