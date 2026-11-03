#include "pal4/property_string_pair.h"

#include <algorithm>

namespace pal4 {
namespace {

bool EqualField(std::uint32_t left_address, std::uint32_t left_length,
                std::uint32_t right_address, std::uint32_t right_length,
                IPropertyStringPairRuntime& runtime) {
    const auto effective_left = left_address != 0u
        ? left_address : PropertyStringPair::kEmptyStringAddress;
    if (runtime.CompareBytes(right_address, effective_left,
                             std::min(left_length, right_length)) != 0) {
        return false;
    }
    return left_length == right_length;
}

std::uint32_t Release(std::uint32_t text,
                      IPropertyStringPairRuntime& runtime) {
    if (text == 0u) return 0u;
    const auto header = text - 1u;
    const auto count = runtime.ReadByte(header);
    if (count == 0u || count == 0xffu) return runtime.Free(header);
    runtime.WriteByte(header, static_cast<std::uint8_t>(count - 1u));
    return header;
}

}  // namespace

bool PropertyStringPair::Compare(const Storage& left, const Storage& right,
                                 IPropertyStringPairRuntime& runtime) {
    return EqualField(left[1], left[2], right[2], right[3], runtime) &&
           EqualField(left[5], left[6], right[6], right[7], runtime);
}

std::uint32_t PropertyStringPair::ItemProperty(
    const Storage& object) noexcept {
    return object[5];
}

std::uint32_t PropertyStringPair::Cleanup(
    Storage& object, IPropertyStringPairRuntime& runtime) {
    (void)Release(object[5], runtime);
    object[5] = object[6] = object[7] = 0u;
    const auto result = Release(object[1], runtime);
    object[1] = object[2] = object[3] = 0u;
    return result;
}

}  // namespace pal4
