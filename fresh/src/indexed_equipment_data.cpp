#include "pal4/indexed_equipment_data.h"

namespace pal4 {

std::uint8_t IndexedEquipmentDataRuntimeStub::ReadByte(
    std::uint32_t) { return 0u; }
void IndexedEquipmentDataRuntimeStub::WriteByte(
    std::uint32_t, std::uint8_t) {}
std::uint32_t IndexedEquipmentDataRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void IndexedEquipmentDataRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
void IndexedEquipmentDataRuntimeStub::SetErrorFlag(
    std::uint32_t, std::uint8_t) {}
void IndexedEquipmentDataRuntimeStub::ReallocateText(
    std::uint32_t, std::uint32_t) {}
void IndexedEquipmentDataRuntimeStub::CopyErrorText(std::uint32_t) {}
void IndexedEquipmentDataRuntimeStub::RegisterCleanup() {}
void IndexedEquipmentDataRuntimeStub::Free(std::uint32_t) {}

std::uint32_t IndexedEquipmentData::Get(
    std::uint32_t item, std::int32_t index,
    IIndexedEquipmentDataRuntime& runtime) {
    if (index >= 0 && index < 13)
        return item + 0x98u + static_cast<std::uint32_t>(index) * 16u;
    const auto guard = runtime.ReadByte(kGuard);
    if ((guard & 1u) != 0u) return kFallback;

    runtime.WriteByte(kGuard, guard | 1u);
    runtime.WriteByte(kFallback, static_cast<std::uint8_t>(index));
    runtime.WriteDword(kFallback + 4u, 0u);
    runtime.WriteDword(kFallback + 8u, 0u);
    runtime.WriteDword(kFallback + 12u, 0u);
    runtime.SetErrorFlag(kFallback, 1u);
    runtime.ReallocateText(kFallback, 5u);
    const auto destination = runtime.ReadDword(kFallback + 4u);
    runtime.CopyErrorText(destination);
    runtime.WriteDword(kFallback + 8u, 5u);
    runtime.WriteByte(destination + 5u, 0u);
    runtime.RegisterCleanup();
    return kFallback;
}

void IndexedEquipmentData::Cleanup(
    IIndexedEquipmentDataRuntime& runtime) {
    const auto text = runtime.ReadDword(kFallback + 4u);
    if (text != 0u) {
        const auto count = runtime.ReadByte(text - 1u);
        if (count == 0u || count == 0xffu)
            runtime.Free(text - 1u);
        else
            runtime.WriteByte(text - 1u,
                              static_cast<std::uint8_t>(count - 1u));
    }
    runtime.WriteDword(kFallback + 4u, 0u);
    runtime.WriteDword(kFallback + 8u, 0u);
    runtime.WriteDword(kFallback + 12u, 0u);
}

}  // namespace pal4
