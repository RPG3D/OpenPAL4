#pragma once
#include "pal4/native_game_database.h"
#include <span>

namespace pal4 {
struct NativeFloorCombatArea {
    std::uint32_t id;
    std::string world, area;
    std::int32_t floor;
};
// Host-owned CombatScene table snapshot, resolved with41F9C0's floor policy.
// An empty result means the original lookup did not match;434E30 owns x fallback.
class NativeFloorCombatAreas final {
public:
    explicit NativeFloorCombatAreas(const NativeGameDatabase& database);
    explicit NativeFloorCombatAreas(const NativeGameDbTable& table);
    std::string Resolve(std::string_view world, std::int32_t current,
        std::int32_t previous) const;
    std::span<const NativeFloorCombatArea> Records() const { return records_; }
private:
    std::vector<NativeFloorCombatArea> records_;
};
}
