#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct BoneRecordState final {
    std::array<std::uint32_t, 4> words{};
};

static_assert(sizeof(BoneRecordState) == 16);

class BoneRecord final {
public:
    // IDA 0x5fc0a0.
    [[nodiscard]] static BoneRecordState& Initialize(BoneRecordState& state);
};

}  // namespace pal4
