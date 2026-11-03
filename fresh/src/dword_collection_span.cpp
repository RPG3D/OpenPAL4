#include "pal4/dword_collection_span.h"

#include <bit>

namespace pal4 {

std::int32_t DwordCollectionSpan::Size(
    const DwordCollectionSpanState& state) noexcept {
    if (state.begin == 0u) return 0;
    const auto bytes = std::bit_cast<std::int32_t>(state.end - state.begin);
    return bytes >> 2;
}

std::uint32_t DwordCollectionSpan::CopyTo(
    DwordCollectionSpanState& state, std::uint32_t destination,
    std::uint32_t source, IDwordCollectionSpanRuntime& runtime) {
    const auto original_destination = destination;
    const auto source_end = state.end;
    while (source != source_end) {
        runtime.WriteWord(destination, runtime.ReadWord(source));
        source += 4u;
        destination += 4u;
    }
    state.end = destination;
    return original_destination;
}

}  // namespace pal4
