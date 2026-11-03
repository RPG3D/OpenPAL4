#include "pal4/game_question_data_table.h"

#include <initializer_list>

namespace pal4 {

std::uint32_t GameQuestionDataTable::Destroy(
    std::uint32_t table,
    IGameQuestionDataTableDestroyRuntime& runtime) {
    runtime.WriteWord(table, 0x84478c);
    runtime.ClearEffectData(table);

    const auto rows = runtime.ReadWord(table + 96);
    const auto row_end = runtime.ReadWord(table + 100);
    for (auto record = rows; record != row_end; record += 76)
        runtime.DestroyQuestionRecord(record);
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
         record != final_row_end; record += 76)
        runtime.DestroyQuestionRecord(record);
    runtime.Free(runtime.ReadWord(table + 96));
    for (const auto offset : {96u, 100u, 104u})
        runtime.WriteWord(table + offset, 0);
    return runtime.DestroyBase(table);
}

}  // namespace pal4
