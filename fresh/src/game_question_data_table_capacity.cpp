#include "pal4/game_question_data_table.h"
#include "data_table_capacity_common.h"

namespace pal4 {

std::uint32_t GameQuestionDataTable::Reserve(
    std::uint32_t table, std::int32_t requested,
    IGameQuestionDataCapacityRuntime& runtime) {
    return detail::ReserveDataTable<76>(table, requested, runtime);
}

}  // namespace pal4
