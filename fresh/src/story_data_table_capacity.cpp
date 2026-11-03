#include "pal4/story_data_table.h"
#include "pal4/story_desc_data_table.h"
#include "data_table_capacity_common.h"

namespace pal4 {

std::uint32_t StoryDataTable::Reserve(
    std::uint32_t table, std::int32_t requested,
    IStoryDataCapacityRuntime& runtime) {
    return detail::ReserveDataTable<36>(table, requested, runtime);
}

std::uint32_t StoryDescDataTable::Reserve(
    std::uint32_t table, std::int32_t requested,
    IStoryDataCapacityRuntime& runtime) {
    return StoryDataTable::Reserve(table, requested, runtime);
}

}  // namespace pal4
