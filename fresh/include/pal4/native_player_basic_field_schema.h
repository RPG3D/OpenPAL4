#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace pal4 {

struct NativePlayerBasicField {
    std::string name;
    std::uint32_t type{};
    std::uint32_t slot_offset{};
    // Zero for ordinary scalars; enum setters reject indices outside this range.
    std::uint32_t index_count{};
};

class NativePlayerBasicFieldSchema final {
public:
    // Uses the original race and structure property registries.
    [[nodiscard]] static std::vector<NativePlayerBasicField> Build();
};

}  // namespace pal4
