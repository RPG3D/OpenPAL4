#include "pal4/equipment_event_callback_cloner.h"

#include <cstdint>

namespace pal4 {
namespace {

std::uint32_t Clone(std::uint32_t source,
                    std::uint32_t provisional_vtable,
                    std::uint32_t final_vtable,
                    IEquipmentEventCallbackCloneRuntime& runtime) {
    const auto target = runtime.Allocate(24u);
    if (target == 0u) return 0u;
    runtime.WriteWord(target, provisional_vtable);
    for (std::uint32_t index = 2u; index <= 4u; ++index)
        runtime.WriteWord(target + 4u * index,
                          runtime.ReadWord(source + 4u * index));
    runtime.WriteWord(target, final_vtable);
    return target;
}

}  // namespace

std::uint32_t EquipmentEventCallbackCloner::CloneCommand(
    std::uint32_t source,
    IEquipmentEventCallbackCloneRuntime& runtime) {
    return Clone(source, 0x843500u, 0x84382cu, runtime);
}

std::uint32_t EquipmentEventCallbackCloner::CloneEvent(
    std::uint32_t source,
    IEquipmentEventCallbackCloneRuntime& runtime) {
    return Clone(source, 0x8435d0u, 0x8438c4u, runtime);
}

}  // namespace pal4
