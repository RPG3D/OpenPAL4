#include "pal4/game_data_parameter_registry.h"

#include <string_view>
#include <utility>

namespace pal4 {
namespace {

void AppendSource(std::uint32_t offset, std::uint32_t source,
                  std::string_view suffix, GameDataFieldKind kind,
                  std::uint32_t vtable,
                  std::vector<GameDataFieldDescriptor>& fields,
                  IGameDataParameterRegistryRuntime& runtime) {
    runtime.AppendField(fields,
                        {offset, runtime.SourceName(source) +
                                     std::string(suffix),
                         kind, vtable, 24, source, 0});
}

void AppendGroup(std::uint32_t first_offset, std::uint32_t count,
                 std::uint32_t source_base, std::uint32_t source_step,
                 std::string_view suffix, GameDataFieldKind kind,
                 std::uint32_t vtable,
                 std::vector<GameDataFieldDescriptor>& fields,
                 IGameDataParameterRegistryRuntime& runtime,
                 std::uint32_t destination_step = 4u) {
    for (std::uint32_t i = 0; i < count; ++i)
        AppendSource(first_offset + i * destination_step, source_base + i * source_step,
                     suffix, kind, vtable, fields, runtime);
}

}  // namespace

const std::vector<GameDataFieldDescriptor>& GameDataParameterRegistry::Initialize(
    IGameDataParameterRegistryRuntime& runtime) {
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
             {100, "TYPE"}, {324, "CALC_TYPE"}})
        runtime.AppendField(fields_,
                            {offset, std::string(name) + empty,
                             GameDataFieldKind::numeric, 0x843368, 24, 0, 0});

    AppendGroup(4, 6, 0x8eef90, 16, empty, GameDataFieldKind::text,
                0x844574, fields_, runtime, 16u);
    AppendGroup(124, 7, 0x8ef958, 16, empty, GameDataFieldKind::numeric,
                0x843368, fields_, runtime);
    AppendGroup(152, 4, 0x8eede0, 16, empty, GameDataFieldKind::numeric,
                0x843368, fields_, runtime);
    AppendGroup(168, 4, 0x8eede0, 16, "_NUMBER", GameDataFieldKind::numeric,
                0x843368, fields_, runtime);
    AppendGroup(184, 8, 0x8eeec0, 16, empty, GameDataFieldKind::numeric,
                0x843368, fields_, runtime);
    AppendGroup(216, 5, 0x8efa08, 16, empty, GameDataFieldKind::numeric,
                0x843368, fields_, runtime);
    AppendGroup(236, 6, 0x8eee20, 16, "_ADD", GameDataFieldKind::numeric,
                0x843368, fields_, runtime);
    AppendGroup(260, 6, 0x8eee20, 16, "_EXTRACT",
                GameDataFieldKind::hit_extra, 0x8444dc, fields_, runtime);
    AppendGroup(284, 6, 0x8eee20, 16, "_REACT",
                GameDataFieldKind::hit_extra, 0x8444dc, fields_, runtime);
    AppendGroup(308, 4, 0x8eee80, 16, empty,
                GameDataFieldKind::hit_extra, 0x8444dc, fields_, runtime);
    AppendGroup(104, 5, 0x8eef40, 16, empty, GameDataFieldKind::numeric,
                0x843368, fields_, runtime);
    runtime.Sort(fields_);
    return fields_;
}

void GameDataParameterRegistry::Cleanup(
    IGameDataParameterRegistryRuntime& runtime) {
    runtime.CleanupGlobalStorage();
    fields_.clear();
}

std::uint8_t GameDataParameterRegistry::Process(
    std::uint32_t collection, IGameDataParameterRegistryRuntime& runtime) {
    return runtime.ProcessCollection(collection, Initialize(runtime));
}

}  // namespace pal4
