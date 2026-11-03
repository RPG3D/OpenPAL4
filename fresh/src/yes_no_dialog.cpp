#include "pal4/yes_no_dialog.h"

#include "pal4/range_no_op_callback.h"
#include "pal4/preview_text_codec.h"

#include <array>

namespace pal4 {
namespace {

struct Control {
    std::string_view suffix;
    std::uint32_t field;
    bool clicked;
    std::uint32_t button_id;
};

constexpr std::array<Control, 3> kControls{{
    {"BtYes", 64u, true, 1u},
    {"BtNo", 68u, true, 0u},
    {"Message", 72u, false, 0u},
}};

void SubscribeClick(const std::uint32_t dialog,
                    const std::uint32_t window,
                    IYesNoDialogRuntime& runtime) {
    const auto memory = runtime.Allocate(0x18u);
    const auto handler = memory == 0u ? 0u :
        DialogEventHandler::ConstructAlt(memory, YesNoDialog::kOnClick,
                                         0u, dialog, runtime);
    const auto connection = runtime.Subscribe(
        window, YesNoDialog::kClickedEvent, handler);
    if (connection != 0u) runtime.ReleaseConnection(connection);
}

void SubscribeKeyDown(const std::uint32_t dialog,
                      IYesNoDialogRuntime& runtime) {
    const auto memory = runtime.Allocate(0x18u);
    if (memory != 0u)
        (void)DialogEventHandler::ConstructAlt(
            memory, YesNoDialog::kOnKeyDown, 0u, dialog, runtime);
    const auto connection = runtime.Subscribe(
        runtime.ReadWord(dialog + 36u), YesNoDialog::kKeyDownEvent, memory);
    if (connection != 0u) runtime.ReleaseConnection(connection);
}

}  // namespace

std::uint32_t YesNoDialog::Construct(
    const std::uint32_t dialog, const std::string_view layout_name,
    IYesNoDialogRuntime& runtime) {
    runtime.LoadLayout(dialog, layout_name);
    try {
        runtime.InitializeSubobject(dialog + 40u);
        runtime.WriteWord(dialog, kFrameVtable);
        runtime.WriteWord(dialog + 40u, kSubobjectVtable);
        const auto name = runtime.BeginName(dialog + 20u);
        try {
            for (const auto& control : kControls) {
                runtime.ResetName(name, dialog + 20u);
                runtime.AppendName(name, control.suffix);
                const auto window = runtime.FindWindow(name);
                runtime.WriteWord(dialog + control.field, window);
                if (control.clicked) {
                    SubscribeClick(dialog, window, runtime);
                    runtime.WriteWord(window + 280u, control.button_id);
                }
            }
            SubscribeKeyDown(dialog, runtime);
            const auto begin = runtime.ReadWord(dialog + 48u);
            RangeNoOpCallback::Invoke(begin, runtime.ReadWord(dialog + 52u));
            runtime.WriteWord(dialog + 52u, begin);
            for (const auto field : {64u, 68u}) {
                const auto window = runtime.ReadWord(dialog + field);
                runtime.SetVisible(window, true);
                runtime.ProcessEvent19(dialog + 44u,
                                       runtime.ReadWord(dialog + 52u), 1u,
                                       dialog + field);
            }
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

std::uint32_t YesNoDialog::Destruct(
    const std::uint32_t dialog, IYesNoDialogRuntime& runtime) {
    runtime.WriteWord(dialog, kFrameVtable);
    runtime.WriteWord(dialog + 40u, kSubobjectVtable);
    runtime.Free(runtime.ReadWord(dialog + 48u));
    runtime.WriteWord(dialog + 48u, 0u);
    runtime.WriteWord(dialog + 52u, 0u);
    runtime.WriteWord(dialog + 56u, 0u);
    return runtime.DestroyBase(dialog);
}

void YesNoDialog::ConvertAndShow(
    const std::uint32_t dialog, const std::string_view source,
    IYesNoDialogRuntime& runtime) {
    const auto utf8 = runtime.ConvertToUtf8(source);
    const auto text = PreviewTextCodec::DecodeUtf8(utf8);
    Show(dialog, text, runtime);
}

void YesNoDialog::Show(const std::uint32_t dialog,
                       const std::u32string_view text,
                       IYesNoDialogRuntime& runtime) {
    runtime.WriteWord(dialog + 60u, 0u);
    runtime.WriteWord(dialog + 76u, 0u);
    runtime.PlaySound("wa03.wav", 1u);
    runtime.SetText(runtime.ReadWord(dialog + 72u), text);
    const auto root = runtime.ReadWord(dialog + 36u);
    runtime.AddToSheet(root);
    runtime.SetPosition(root, 0u, 0.2875f, 0.32f);
    runtime.SetAlwaysOnTop(root, true);
    runtime.ActivateSubobject(dialog + 40u);
    runtime.SetModal(root, true);
}

void YesNoDialog::Close(const std::uint32_t dialog,
                        IYesNoDialogCloseRuntime& runtime) {
    const auto root = runtime.ReadWord(dialog + 36u);
    runtime.SetModal(root, false);
    const auto parent = runtime.ReadWord(root + 68u);
    if (parent != 0u) runtime.RemoveChild(parent, root);
}

std::uint8_t YesNoDialog::HandleNavigationKey(std::uint32_t member,std::uint32_t event,
                                                IYesNoDialogKeyRuntime& runtime) {
    if(runtime.ReadWord(event+16u)!=1u)return runtime.ForwardNavigationKey(member,event);
    runtime.WriteWord(member+36u,0u);
    runtime.CloseWindow(member-40u);
    return 1u;
}

bool YesNoDialog::OnClick(const std::uint32_t dialog,
                          const std::uint32_t event,
                          IYesNoDialogRuntime& runtime) {
    const auto control = runtime.ReadWord(event + 8u);
    runtime.WriteWord(dialog + 76u, runtime.ReadWord(control + 280u));
    Close(dialog, runtime);
    return true;
}

}  // namespace pal4
