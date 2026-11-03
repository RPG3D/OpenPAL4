#include "pal4/shared_text_record_array.h"

#include "pal4/arena_sequence_resources.h"
#include "pal4/utilities_text_record_table.h"

namespace pal4 {

int SharedTextRecordArray::Clear(SharedTextRecordArrayState& state,
                                 IArenaSequenceRuntime& runtime) {
    for (std::uint32_t record = state.begin; record != state.end;
         record += 20u) {
        ArenaSequenceResources::ReleaseText(record, runtime);
    }
    (void)runtime.Free(state.begin);
    state.begin = 0u;
    state.end = 0u;
    state.capacity_end = 0u;
    return 0;
}

void SharedTextRecordArray::InsertCopies(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    IUtilitiesRecordArrayRuntime& runtime) {
    UtilitiesTextRecordArray::InsertCopies(
        array, position, count, source, runtime);
}

}  // namespace pal4
