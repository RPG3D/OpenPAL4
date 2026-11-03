#include "pal4/game_question_data_table.h"

#include <initializer_list>

namespace pal4 {

std::uint32_t GameQuestionDataTable::ReleaseRecordArray(
    std::uint32_t array, IGameQuestionDataStorageRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    for (auto record = begin; record != end; record += 76)
        static_cast<void>(runtime.DestroyQuestionRecord(record));
    runtime.Free(begin);
    for (const auto offset : {4u, 8u, 12u})
        runtime.WriteWord(array + offset, 0);
    return 0;
}

std::uint32_t GameQuestionDataTable::ClearRows(
    std::uint32_t table, IGameQuestionDataStorageRuntime& runtime) {
    auto result = runtime.ClearEffectData(table);
    const auto begin = runtime.ReadWord(table + 96);
    const auto end = runtime.ReadWord(table + 100);
    for (auto record = begin; record != end; record += 76)
        result = runtime.DestroyQuestionRecord(record);
    runtime.WriteWord(table + 100, begin);
    return result;
}

}  // namespace pal4
