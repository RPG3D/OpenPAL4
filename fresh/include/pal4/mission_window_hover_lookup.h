#pragma once

#include "pal4/mission_window_lifecycle.h"

#include <cstdint>

namespace pal4 {

class IMissionWindowMissionLookupRuntime {
public:
    virtual ~IMissionWindowMissionLookupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t HoverItem(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t ItemId(
        std::uint32_t item, std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::uint32_t FindMission(
        std::uint32_t id) = 0;
};

class IMissionWindowHoverLookupRuntime : public IMissionWindowMissionLookupRuntime {
public:
    [[nodiscard]] virtual std::uint32_t FindChapter(
        std::uint32_t id) = 0;
};

class MissionWindowHoverLookup final {
public:
    [[nodiscard]] static std::uint32_t Mission(
        const MissionWindowState& state,
        IMissionWindowMissionLookupRuntime& runtime);
    [[nodiscard]] static std::uint32_t Chapter(
        const MissionWindowState& state,
        IMissionWindowHoverLookupRuntime& runtime);
};

}  // namespace pal4
