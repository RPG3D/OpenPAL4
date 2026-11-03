#include "pal4/game_question_data_table.h"
#include "data_table_index_common.h"

namespace pal4 {

void GameQuestionDataTable::RebuildIndex(
    std::uint32_t table, IGameQuestionDataIndexRuntime& runtime) {
    detail::RebuildDataTableIndex<76>(table, runtime);
}

std::uint32_t GameQuestionDataTable::DestroyRange(
    std::uint32_t begin, std::uint32_t end,
    IGameQuestionDataIndexRuntime& runtime) {
    return detail::DestroyDataTableRange<76>(begin, end, runtime);
}

}  // namespace pal4
