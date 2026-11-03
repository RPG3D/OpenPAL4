#include "pal4/role_state_receiver_names.h"

namespace pal4 {

std::uint32_t RoleStateReceiverNames::CreateDrag(
    PreviewTitleStringState& state, IPreviewTitleStringRuntime& runtime) {
    return PreviewTitleString::Initialize(state, runtime, "DragReceiver",
                                          kDragInline);
}

int RoleStateReceiverNames::InitializeDrag(
    PreviewTitleStringState& state, IPreviewTitleStringRuntime& runtime) {
    static_cast<void>(CreateDrag(state, runtime));
    return runtime.RegisterAtExit(kDragCleanup);
}

void RoleStateReceiverNames::DestroyDrag(
    PreviewTitleStringState& state, IPreviewTitleStringRuntime& runtime) {
    runtime.Destroy(state);
}

std::uint32_t RoleStateReceiverNames::CreateCan(
    PreviewTitleStringState& state, IPreviewTitleStringRuntime& runtime) {
    return PreviewTitleString::Initialize(state, runtime, "CanReceiver",
                                          kCanInline);
}

int RoleStateReceiverNames::InitializeCan(
    PreviewTitleStringState& state, IPreviewTitleStringRuntime& runtime) {
    static_cast<void>(CreateCan(state, runtime));
    return runtime.RegisterAtExit(kCanCleanup);
}

void RoleStateReceiverNames::DestroyCan(
    PreviewTitleStringState& state, IPreviewTitleStringRuntime& runtime) {
    runtime.Destroy(state);
}

}  // namespace pal4
