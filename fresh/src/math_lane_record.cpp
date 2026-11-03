#include "pal4/math_lane_record.h"

namespace pal4 {

MathLaneRecord& MathLaneRecordWriter::Fill(
    MathLaneRecord& record, const float value,
    const std::uint32_t lane1, const std::uint32_t lane2,
    const std::uint32_t lane3) {
    record.lane1 = lane1;
    record.lane2 = lane2;
    record.value = value;
    record.lane3 = lane3;
    return record;
}

}  // namespace pal4
