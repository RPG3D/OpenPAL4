#include "pal4/math_lane_record.h"

#include <bit>
#include <cstdlib>
#include <iostream>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    pal4::MathLaneRecord record{};
    const auto value = std::bit_cast<float>(0x7fc12345u);
    Check(&pal4::MathLaneRecordWriter::Fill(
              record, value, 0x11111111u, 0x22222222u,
              0x33333333u) == &record &&
              std::bit_cast<std::uint32_t>(record.value) == 0x7fc12345u &&
              record.lane1 == 0x11111111u &&
              record.lane2 == 0x22222222u &&
              record.lane3 == 0x33333333u,
          "fills exact 16-byte lane layout and returns target");
    Check(&pal4::MathLaneRecordWriter::Fill(
              record, -0.0f, 1u, 2u, 3u) == &record &&
              std::bit_cast<std::uint32_t>(record.value) == 0x80000000u &&
              record.lane1 == 1u && record.lane2 == 2u &&
              record.lane3 == 3u,
          "repeat fill overwrites all fields and preserves float bits");
}
