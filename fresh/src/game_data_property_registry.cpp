#include "pal4/game_data_property_registry.h"

#include <string_view>
#include <utility>

namespace pal4 {
namespace {

void AppendSource(std::uint32_t offset, std::uint32_t source,
                  std::string_view suffix, GameDataFieldKind kind,
                  std::uint32_t vtable,
                  std::vector<GameDataFieldDescriptor>& fields,
                  IGameDataPropertyRegistryRuntime& runtime) {
    runtime.AppendField(fields,
                        {offset, runtime.SourceName(source) +
                                     std::string(suffix),
                         kind, vtable, 24, source, 0});
}

}  // namespace

const std::vector<GameDataFieldDescriptor>& GameDataPropertyRegistry::Initialize(
    IGameDataPropertyRegistryRuntime& runtime) {
    if (!globals_initialized_) {
        globals_initialized_ = true;
        runtime.InitializeGlobalStorage();
        runtime.RegisterGlobalCleanup();
    }
    if (entries_initialized_) return fields_;
    entries_initialized_ = true;

    const auto empty = runtime.ErrorSuffix();
    for (const auto& [offset, name] : {
             std::pair<std::uint32_t, std::string_view>{0, "ID"},
             {4, "TYPE"}, {8, "AI_CMD_TYPE"}})
        runtime.AppendField(fields_,
                            {offset, std::string(name) + empty,
                             GameDataFieldKind::numeric, 0x843368, 24, 0, 0});

    for (std::uint32_t i = 0; i < 7; ++i)
        AppendSource(16 + i * 16, 0x8ef080 + i * 16, empty,
                     GameDataFieldKind::text, 0x844574,
                     fields_, runtime);
    for (std::uint32_t i = 0; i < 5; ++i)
        AppendSource(144 + i * 4, 0x8ef030 + i * 16, empty,
                     GameDataFieldKind::numeric, 0x843368,
                     fields_, runtime);
    for (std::uint32_t i = 0; i < 4; ++i)
        AppendSource(128 + i * 4, 0x8ef9c8 + i * 16, "_CAN_USE",
                     GameDataFieldKind::numeric, 0x843368,
                     fields_, runtime);

    runtime.AppendField(fields_,
                        {12, "TARGET" + empty, GameDataFieldKind::numeric,
                         0x843368, 24, 0, 0});
    runtime.AppendField(fields_,
                        {164, "PA_ATK_DELAY" + empty,
                         GameDataFieldKind::hit_extra,
                         0x8444dc, 24, 0, 0});
    runtime.Sort(fields_);
    return fields_;
}

void GameDataPropertyRegistry::Cleanup(
    IGameDataPropertyRegistryRuntime& runtime) {
    runtime.CleanupGlobalStorage();
    fields_.clear();
}

std::uint8_t GameDataPropertyRegistry::Process(
    std::uint32_t collection,
    IGameDataPropertyRegistryRuntime& runtime) {
    return runtime.ProcessCollection(collection, Initialize(runtime));
}

}  // namespace pal4
