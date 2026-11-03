#include "pal4/player_manager_slot_data_copier.h"

namespace pal4 {

std::uint32_t PlayerManagerSlotCopyRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void PlayerManagerSlotCopyRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
std::uint8_t PlayerManagerSlotCopyRuntimeStub::ReadByte(
    std::uint32_t) { return 0u; }
void PlayerManagerSlotCopyRuntimeStub::WriteByte(
    std::uint32_t, std::uint8_t) {}
std::uint32_t PlayerManagerSlotCopyRuntimeStub::EmptyTextAddress() {
    return PlayerManagerSlotDataCopier::kEmptyTextAddress;
}
void PlayerManagerSlotCopyRuntimeStub::EraseText(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void PlayerManagerSlotCopyRuntimeStub::SetErrorFlag(
    std::uint32_t, std::uint8_t) {}
bool PlayerManagerSlotCopyRuntimeStub::AllocateText(
    std::uint32_t, std::uint32_t, std::uint8_t) { return false; }
void PlayerManagerSlotCopyRuntimeStub::CopyBytes(
    std::uint32_t, std::uint32_t, std::uint32_t) {}

void PlayerManagerSlotDataCopier::CopyTextRecord(
    std::uint32_t destination, std::uint32_t source,
    IPlayerManagerSlotCopyRuntime& runtime) {
    const auto length = runtime.ReadDword(source + 8u);
    if (destination == source) {
        runtime.EraseText(destination, length, 0xffffffffu);
        runtime.EraseText(destination, 0u, 0u);
        return;
    }

    if (length != 0u) {
        auto text = runtime.ReadDword(source + 4u);
        if (text == 0u) text = runtime.EmptyTextAddress();
        if (runtime.ReadByte(text - 1u) < 0xfeu) {
            runtime.SetErrorFlag(destination, 1u);
            text = runtime.ReadDword(source + 4u);
            if (text == 0u) text = runtime.EmptyTextAddress();
            runtime.WriteDword(destination + 4u, text);
            runtime.WriteDword(destination + 8u,
                               runtime.ReadDword(source + 8u));
            runtime.WriteDword(destination + 12u,
                               runtime.ReadDword(source + 12u));
            runtime.WriteByte(text - 1u,
                static_cast<std::uint8_t>(
                    runtime.ReadByte(text - 1u) + 1u));
            return;
        }
    }

    if (!runtime.AllocateText(destination, length, 1u)) return;
    auto text = runtime.ReadDword(source + 4u);
    if (text == 0u) text = runtime.EmptyTextAddress();
    const auto output = runtime.ReadDword(destination + 4u);
    runtime.CopyBytes(output, text, length);
    runtime.WriteDword(destination + 8u, length);
    runtime.WriteByte(output + length, 0u);
}

std::uint32_t PlayerManagerSlotDataCopier::Copy(
    std::uint32_t object, std::uint32_t source,
    IPlayerManagerSlotCopyRuntime& runtime) {
    const auto destination_records =
        object == 0u ? 0u : object + kDestinationOffset;
    const auto source_records =
        source == 0u ? 0u : source + kSourceOffset;

    for (std::uint32_t index = 0u; index < 4u; ++index)
        CopyTextRecord(destination_records + index * 16u,
                       source_records + index * 16u, runtime);
    for (std::uint32_t index = 0u; index < 8u; ++index)
        runtime.WriteDword(destination_records + 0x40u + index * 4u,
            runtime.ReadDword(source_records + 0x40u + index * 4u));
    for (std::uint32_t index = 0u; index < 5u; ++index)
        CopyTextRecord(destination_records + 0x60u + index * 16u,
                       source_records + 0x60u + index * 16u, runtime);
    for (std::uint32_t index = 0u; index < 0x24u; ++index)
        runtime.WriteDword(object + kRawRecordOffset + index * 4u,
                           runtime.ReadDword(source + index * 4u));
    return object;
}

}  // namespace pal4
