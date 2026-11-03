#include "pal4/story_data_table.h"
#include "data_table_index_common.h"

namespace pal4 {
void StoryDataTable::RebuildIndex(
    std::uint32_t table, IStoryDataIndexRuntime& runtime) {
    detail::RebuildDataTableIndex<36>(table, runtime);
}

std::uint32_t StoryDataTable::DestroyRange(
    std::uint32_t begin, std::uint32_t end,
    IStoryDataIndexRuntime& runtime) {
    return detail::DestroyDataTableRange<36>(begin, end, runtime);
}

}  // namespace pal4
