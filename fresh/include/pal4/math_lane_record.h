#pragma once

#include <cstdint>

namespace pal4 {

struct MathLaneRecord final {
    float value = 0.0f;
    std::uint32_t lane1 = 0u;
    std::uint32_t lane2 = 0u;
    std::uint32_t lane3 = 0u;
};

static_assert(sizeof(MathLaneRecord) == 16);

class MathLaneRecordWriter final {
public:
    // IDA 0x6019e0.
    [[nodiscard]] static MathLaneRecord& Fill(
        MathLaneRecord& record, float value,
        std::uint32_t lane1, std::uint32_t lane2,
        std::uint32_t lane3);
};

}  // namespace pal4
