#include "pal4/magic_window.h"

namespace pal4 {

MagicWindowState& MagicWindow::Construct(std::uint32_t object,
                                        std::uint32_t name,
                                        MagicWindowState& state,
                                        IMagicWindowRuntime& runtime) {
    runtime.LoadLayout(object, name);
    runtime.ConstructMember(object + 0x28u);
    runtime.ConstructTextArray(object + 0x170u, 7u);
    state.member_vtable = kMemberVtable;
    state.selection_vtable = kSelectionVtable;
    state.list_vtable = kListVtable;
    state.event_vtable = kEventVtable;
    state.vtable = kVtable;

    auto callback = runtime.Allocate(0x18u);
    if (callback != 0) {
        runtime.WriteWord(callback + 8u, kCallbackHandler);
        runtime.WriteWord(callback + 0xcu, 0u);
        runtime.WriteWord(callback + 0x10u, object);
        runtime.WriteWord(callback, kCallbackVtable);
    }
    try {
        const auto manager = runtime.FrameManager();
        runtime.InsertOrReplace(manager + 0x104u, "use_stunt", callback);
    } catch (...) {
        if (callback != 0) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0) runtime.DeleteCallback(callback, 1u);

    state.category_position = 1u;
    state.slot_order = {5u, 0u, 1u, 2u, 3u, 4u};
    for (std::uint32_t index = 0; index < 7u; ++index) {
        if (runtime.HasStuntText(index)) {
            state.stunt_text[index] = runtime.ReadStuntText(index);
        }
    }
    runtime.InitializePanel(object);
    state.list_owner = object;
    state.selection_owner = object;
    return state;
}

std::uint32_t MagicWindow::Destroy(std::uint32_t object,
                                   MagicWindowState& state,
                                   IMagicWindowRuntime& runtime) {
    runtime.DestroyTextArray(object + 0x170u, 7u);
    runtime.FreeMemberStorage(state.member_storage);
    state.member_storage = 0;
    state.member_size = 0;
    state.member_capacity = 0;
    return runtime.DestroyBase(object);
}

}  // namespace pal4
