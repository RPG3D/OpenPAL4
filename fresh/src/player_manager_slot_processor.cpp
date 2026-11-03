#include "pal4/player_manager_slot_processor.h"

#include "pal4/player_manager_slot_collection_processor.h"

namespace pal4 {

std::uint32_t PlayerManagerSlotProcessRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void PlayerManagerSlotProcessRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
void PlayerManagerSlotProcessRuntimeStub::ClearCollectionFirstFive(
    std::uint32_t) {}
void PlayerManagerSlotProcessRuntimeStub::ClearFixedSortedValues(
    std::uint32_t) {}
void PlayerManagerSlotProcessRuntimeStub::ResetInventorySlots(
    std::uint32_t) {}
void PlayerManagerSlotProcessRuntimeStub::CleanupScriptLists(
    std::uint32_t) {}
void PlayerManagerSlotProcessRuntimeStub::InitializeMember(
    std::uint32_t) {}
std::uint32_t PlayerManagerSlotProcessRuntimeStub::GameDatabase() {
    return 0u;
}
std::uint32_t PlayerManagerSlotProcessRuntimeStub::FindRecord(
    std::uint32_t, std::uint32_t) { return 0u; }
IPlayerManagerSlotCopyRuntime&
PlayerManagerSlotProcessRuntimeStub::CopyRuntime() {
    return copy_runtime_;
}
void PlayerManagerSlotProcessRuntimeStub::ProcessCollection(
    std::uint32_t object) {
    PlayerManagerSlotCollectionRuntimeStub collection_runtime;
    static_cast<void>(PlayerManagerSlotCollectionProcessor::Process(
        object, 0u, collection_runtime));
}
void PlayerManagerSlotProcessRuntimeStub::UpdateEvent(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void PlayerManagerSlotProcessRuntimeStub::FinalizeScript(
    std::uint32_t) {}
std::uint32_t PlayerManagerSlotProcessRuntimeStub::GetElementByIndex(
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t PlayerManagerSlotProcessRuntimeStub::SetProperty(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }

std::uint32_t PlayerManagerSlotProcessor::Process(
    std::uint32_t object, IPlayerManagerSlotProcessRuntime& runtime) {
    const auto collections = object + 0x938u;
    const auto script = object + 0x654u;
    const auto data = object + 0x7a8u;
    runtime.ClearCollectionFirstFive(collections);
    runtime.ClearFixedSortedValues(collections);
    runtime.ResetInventorySlots(object + 0x920u);
    runtime.CleanupScriptLists(script);
    runtime.InitializeMember(object + 0x658u);
    runtime.InitializeMember(data);

    const auto key = runtime.ReadDword(object + 0xa48u);
    const auto database = runtime.GameDatabase();
    const auto record = runtime.FindRecord(database + 0x20u, key);
    static_cast<void>(PlayerManagerSlotDataCopier::Copy(
        object, record, runtime.CopyRuntime()));
    runtime.ProcessCollection(object);
    runtime.UpdateEvent(script, 1u,
                        runtime.ReadDword(object + 0xa48u));
    runtime.FinalizeScript(script);

    const auto first = static_cast<std::int32_t>(
        runtime.ReadDword(object + 0x7acu));
    runtime.WriteDword(data + 0xe8u,
                       static_cast<std::uint32_t>(first));
    const auto first_max = static_cast<std::int32_t>(
        runtime.GetElementByIndex(data, 0u));
    if (first > first_max) {
        runtime.WriteDword(data + 0xe8u,
                           runtime.GetElementByIndex(data, 0u));
    } else if (first < 0) {
        runtime.WriteDword(data + 0xe8u, 0u);
    }

    const auto second = static_cast<std::int32_t>(
        runtime.ReadDword(object + 0x7b4u));
    runtime.WriteDword(data + 0xf0u,
                       static_cast<std::uint32_t>(second));
    const auto second_max = static_cast<std::int32_t>(
        runtime.GetElementByIndex(data, 2u));
    if (second > second_max) {
        runtime.WriteDword(data + 0xf0u,
                           runtime.GetElementByIndex(data, 2u));
    } else if (second < 0) {
        runtime.WriteDword(data + 0xf0u, 0u);
    }
    return runtime.SetProperty(data, 8u, 0u);
}

}  // namespace pal4
