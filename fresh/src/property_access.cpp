#include "pal4/property_access.h"

#include <algorithm>
#include <cstring>

namespace pal4 {

std::int32_t PropertyAccess::ProcessByName(
    const PropertyCollectionState& collection, const std::uint32_t target,
    const char* const name, IPropertyAccessRuntime& runtime) {
    for (std::int32_t index = 0; index < collection.count; ++index) {
        const auto entry = collection.entries +
            static_cast<std::uint32_t>(index) * 16u;
        const auto name_length = std::strlen(name);
        const auto length = runtime.ReadDword(entry + 8u);
        const auto data = runtime.ReadDword(entry + 4u);
        const auto compared = std::min<std::uint32_t>(
            length, static_cast<std::uint32_t>(name_length));
        bool same = true;
        for (std::uint32_t i = 0; i < compared; ++i) {
            if (runtime.ReadByte(data + i) !=
                static_cast<std::uint8_t>(name[i])) {
                same = false;
                break;
            }
        }
        if (same && length == name_length) {
            return runtime.DispatchByIndex(collection, target, index, 0x14u);
        }
    }
    return collection.count;
}

std::uint32_t PropertyAccess::SetFloat(
    const PropertyScalarState& property, const std::uint32_t target,
    const float value, IPropertyAccessRuntime& runtime) {
    runtime.WriteFloat(target + property.value_offset, value);
    return property.value_offset;
}

double PropertyAccess::GetFloat(
    const PropertyScalarState& property, const std::uint32_t target,
    IPropertyAccessRuntime& runtime) {
    return runtime.ReadFloat(target + property.value_offset);
}

std::uint32_t PropertyAccess::SetInt(
    const PropertyScalarState& property, const std::uint32_t target,
    const std::uint32_t value, IPropertyAccessRuntime& runtime) {
    runtime.WriteDword(target + property.value_offset, value);
    return property.value_offset;
}

std::uint32_t PropertyAccess::GetInt(
    const PropertyScalarState& property, const std::uint32_t target,
    IPropertyAccessRuntime& runtime) {
    return runtime.ReadDword(target + property.value_offset);
}

std::int32_t PropertyAccess::SetIndex(
    const PropertyScalarState& property, const std::uint32_t target,
    const std::int32_t index, IPropertyAccessRuntime& runtime) {
    if (index >= 0 && index < property.index_count) {
        runtime.WriteDword(target + property.index_offset,
                           static_cast<std::uint32_t>(index));
    }
    return index;
}

std::uint32_t PropertyAccess::GetIndex(
    const PropertyScalarState& property, const std::uint32_t target,
    IPropertyAccessRuntime& runtime) {
    return runtime.ReadDword(target + property.index_offset);
}

}  // namespace pal4
