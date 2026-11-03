#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

// The shared 32-byte script wait layout for common/select dialogs.
struct ScriptDialogWaitState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 28> base{};
};
static_assert(sizeof(ScriptDialogWaitState) == 0x20u);

// Native UI dialog, XML and script-queue operations.
class IScriptDialogsRuntime {
public:
    virtual ~IScriptDialogsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateCommonDialog(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t CreateSelectDialog(
        std::uint32_t manager) = 0;
    virtual void ShowCommon(std::string_view text,
                            std::int32_t first,
                            std::int32_t second,
                            std::uint32_t mode,
                            std::uint32_t flags) = 0;
    virtual void SetSelectQuestion(std::string_view text,
                                   std::uint32_t option) = 0;
    [[nodiscard]] virtual ScriptDialogWaitState* AllocateWait(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ConstructWait(ScriptDialogWaitState& state,
                               std::uint8_t mode,
                               std::uint32_t queue) = 0;
    virtual void FlushWait(ScriptDialogWaitState& state) = 0;
    [[nodiscard]] virtual std::uint32_t AddHandler(
        std::uint32_t queue, ScriptDialogWaitState* state) = 0;
    [[nodiscard]] virtual bool IsVisible(std::uint32_t dialog) = 0;
    virtual void CloseWait(ScriptDialogWaitState& state) = 0;
    [[nodiscard]] virtual std::uint32_t CreateDocument() = 0;
    virtual void UpsertPointerAttribute(std::uint32_t document,
                                        std::string_view key,
                                        std::uint32_t value) = 0;
    virtual void EnterState(std::uint32_t state,
                            std::uint32_t document) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyDocument(
        std::uint32_t document) = 0;
};

// Script entrypoints for common and select dialogs and their wait polls.
class ScriptDialogs final {
public:
    static constexpr std::uint32_t kCommonVtable = 0x849648u;
    static constexpr std::uint32_t kSelectVtable = 0x849654u;
    static constexpr std::uint32_t kQueueOffset = 0x54u;

    explicit ScriptDialogs(IScriptDialogsRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e7a30: show common message and return queued handler result.
    [[nodiscard]] std::uint32_t ShowCommon(std::string_view text,
                                           std::int32_t first,
                                           std::int32_t second);
    // 0x5e8d00: show common dialog in select mode (mode three).
    [[nodiscard]] std::uint32_t ShowCommonInSelectMode(
        std::string_view text, std::int32_t first, std::int32_t second);
    // 0x5e7af0: close common wait when dialog is no longer visible.
    void UpdateCommon(ScriptDialogWaitState& state);
    // 0x5e7b20: show select dialog, enter state 10 and return XML cleanup.
    [[nodiscard]] std::uint32_t ShowSelect(std::string_view text);
    // 0x5e7cc0: close select wait when dialog is no longer visible.
    void UpdateSelect(ScriptDialogWaitState& state);

private:
    [[nodiscard]] std::uint32_t ShowCommonWithMode(
        std::string_view text, std::int32_t first, std::int32_t second,
        std::uint32_t mode);
    void QueueWait(std::uint32_t vtable);
    [[nodiscard]] std::uint32_t QueueWaitWithResult(std::uint32_t vtable);

    IScriptDialogsRuntime& runtime_;
};

}  // namespace pal4
