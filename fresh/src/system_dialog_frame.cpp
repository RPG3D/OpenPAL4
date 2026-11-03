#include "pal4/system_dialog_frame.h"

namespace pal4 {
namespace {

void Bind(const std::uint32_t frame, const std::uint32_t window,
          const std::uint32_t event_address,
          const std::uint32_t function,
          ISystemDialogFrameRuntime& runtime) {
    const auto callback = runtime.Allocate(12u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, function);
        runtime.WriteWord(callback + 8u, frame);
        runtime.WriteWord(callback, SystemDialogFrame::kCallbackVtable);
    }
    const auto connection = runtime.Subscribe(
        window, runtime.ReadWord(event_address), callback);
    if (connection != 0u) {
        runtime.ReleaseConnection(connection);
    }
}

}  // namespace

std::uint32_t SystemDialogFrame::Construct(
    const std::uint32_t frame, const std::uint32_t name,
    ISystemDialogFrameRuntime& runtime) {
    runtime.LoadLayout(frame, name);
    try {
        runtime.WriteWord(frame, kFrameVtable);
        const auto buffer = runtime.BeginName(frame + 20u);
        try {
            runtime.AppendName(buffer, "EditInfo");
            runtime.WriteWord(frame + 40u,
                              runtime.FindWindow(frame, buffer));
            runtime.ResetName(buffer, frame + 20u);
            runtime.AppendName(buffer, "BtnOk");
            const auto button = runtime.FindWindow(frame, buffer);
            runtime.WriteWord(frame + 44u, button);
            Bind(frame, button, kClickedEvent, kOnOkClick, runtime);
            Bind(frame, runtime.ReadWord(frame + 36u),
                 kKeyDownEvent, kOnKeyDown, runtime);
        } catch (...) {
            runtime.EndName(buffer);
            throw;
        }
        runtime.EndName(buffer);
    } catch (...) {
        runtime.DestroyBase(frame);
        throw;
    }
    return frame;
}

}  // namespace pal4
