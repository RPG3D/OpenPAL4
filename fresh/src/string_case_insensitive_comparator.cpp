#include "pal4/string_case_insensitive_comparator.h"

namespace pal4 {
namespace {

struct TemporaryBuffer {
    StringBuffer value;
    IStringBufferRuntime& runtime;

    ~TemporaryBuffer() { static_cast<void>(value.Cleanup(runtime)); }
};

}  // namespace

std::int32_t StringCaseInsensitiveComparator::Compare(
    std::uint32_t left, std::uint32_t right,
    IStringBufferRuntime& runtime) {
    TemporaryBuffer first{{}, runtime};
    static_cast<void>(first.value.InitializeFromCString(left, runtime));
    TemporaryBuffer second{{}, runtime};
    static_cast<void>(second.value.InitializeFromCString(right, runtime));
    return first.value.CompareCaseInsensitive(second.value, runtime);
}

std::int32_t StringCaseInsensitiveComparator::CompareManagerKey(
    const std::uint32_t left, const std::uint32_t right,
    IStringBufferRuntime& runtime) {
    return Compare(left, right, runtime);
}

}  // namespace pal4
