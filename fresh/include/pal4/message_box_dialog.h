#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include "pal4/dialog_event_handler.h"

namespace pal4 {

class IMessageBoxDialogKeyRuntime : public IDialogEventHandlerMemory {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void CloseWindow(std::uint32_t dialog) = 0;
    [[nodiscard]] virtual bool IsVisible(std::uint32_t window) = 0;
    virtual void InvokeHandler(std::uint32_t handler, std::uint32_t selection) = 0;
    [[nodiscard]] virtual std::uint8_t ForwardNavigationKey(std::uint32_t member, std::uint32_t event) = 0;
};

class IMessageBoxDialogConstructionRuntime : public IDialogEventHandlerMemory {
public:
    virtual ~IMessageBoxDialogConstructionRuntime() = default;
    virtual void LoadLayout(std::uint32_t dialog,
                            std::uint32_t layout_name) = 0;
    virtual void InitializeSubobject(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(
        std::uint32_t dialog) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
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
        std::uint32_t callback) = 0;
    virtual void ReleaseConnection(std::uint32_t connection) = 0;
};

class IMessageBoxDialogRuntime : public IMessageBoxDialogConstructionRuntime {
public:
    virtual void DeleteHandler(std::uint32_t handler,
                               std::uint32_t flags) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t CloneHandler(
        std::uint32_t handler) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void ProcessEvent19(std::uint32_t range,
                                std::uint32_t end,
                                std::uint32_t count,
                                std::uint32_t control_field) = 0;
    virtual void PlaySound(std::string_view file, std::uint32_t repeat) = 0;
    virtual void SetText(std::uint32_t window,
                         std::u32string_view text) = 0;
    virtual void AddToSheet(std::uint32_t root) = 0;
    virtual void SetPosition(std::uint32_t window, std::uint32_t metrics,
                             float x, float y) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window, bool enabled) = 0;
    [[nodiscard]] virtual float GetTextHeight(std::uint32_t window) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void SetHeight(std::uint32_t window, std::uint32_t metrics,
                           float height) = 0;
    [[nodiscard]] virtual float GetY(std::uint32_t window,
                                      std::uint32_t metrics) = 0;
    virtual void SetY(std::uint32_t window, std::uint32_t metrics,
                      float y) = 0;
    [[nodiscard]] virtual float GetX(std::uint32_t window,
                                      std::uint32_t metrics) = 0;
    [[nodiscard]] virtual float GetWidth(std::uint32_t window,
                                          std::uint32_t metrics) = 0;
    virtual void SetX(std::uint32_t window, std::uint32_t metrics,
                      float x) = 0;
    [[nodiscard]] virtual float GetHeight(std::uint32_t window,
                                           std::uint32_t metrics) = 0;
    virtual void ActivateSubobject(std::uint32_t address) = 0;
    virtual void SetModal(std::uint32_t window, bool enabled) = 0;
    virtual void CloseWindow(std::uint32_t dialog) = 0;
    virtual void InvokeHandler(std::uint32_t handler,
                               std::uint32_t selection) = 0;
};

class MessageBoxDialog final {
public:
    static constexpr std::uint32_t kFrameVtable = 0x844020u;
    static constexpr std::uint32_t kSubobjectVtable = 0x843ff8u;
    static constexpr std::uint32_t kKeyHandlerVtable = 0x843ff0u;
    static constexpr std::uint32_t kClickedEvent = 0x8411b8u;
    static constexpr std::uint32_t kKeyDownEvent = 0x84103cu;
    static constexpr std::uint32_t kOnClick = 0x4aac40u;
    static constexpr std::uint32_t kOnKeyDown = 0x4b9fa0u;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t dialog, std::uint32_t layout_name,
        IMessageBoxDialogConstructionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t dialog, IMessageBoxDialogRuntime& runtime);
    static void ConvertAndShow(std::uint32_t dialog,
                               std::string_view source,
                               std::uint32_t argument,
                               std::uint8_t flags,
                               std::uint32_t callback,
                               IMessageBoxDialogRuntime& runtime);
    static void ShowWithFlags(std::uint32_t dialog,
                              std::u32string_view text,
                              std::uint32_t argument,
                              std::uint8_t flags,
                              std::uint32_t callback,
                              IMessageBoxDialogRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetLastSelection(
        std::uint32_t dialog, IMessageBoxDialogRuntime& runtime);
    [[nodiscard]] static bool OnClick(std::uint32_t dialog,
                                      std::uint32_t event,
                                      IMessageBoxDialogRuntime& runtime);
    static void ReleaseResource(std::uint32_t dialog,
                                IMessageBoxDialogRuntime& runtime);
    static void CreateYesNoBox(std::uint32_t dialog,
                               IMessageBoxDialogRuntime& runtime);
    // IDA 0x4AAE00. Member is dialog+40; root KeyDown wrapper consumes the key.
    [[nodiscard]] static std::uint8_t HandleNavigationKey(std::uint32_t member,
        std::uint32_t event, IMessageBoxDialogKeyRuntime& runtime);
};

}  // namespace pal4
