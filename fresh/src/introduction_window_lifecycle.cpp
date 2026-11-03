#include "pal4/introduction_window_lifecycle.h"

namespace pal4 {

IntroductionWindowState& IntroductionWindowLifecycle::Construct(
    IntroductionWindowState& state, std::uint32_t object_address,
    std::uint32_t name, IIntroductionWindowLifecycleRuntime& runtime) {
    runtime.LoadLayout(state, name, 0u, 0u);
    try {
        runtime.ConstructMember(state, 0x28u);
        try {
            runtime.ConstructCollections(state, 0x40u, 0x10u, 4u);
            try {
                state.Write(0xb0u, kTailVtable);
                state.Write(0u, kObjectVtable);
                state.Write(0x28u, kMemberVtable);
                runtime.InitializePanelElements(state);

                const auto first_collection = object_address + 0x40u;
                runtime.InsertFirst(first_collection, state.Read(0x48u), 1u);
                runtime.InsertNext(first_collection, state.Read(0x48u), 1u, 2u);
                for (std::uint32_t id = 8u; id <= 11u; ++id) {
                    runtime.InsertNext(object_address + 0x50u,
                                       state.Read(0x58u), 1u, id);
                }
                for (std::uint32_t id = 3u; id <= 7u; ++id) {
                    runtime.InsertNext(object_address + 0x60u,
                                       state.Read(0x68u), 1u, id);
                }
                for (std::uint32_t id = 12u; id <= 16u; ++id) {
                    runtime.InsertNext(object_address + 0x70u,
                                       state.Read(0x78u), 1u, id);
                }
                return state;
            } catch (...) {
                runtime.DestroyCollections(state, 0x40u, 0x10u, 4u);
                throw;
            }
        } catch (...) {
            runtime.CleanupMember(state, 0x28u);
            throw;
        }
    } catch (...) {
        (void)runtime.DestroyDialog(state);
        throw;
    }
}

std::uint32_t IntroductionWindowLifecycle::Destroy(
    IntroductionWindowState& state,
    IIntroductionWindowLifecycleRuntime& runtime) {
    try {
        runtime.DestroyCollections(state, 0x40u, 0x10u, 4u);
        const auto storage = state.Read(0x30u);
        runtime.FreeMemberStorage(storage);
        state.Write(0x30u, 0u);
        state.Write(0x34u, 0u);
        state.Write(0x38u, 0u);
    } catch (...) {
        runtime.CleanupMember(state, 0x28u);
        (void)runtime.DestroyDialog(state);
        throw;
    }
    return runtime.DestroyDialog(state);
}

}  // namespace pal4
