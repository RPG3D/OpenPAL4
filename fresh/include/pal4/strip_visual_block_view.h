#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace pal4 {

// IDA 63CC80/649520: a visual resource block is 136 bytes. Both the
// 228-byte DynamicStrip object and StripSystemEffect use these algorithms.
// This borrowed view must never extend into the adjacent resource block.
struct StripVisualBlockView final {
    static constexpr std::size_t kWordCount = 34u;
    std::span<std::uint32_t, kWordCount> words;
};

struct StripVisualBlockConstView final {
    std::span<const std::uint32_t, StripVisualBlockView::kWordCount> words;
};

}  // namespace pal4
