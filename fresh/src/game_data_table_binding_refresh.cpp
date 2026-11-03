#include "pal4/skill_data_table.h"
#include "pal4/stunt_data_table.h"
#include "pal4/magic_data_table.h"
#include "pal4/story_data_table.h"
#include "pal4/story_desc_data_table.h"
#include "pal4/game_question_data_table.h"

namespace pal4 {
namespace {

std::uint32_t RefreshBindings(
    std::uint32_t table, IGameDataTableBindingRefreshRuntime& runtime) {
    const auto field_begin = runtime.ReadWord(table + 128);
    const auto field_count = field_begin
        ? static_cast<std::int32_t>(
              runtime.ReadWord(table + 132) - field_begin) / 4 : 0;
    const auto source_begin = runtime.ReadWord(table + 24);
    const auto source_count = source_begin
        ? static_cast<std::int32_t>(
              runtime.ReadWord(table + 28) - source_begin) / 60 : 0;
    if (field_count == source_count)
        return source_begin ? (source_count < 0 ? 1u : 0u) : table + 20;

    try {
        runtime.LogBind("beginBindField, table:", table + 52);
        runtime.AssignEffects(table + 108, table + 20);
        runtime.ProcessPropertyRegistry(table + 108);
        runtime.BindEffects(table + 108);
        runtime.LogBind("endBindField, table:", table + 52);
    } catch (...) {
        static_cast<void>(runtime.ReleaseLogBuffer());
        throw;
    }
    return runtime.ReleaseLogBuffer();
}

}  // namespace

std::uint32_t SkillDataTable::RefreshBindings(
    std::uint32_t table, IGameDataTableBindingRefreshRuntime& runtime) {
    return pal4::RefreshBindings(table, runtime);
}

std::uint32_t StuntDataTable::RefreshBindings(
    std::uint32_t table, IGameDataTableBindingRefreshRuntime& runtime) {
    return pal4::RefreshBindings(table, runtime);
}

std::uint32_t MagicDataTable::RefreshBindings(
    std::uint32_t table, IGameDataTableBindingRefreshRuntime& runtime) {
    return pal4::RefreshBindings(table, runtime);
}

std::uint32_t StoryDataTable::RefreshBindings(
    std::uint32_t table, IGameDataTableBindingRefreshRuntime& runtime) {
    return pal4::RefreshBindings(table, runtime);
}

std::uint32_t StoryDescDataTable::RefreshBindings(
    std::uint32_t table, IGameDataTableBindingRefreshRuntime& runtime) {
    return pal4::RefreshBindings(table, runtime);
}

std::uint32_t GameQuestionDataTable::RefreshBindings(
    std::uint32_t table, IGameDataTableBindingRefreshRuntime& runtime) {
    return pal4::RefreshBindings(table, runtime);
}

}  // namespace pal4
