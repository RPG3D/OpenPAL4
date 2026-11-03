#pragma once

#include "pal4/indexed_property_grid.h"
#include "pal4/ui_property_element.h"

#include <array>
#include <cstdint>
#include <vector>

namespace pal4 {

class UiPropertyGrid;

class UiPlacementRule final {
public:
    static constexpr std::uint32_t kVtable = 0x8493fc;

    // IDA 0x5acf70.
    UiPlacementRule() = default;
    void Configure(std::uint32_t property,
                   const std::array<std::int32_t, 2>& first,
                   const std::array<std::int32_t, 2>& second);

    // IDA 0x5acf50: vtable thunk shared by the basic and sequence rules.
    [[nodiscard]] static bool BasicCanOccupy(
        const std::array<std::int32_t, 2>& origin,
        std::uint32_t element, const IndexedPropertyGrid& grid,
        IUiPropertyElementRuntime& runtime);

    // IDA 0x5acf90/0x5ad000: slots +4 and +0 of the same native vtable.
    [[nodiscard]] bool MatchesGridProperties(
        const UiPropertyGrid& grid,
        IUiPropertyElementRuntime& runtime) const;
    [[nodiscard]] bool CanOccupyOrOverlap(
        const std::array<std::int32_t, 2>& origin,
        std::uint32_t element,
        const UiPropertyGrid& grid,
        IUiPropertyElementRuntime& runtime) const;

    [[nodiscard]] std::uint32_t Property() const { return property_; }
    [[nodiscard]] std::array<std::int32_t, 2> First() const {
        return first_;
    }
    [[nodiscard]] std::array<std::int32_t, 2> Second() const {
        return second_;
    }

private:
    std::uint32_t property_ = 0;
    std::array<std::int32_t, 2> first_{};
    std::array<std::int32_t, 2> second_{};
};

struct UiPlacementImageLink {
    std::uint32_t property = 0;
    std::int32_t x = 0;
    std::int32_t y = 0;
};

class UiPlacementSequenceRule final {
public:
    static constexpr std::uint32_t kVtable = 0x849408;

    void AddLink(const UiPlacementImageLink& link);
    [[nodiscard]] const std::vector<UiPlacementImageLink>& Links() const {
        return links_;
    }
    // IDA 0x5ad0c0: an empty sequence matches.
    [[nodiscard]] bool MatchesImageLinks(
        const UiPropertyGrid& grid,
        IUiPropertyElementRuntime& runtime) const;

private:
    std::vector<UiPlacementImageLink> links_;
};

}  // namespace pal4
