#include "pal4/story_data_table.h"
#include "pal4/story_desc_data_table.h"
#include "pal4/game_question_data_table.h"

#include <initializer_list>

namespace pal4 {
namespace {

std::uint32_t CreateTable(
    std::uint32_t name, std::uint32_t vtable,
    IStoryDataTableRuntime& runtime) {
    const auto table = runtime.Allocate(140);
    if (!table) return 0;
    runtime.ConstructBase(table, name);
    for (const auto offset : {92u, 108u, 124u}) {
        runtime.WriteByte(table + offset,
                          static_cast<std::uint8_t>(name));
        for (std::uint32_t word = 4; word <= 12; word += 4)
            runtime.WriteWord(table + offset + word, 0);
    }
    runtime.WriteWord(table, vtable);
    return table;
}

std::uint32_t DestroyTable(
    std::uint32_t table, std::uint32_t vtable,
    IStoryDataTableDestroyRuntime& runtime) {
    runtime.WriteWord(table, vtable);
    runtime.ClearEffectData(table);

    const auto rows = runtime.ReadWord(table + 96);
    const auto row_end = runtime.ReadWord(table + 100);
    for (auto record = rows; record != row_end; record += 36) {
        runtime.DestroyStoryText(record + 20);
        runtime.DestroyStoryText(record + 4);
    }
    runtime.WriteWord(table + 100, rows);

    runtime.ProcessElements(table + 108);
    runtime.Free(runtime.ReadWord(table + 128));
    for (const auto offset : {128u, 132u, 136u})
        runtime.WriteWord(table + offset, 0);

    const auto properties = runtime.ReadWord(table + 112);
    const auto property_end = runtime.ReadWord(table + 116);
    for (auto record = properties; record != property_end; record += 60)
        runtime.DestroyPropertyRecord(record);
    runtime.Free(properties);
    for (const auto offset : {112u, 116u, 120u})
        runtime.WriteWord(table + offset, 0);

    const auto final_row_end = runtime.ReadWord(table + 100);
    for (auto record = runtime.ReadWord(table + 96);
         record != final_row_end; record += 36)
        runtime.DestroyStoryRecord(record);
    runtime.Free(runtime.ReadWord(table + 96));
    for (const auto offset : {96u, 100u, 104u})
        runtime.WriteWord(table + offset, 0);
    return runtime.DestroyBase(table);
}

}  // namespace

std::uint32_t StoryDataTable::Create(
    std::uint32_t name, IStoryDataTableRuntime& runtime) {
    return CreateTable(name, 0x84474c, runtime);
}

std::uint32_t StoryDataTable::Destroy(
    std::uint32_t table, IStoryDataTableDestroyRuntime& runtime) {
    return DestroyTable(table, 0x84474c, runtime);
}

std::uint32_t StoryDescDataTable::Create(
    std::uint32_t name, IStoryDataTableRuntime& runtime) {
    return CreateTable(name, 0x84476c, runtime);
}

std::uint32_t StoryDescDataTable::Destroy(
    std::uint32_t table, IStoryDataTableDestroyRuntime& runtime) {
    return DestroyTable(table, 0x84476c, runtime);
}

std::uint32_t GameQuestionDataTable::Create(
    std::uint32_t name, IStoryDataTableRuntime& runtime) {
    return CreateTable(name, 0x84478c, runtime);
}

}  // namespace pal4
