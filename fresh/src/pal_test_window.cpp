#include "pal4/pal_test_window.h"

namespace pal4 {

PalTestWindowState& PalTestWindow::Construct(
    PalTestWindowState& state, std::uint32_t name,
    IPalTestWindowRuntime& runtime) {
    runtime.LoadLayout(state, name, 0u, 0u);
    try {
        runtime.ConstructMember(state);
    } catch (...) {
        runtime.DestroyDialogBase(state);
        throw;
    }
    state.Write(0x6cu, 0u);
    state.bytes[0x68u] = static_cast<std::uint8_t>(name);
    state.Write(0x70u, 0u);
    state.Write(0x74u, 0u);
    state.Write(0x84u, kCallbackVtable);
    state.Write(0u, kObjectVtable);
    state.Write(0x28u, kMemberVtable);
    try {
        runtime.InitializeControls(state);
    } catch (...) {
        runtime.CleanupNameBlock(state);
        runtime.CleanupMember(state);
        runtime.DestroyDialogBase(state);
        throw;
    }
    return state;
}

void PalTestWindow::Destroy(PalTestWindowState& state,
                            IPalTestWindowRuntime& runtime) {
    runtime.Free(state.Read(0x6cu));
    state.Write(0x6cu, 0u);
    state.Write(0x70u, 0u);
    state.Write(0x74u, 0u);
    runtime.Free(state.Read(0x30u));
    state.Write(0x30u, 0u);
    state.Write(0x34u, 0u);
    state.Write(0x38u, 0u);
    runtime.DestroyDialogBase(state);
}

void PalTestWindow::SetVisible(PalTestWindowState& state,
                               std::uint32_t object_address,
                               std::uint8_t showing, std::uint32_t parent,
                               IPalTestWindowRuntime& runtime) {
    runtime.SetParent(state, showing, parent);
    const auto member = object_address == 0u ? 0u : object_address + 0x28u;
    if (showing != 0u) {
        runtime.ResetUI(state);
        const auto manager = runtime.FrameManager();
        runtime.InsertActive(manager + 0x134u, member);
        runtime.SetCurrentQuestion(state, 0u);
        runtime.UpdateSystemDialogVisibility(runtime.FrameManager(), 11u);
    } else {
        const auto manager = runtime.FrameManager();
        runtime.RemoveActive(manager + 0x134u, member);
        state.Write(0x3cu, 0u);
    }
}

}  // namespace pal4
