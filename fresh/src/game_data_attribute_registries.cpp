#include "pal4/game_data_attribute_registries.h"

#include <array>
#include <string_view>
#include <utility>

namespace pal4 {
namespace {

struct LeadingRecord {
    std::uint32_t id;
    std::string_view name;
};

constexpr std::array<LeadingRecord, 1> kConditionLeading{{
    {0, "AI_TYPE"},
}};

constexpr std::array<LeadingRecord, 4> kParameterLeading{{
    {0, "ID"},
    {4, "AI_TYPE"},
    {8, "TEAM_STATE"},
    {12, "ENEMY_STATE"},
}};

std::uint32_t CollectionAddress(GameDataAttributeGroup group) {
    return group == GameDataAttributeGroup::conditions ? 0x8ee60c : 0x8ee5d8;
}

}  // namespace

std::uint32_t GameDataAttributeRegistries::Initialize(
    State& state, GameDataAttributeGroup group,
    std::uint8_t first_seed, std::uint8_t second_seed,
    IGameDataAttributeRegistriesRuntime& runtime) {
    if (!state.storage_initialized) {
        state.storage_initialized = true;
        runtime.InitializeStorage(group, first_seed, second_seed);
        runtime.RegisterCleanup(group);
    }
    if (state.records_initialized) return CollectionAddress(group);
    state.records_initialized = true;

    const auto append = [&](GameDataProperty record) {
        runtime.AppendRecord(group, record, second_seed);
        state.records.push_back(std::move(record));
    };
    if (group == GameDataAttributeGroup::conditions) {
        for (const LeadingRecord& entry : kConditionLeading)
            append({entry.id, 0, GameDataPropertyKind::numeric,
                    std::string(entry.name) + runtime.ErrorSuffix(), 0});
    } else {
        for (const LeadingRecord& entry : kParameterLeading)
            append({entry.id, 0, GameDataPropertyKind::numeric,
                    std::string(entry.name) + runtime.ErrorSuffix(), 0});
    }
    std::uint32_t id = group == GameDataAttributeGroup::conditions ? 4 : 16;
    std::uint32_t source = group == GameDataAttributeGroup::conditions
                               ? 0x8ee710 : 0x8ee640;
    for (std::size_t i = 0; i < 13; ++i, id += 4, source += 16)
        append({id, source, GameDataPropertyKind::text,
                runtime.SourceName(source) + runtime.ErrorSuffix(), 0});
    runtime.Sort(group, state.records);
    return CollectionAddress(group);
}

std::uint32_t GameDataAttributeRegistries::InitializeConditions(
    std::uint8_t first_seed, std::uint8_t second_seed,
    IGameDataAttributeRegistriesRuntime& runtime) {
    return Initialize(conditions_, GameDataAttributeGroup::conditions,
                      first_seed, second_seed, runtime);
}

void GameDataAttributeRegistries::CleanupConditions(
    IGameDataAttributeRegistriesRuntime& runtime) {
    runtime.CleanupStorage(GameDataAttributeGroup::conditions);
    conditions_.records.clear();
}

std::uint8_t GameDataAttributeRegistries::ProcessConditions(
    std::uint32_t object, std::uint8_t first_seed,
    std::uint8_t second_seed, IGameDataAttributeRegistriesRuntime& runtime) {
    const auto collection = InitializeConditions(first_seed, second_seed, runtime);
    return static_cast<std::uint8_t>(runtime.ProcessCollection(object, collection));
}

std::uint32_t GameDataAttributeRegistries::InitializeParameters(
    std::uint8_t first_seed, std::uint8_t second_seed,
    IGameDataAttributeRegistriesRuntime& runtime) {
    return Initialize(parameters_, GameDataAttributeGroup::parameters,
                      first_seed, second_seed, runtime);
}

void GameDataAttributeRegistries::CleanupParameters(
    IGameDataAttributeRegistriesRuntime& runtime) {
    runtime.CleanupStorage(GameDataAttributeGroup::parameters);
    parameters_.records.clear();
}

std::uint8_t GameDataAttributeRegistries::ProcessParameters(
    std::uint32_t object, std::uint8_t first_seed,
    std::uint8_t second_seed, IGameDataAttributeRegistriesRuntime& runtime) {
    const auto collection = InitializeParameters(first_seed, second_seed, runtime);
    return static_cast<std::uint8_t>(runtime.ProcessCollection(object, collection));
}

}  // namespace pal4
