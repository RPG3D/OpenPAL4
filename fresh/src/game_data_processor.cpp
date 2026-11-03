#include "pal4/game_data_processor.h"

namespace pal4 {

std::uint32_t GameDataProcessorRuntimeStub::GetGameDatabase() {
    return 0u;
}
std::uint32_t GameDataProcessorRuntimeStub::LookupPrimary(
    std::uint32_t, std::uint32_t) {
    return 0u;
}
std::uint32_t GameDataProcessorRuntimeStub::LookupSoundPrimary(
    std::uint32_t, std::uint32_t) {
    return 0u;
}
std::uint32_t GameDataProcessorRuntimeStub::LookupSecondary(
    std::uint32_t, std::uint32_t) {
    return 0u;
}
std::uint32_t GameDataProcessorRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
void GameDataProcessorRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
std::uint32_t GameDataProcessorRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
void GameDataProcessorRuntimeStub::Free(std::uint32_t) {}

namespace {

std::uint32_t ProcessAndInsertVariant(
    IndexedFilteredResultArrayState& result, std::uint32_t id,
    bool sound_primary, IGameDataProcessorRuntime& runtime) {
    const auto first_database = runtime.GetGameDatabase();
    const auto primary = sound_primary ?
        runtime.LookupSoundPrimary(first_database + 12u, id) :
        runtime.LookupPrimary(first_database + 12u, id);
    if (primary == 0u)
        return 0u;

    const auto second_database = runtime.GetGameDatabase();
    const auto secondary_key = runtime.ReadDword(
        primary + (sound_primary ? 0x70u : 0x48u));
    const auto secondary = runtime.LookupSecondary(
        second_database + 12u, secondary_key);
    if (secondary == 0u)
        return 0u;
    if (runtime.ReadDword(secondary + 12u) != 0xffffffffu)
        return secondary & 0xffffff00u;

    const auto value = runtime.ReadDword(secondary);
    return DynamicDwordResultArray::Append(result, value, runtime);
}

}  // namespace

std::uint32_t GameDataProcessor::ProcessAndInsert(
    IndexedFilteredResultArrayState& result, std::uint32_t id,
    IGameDataProcessorRuntime& runtime) {
    return ProcessAndInsertVariant(result, id, false, runtime);
}

std::uint32_t GameDataProcessor::ProcessSoundAndInsert(
    IndexedFilteredResultArrayState& result, std::uint32_t id,
    IGameDataProcessorRuntime& runtime) {
    return ProcessAndInsertVariant(result, id, true, runtime);
}

}  // namespace pal4
