#include "pal4/message_box_dialog.h"
#include "pal4/preview_text_codec.h"
#include "pal4/range_no_op_callback.h"

#include <array>
#include <string_view>
#include <utility>

namespace pal4 {
std::uint8_t MessageBoxDialog::HandleNavigationKey(std::uint32_t member,
    std::uint32_t event, IMessageBoxDialogKeyRuntime& runtime) {
    if (runtime.ReadWord(event + 16u) != 1u)
        return runtime.ForwardNavigationKey(member, event);
    runtime.CloseWindow(member - 40u);
    const auto owner = runtime.ReadWord(member + 52u);
    if (owner != 0u) {
        const auto selection = runtime.IsVisible(runtime.ReadWord(member + 32u)) ? 2u : 4u;
        runtime.WriteWord(member + 48u, selection);
        runtime.InvokeHandler(runtime.ReadWord(owner), selection);
    }
    return 1u;
}
namespace {

struct Control {
    std::string_view suffix;
    std::uint32_t offset;
    std::uint32_t button_id;
};

constexpr std::array<Control, 6> kControls{{
    {"BtOk", 64u, 4u},
    {"BtYes", 68u, 1u},
    {"BtNo", 72u, 2u},
    {"Message", 76u, 0u},
    {"FrameMessage", 80u, 0u},
    {"FrameButton", 84u, 0u},
}};

void BindClick(const std::uint32_t dialog, const std::uint32_t button,
               IMessageBoxDialogConstructionRuntime& runtime) {
    const auto memory = runtime.Allocate(0x18u);
    const auto handler = memory == 0u ? 0u :
        DialogEventHandler::Construct(memory, MessageBoxDialog::kOnClick,
                                      0u, dialog, runtime);
    const auto connection = runtime.Subscribe(
        button, MessageBoxDialog::kClickedEvent, handler);
    if (connection != 0u) runtime.ReleaseConnection(connection);
}

void BindKeyDown(const std::uint32_t dialog,
                 IMessageBoxDialogConstructionRuntime& runtime) {
    const auto memory = runtime.Allocate(0x18u);
    if (memory != 0u)
        (void)DialogEventHandler::Construct(
            memory, MessageBoxDialog::kOnKeyDown, 0u, dialog, runtime);
    const auto connection = runtime.Subscribe(
        runtime.ReadWord(dialog + 36u), MessageBoxDialog::kKeyDownEvent,
        memory);
    if (connection != 0u) runtime.ReleaseConnection(connection);
}

}  // namespace

std::uint32_t MessageBoxDialog::Construct(
    const std::uint32_t dialog, const std::uint32_t layout_name,
    IMessageBoxDialogConstructionRuntime& runtime) {
    runtime.LoadLayout(dialog, layout_name);
    try {
        runtime.InitializeSubobject(dialog + 40u);
        runtime.WriteWord(dialog + 88u, 0u);
        runtime.WriteWord(dialog + 92u, 0u);
        runtime.WriteWord(dialog, kFrameVtable);
        runtime.WriteWord(dialog + 40u, kSubobjectVtable);
        const auto name = runtime.BeginName(dialog + 20u);
        try {
            for (const auto& control : kControls) {
                runtime.ResetName(name, dialog + 20u);
                runtime.AppendName(name, control.suffix);
                const auto window = runtime.FindWindow(name);
                runtime.WriteWord(dialog + control.offset, window);
                if (control.button_id != 0u) {
                    BindClick(dialog, window, runtime);
                    runtime.WriteWord(window + 280u,
                                      control.button_id);
                }
            }
            BindKeyDown(dialog, runtime);
        } catch (...) {
            runtime.EndName(name);
            throw;
        }
        runtime.EndName(name);
    } catch (...) {
        (void)runtime.DestroyBase(dialog);
        throw;
    }
    return dialog;
}

std::uint32_t MessageBoxDialog::Destruct(
    const std::uint32_t dialog, IMessageBoxDialogRuntime& runtime) {
    runtime.WriteWord(dialog, kFrameVtable);
    runtime.WriteWord(dialog + 40u, kSubobjectVtable);
    ReleaseResource(dialog, runtime);
    runtime.Free(runtime.ReadWord(dialog + 48u));
    runtime.WriteWord(dialog + 48u, 0u);
    runtime.WriteWord(dialog + 52u, 0u);
    runtime.WriteWord(dialog + 56u, 0u);
    return runtime.DestroyBase(dialog);
}

void MessageBoxDialog::ConvertAndShow(
    const std::uint32_t dialog, const std::string_view source,
    const std::uint32_t argument, const std::uint8_t flags,
    const std::uint32_t callback, IMessageBoxDialogRuntime& runtime) {
    const auto utf8 = runtime.ConvertToUtf8(source);
    const auto text = PreviewTextCodec::DecodeUtf8(utf8);
    ShowWithFlags(dialog, text, argument, flags, callback, runtime);
}

void MessageBoxDialog::ShowWithFlags(
    const std::uint32_t dialog, const std::u32string_view text,
    const std::uint32_t argument, const std::uint8_t flags,
    const std::uint32_t callback, IMessageBoxDialogRuntime& runtime) {
    (void)argument;
    const auto begin = runtime.ReadWord(dialog + 48u);
    RangeNoOpCallback::Invoke(begin, runtime.ReadWord(dialog + 52u));
    runtime.WriteWord(dialog + 52u, begin);
    runtime.WriteWord(dialog + 60u, 0u);
    runtime.WriteWord(dialog + 88u, 2u);

    if (callback != 0u) {
        ReleaseResource(dialog, runtime);
        const auto memory = runtime.Allocate(4u);
        if (memory != 0u)
            runtime.WriteWord(memory,
                              runtime.CloneHandler(runtime.ReadWord(callback)));
        runtime.WriteWord(dialog + 92u, memory);
    }

    const auto ok = runtime.ReadWord(dialog + 64u);
    if ((flags & 4u) != 0u) {
        runtime.SetVisible(ok, true);
        runtime.ProcessEvent19(dialog + 44u, runtime.ReadWord(dialog + 52u),
                               1u, dialog + 64u);
        runtime.PlaySound("wa04.wav", 1u);
    } else {
        runtime.PlaySound("wa03.wav", 1u);
        runtime.SetVisible(ok, false);
    }
    for (const auto [mask, offset] :
         std::array<std::pair<std::uint32_t, std::uint32_t>, 2>{{
             {1u, 68u}, {2u, 72u}}}) {
        const bool visible = (flags & mask) != 0u;
        runtime.SetVisible(runtime.ReadWord(dialog + offset), visible);
        if (visible)
            runtime.ProcessEvent19(dialog + 44u,
                                   runtime.ReadWord(dialog + 52u), 1u,
                                   dialog + offset);
    }

    runtime.SetText(runtime.ReadWord(dialog + 76u), text);
    const auto root = runtime.ReadWord(dialog + 36u);
    runtime.AddToSheet(root);
    runtime.SetPosition(root, 0u, 0.5f, 0.5f);
    runtime.SetAlwaysOnTop(root, true);
    CreateYesNoBox(dialog, runtime);
    runtime.ActivateSubobject(dialog + 40u);
    runtime.SetModal(root, true);
}

std::uint32_t MessageBoxDialog::GetLastSelection(
    const std::uint32_t dialog, IMessageBoxDialogRuntime& runtime) {
    return runtime.ReadWord(dialog + 88u);
}

bool MessageBoxDialog::OnClick(const std::uint32_t dialog,
                               const std::uint32_t event,
                               IMessageBoxDialogRuntime& runtime) {
    const auto control = runtime.ReadWord(event + 8u);
    const auto selection = runtime.ReadWord(control + 280u);
    runtime.WriteWord(dialog + 88u, selection);
    runtime.CloseWindow(dialog);
    const auto owner = runtime.ReadWord(dialog + 92u);
    if (owner != 0u)
        runtime.InvokeHandler(runtime.ReadWord(owner), selection);
    return true;
}

void MessageBoxDialog::ReleaseResource(
    const std::uint32_t dialog, IMessageBoxDialogRuntime& runtime) {
    const auto owner = runtime.ReadWord(dialog + 92u);
    if (owner == 0u) return;
    const auto handler = runtime.ReadWord(owner);
    if (handler != 0u) runtime.DeleteHandler(handler, 1u);
    runtime.Free(owner);
    runtime.WriteWord(dialog + 92u, 0u);
}

void MessageBoxDialog::CreateYesNoBox(
    const std::uint32_t dialog, IMessageBoxDialogRuntime& runtime) {
    const auto message = runtime.ReadWord(dialog + 76u);
    const auto message_frame = runtime.ReadWord(dialog + 80u);
    const auto button_frame = runtime.ReadWord(dialog + 84u);
    const auto root = runtime.ReadWord(dialog + 36u);
    const float text_height = runtime.GetTextHeight(message) + 8.0f;
    const float delta = text_height - runtime.ReadFloat(message + 272u);
    runtime.SetHeight(message, 1u, text_height);
    runtime.SetHeight(message_frame, 1u,
                      runtime.ReadFloat(message_frame + 272u) + delta);
    for (const auto offset : {84u, 64u, 68u, 72u}) {
        const auto window = runtime.ReadWord(dialog + offset);
        runtime.SetY(window, 1u, runtime.GetY(window, 1u) + delta);
    }
    runtime.SetX(root, 1u,
                 runtime.GetX(root, 1u) -
                     runtime.GetWidth(message_frame, 1u) * 0.5f);
    runtime.SetY(root, 1u,
                 runtime.GetY(root, 1u) -
                     (runtime.GetHeight(message_frame, 1u) +
                      runtime.GetHeight(button_frame, 1u) * 0.5f));
}

}  // namespace pal4
