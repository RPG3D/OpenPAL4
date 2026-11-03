#include "pal4/player_manager_slot_collection_processor.h"

namespace pal4 {

std::uint32_t PlayerManagerSlotCollectionRuntimeStub::Allocate(
    std::uint32_t) { return 0u; }
std::uint32_t PlayerManagerSlotCollectionRuntimeStub::Free(
    std::uint32_t) { return 0u; }
std::uint32_t PlayerManagerSlotCollectionRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void PlayerManagerSlotCollectionRuntimeStub::ProcessInventory(
    std::uint32_t, PlayerManagerResultArray&) {}
void PlayerManagerSlotCollectionRuntimeStub::ProcessIndexedValues(
    std::uint32_t, PlayerManagerResultArray&) {}
void PlayerManagerSlotCollectionRuntimeStub::ProcessFixedValues(
    std::uint32_t, PlayerManagerResultArray&) {}
void PlayerManagerSlotCollectionRuntimeStub::HandleFade(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void PlayerManagerSlotCollectionRuntimeStub::FinalizeScript(
    std::uint32_t) {}

std::uint32_t PlayerManagerSlotCollectionProcessor::Process(
    std::uint32_t object, std::uint8_t incoming_stack_byte,
    IPlayerManagerSlotCollectionRuntime& runtime) {
    PlayerManagerResultArray result{incoming_stack_byte, 0u, 0u, 0u};
    try {
        const auto allocation = runtime.Allocate(0x80u);
        static_cast<void>(runtime.Free(0u));
        result.capacity_end = allocation + 0x80u;
        result.end = allocation;
        result.begin = allocation;

        runtime.ProcessInventory(object + 0x920u, result);
        runtime.ProcessIndexedValues(object + 0x938u, result);
        runtime.ProcessFixedValues(object + 0x938u, result);
        const auto script = object + 0x654u;
        for (auto cursor = result.begin; cursor != result.end;
             cursor += 4u)
            runtime.HandleFade(script, runtime.ReadDword(cursor),
                               object == 0u ? 0u : script);
        runtime.FinalizeScript(script);
        return runtime.Free(result.begin);
    } catch (...) {
        static_cast<void>(runtime.Free(result.begin));
        throw;
    }
}

}  // namespace pal4
