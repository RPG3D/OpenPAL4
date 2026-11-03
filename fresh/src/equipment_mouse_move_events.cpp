#include "pal4/equipment_mouse_move_events.h"

#include <cstdint>

namespace pal4 {

bool EquipmentMouseMoveEvents::Register(
    EquipmentWindowState& state, std::uint32_t member_address,
    IEquipmentMouseMoveRegistrationRuntime& runtime) {
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        const auto callback = runtime.AllocateCallback(12u);
        if (callback != 0u) {
            runtime.WriteWord(callback, kCallbackVtable);
            runtime.WriteWord(callback + 4u, kCallbackHandler);
            runtime.WriteWord(callback + 8u, member_address);
        }
        const auto reference = runtime.SubscribeMouseMove(
            state.Read(0x98u + 4u * index), callback);
        if (reference != 0u) {
            const auto remaining = runtime.ReferenceCount(reference) - 1u;
            runtime.SetReferenceCount(reference, remaining);
            if (remaining == 0u)
                runtime.DeleteReference(reference, 1u);
        }
    }
    return true;
}

bool EquipmentMouseMoveEvents::Handle(
    EquipmentWindowState& state, std::uint32_t event,
    IEquipmentMouseMoveHandlingRuntime& runtime) {
    const auto source = runtime.ReadWord(event + 8u);
    for (std::uint32_t index = 0u; index < 6u; ++index) {
        const auto window = state.Read(0x98u + 4u * index);
        if (!runtime.IsDisabled(window) && window != source) {
            runtime.SetHoverFlag(window, false);
            runtime.RequestRedraw(window);
        }
    }
    return true;
}

int EquipmentMouseMoveEvents::InvokeMemberVirtual(
    std::uint32_t member, IEquipmentMouseMoveHandlingRuntime& runtime) {
    return runtime.CallMemberVirtual(member, 0x30u);
}

}  // namespace pal4
