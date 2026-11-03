#include "pal4/gom_task_derived_properties.h"

namespace pal4 {

std::uint8_t GomTaskDerivedProperties::GetFlag60(
    const std::span<const std::uint32_t> words) {
    return static_cast<std::uint8_t>(words[15u]);
}

std::uint8_t GomTaskDerivedProperties::GetFlag61(
    const std::span<const std::uint32_t> words) {
    return static_cast<std::uint8_t>(words[15u] >> 8u);
}

std::uint32_t GomTaskDerivedProperties::SetWord68(
    const std::span<std::uint32_t> words, const std::uint32_t value) {
    words[17u] = value;
    return value;
}

bool GomTaskDerivedProperties::IsNonZero(const std::uint32_t value) {
    return value != 0u;
}

}  // namespace pal4
