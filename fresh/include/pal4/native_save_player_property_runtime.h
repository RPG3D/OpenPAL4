#pragma once

#include "pal4/player_manager_object.h"

#include <cstdint>

namespace pal4 {
struct GameObjectPropertyState;

// Save-point recovery uses maxima; archive summaries use current properties.
class NativeSavePlayerPropertyRuntime final {
public:
    explicit NativeSavePlayerPropertyRuntime(PlayerManagerObjectState& players)
        : players_(players) {}

    [[nodiscard]] std::uint32_t GetMaximum(std::uint32_t component,
                                         std::uint32_t property) const;
    [[nodiscard]] std::uint32_t GetCurrent(std::uint32_t component,
                                         std::uint32_t property) const;
    void Set(std::uint32_t component, std::uint32_t property,
             std::uint32_t value);

private:
    [[nodiscard]] std::uint32_t SlotIndex(std::uint32_t component) const;
    [[nodiscard]] GameObjectPropertyState ReadState(std::uint32_t component) const;

    PlayerManagerObjectState& players_;
};

}  // namespace pal4
