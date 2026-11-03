#include "pal4/ui_placement_rule.h"

#include "pal4/ui_property_grid.h"

#include <cstdlib>
#include <limits>

namespace pal4 {

void UiPlacementRule::Configure(
    const std::uint32_t property,
    const std::array<std::int32_t, 2>& first,
    const std::array<std::int32_t, 2>& second) {
    property_ = property;
    first_ = first;
    second_ = second;
}

bool UiPlacementRule::BasicCanOccupy(
    const std::array<std::int32_t, 2>& origin,
    const std::uint32_t element,
    const IndexedPropertyGrid& grid,
    IUiPropertyElementRuntime& runtime) {
    return UiPropertyElement::CanOccupy(element, origin, grid, runtime);
}

bool UiPlacementRule::MatchesGridProperties(
    const UiPropertyGrid& grid,
    IUiPropertyElementRuntime& runtime) const {
    const auto element = grid.FindElement(property_, runtime);
    if (element == 0) return false;
    const auto property = static_cast<std::int32_t>(
        runtime.ReadWord(element + 192u));
    const auto first = grid.Grid().Get(first_[0], first_[1]);
    const auto second = grid.Grid().Get(second_[0], second_[1]);
    return first == property || second == property;
}

bool UiPlacementRule::CanOccupyOrOverlap(
    const std::array<std::int32_t, 2>& origin,
    const std::uint32_t element,
    const UiPropertyGrid& grid,
    IUiPropertyElementRuntime& runtime) const {
    if (UiPropertyElement::CanOccupy(element, origin, grid.Grid(), runtime)) {
        return true;
    }
    const auto current_property = runtime.ReadWord(element + 192u);
    if (current_property != property_) return false;
    const auto old_x = static_cast<std::int32_t>(runtime.ReadWord(element));
    const auto old_y = static_cast<std::int32_t>(runtime.ReadWord(element + 4u));
    const auto distance =
        std::llabs(static_cast<long long>(origin[0]) - old_x) +
        std::llabs(static_cast<long long>(origin[1]) - old_y);
    if (distance > 1) return false;
    const auto width = static_cast<std::int32_t>(
        runtime.ReadWord(element + 8u));
    const auto height = static_cast<std::int32_t>(
        runtime.ReadWord(element + 12u));
    for (std::int64_t x = origin[0];
         x < static_cast<std::int64_t>(origin[0]) + width; ++x) {
        for (std::int64_t y = origin[1];
             y < static_cast<std::int64_t>(origin[1]) + height; ++y) {
            if (x < std::numeric_limits<std::int32_t>::min() ||
                x > std::numeric_limits<std::int32_t>::max() ||
                y < std::numeric_limits<std::int32_t>::min() ||
                y > std::numeric_limits<std::int32_t>::max()) return false;
            const auto value = grid.Grid().Get(
                static_cast<std::int32_t>(x), static_cast<std::int32_t>(y));
            if (value != 0 &&
                value != static_cast<std::int32_t>(current_property) &&
                value != -1) return false;
        }
    }
    return true;
}

void UiPlacementSequenceRule::AddLink(const UiPlacementImageLink& link) {
    links_.push_back(link);
}

bool UiPlacementSequenceRule::MatchesImageLinks(
    const UiPropertyGrid& grid,
    IUiPropertyElementRuntime& runtime) const {
    for (const auto& link : links_) {
        const auto element = grid.FindElement(link.property, runtime);
        if (element == 0) return false;
        const auto linked_property = grid.Grid().Get(link.x, link.y);
        const auto linked = grid.FindElement(
            static_cast<std::uint32_t>(linked_property), runtime);
        if (linked == 0 ||
            runtime.ReadWord(linked + 168u) !=
                runtime.ReadWord(element + 168u)) return false;
    }
    return true;
}

}  // namespace pal4
