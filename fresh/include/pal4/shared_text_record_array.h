#pragma once

#include <cstdint>

namespace pal4 {

class IArenaSequenceRuntime;
class IUtilitiesRecordArrayRuntime;

struct SharedTextRecordArrayState final {
    std::uint32_t prefix = 0u;
    std::uint32_t begin = 0u;
    std::uint32_t end = 0u;
    std::uint32_t capacity_end = 0u;
};
static_assert(sizeof(SharedTextRecordArrayState) == 16u);

class SharedTextRecordArray final {
public:
    // IDA 0x62ab10: release each 20-byte record and clear the storage span.
    [[nodiscard]] static int Clear(SharedTextRecordArrayState& state,
                                   IArenaSequenceRuntime& runtime);
    // IDA 0x62ba30: insert repeated copies of one 20-byte text record.
    static void InsertCopies(std::uint32_t array, std::uint32_t position,
                             std::uint32_t count, std::uint32_t source,
                             IUtilitiesRecordArrayRuntime& runtime);
};

}  // namespace pal4
