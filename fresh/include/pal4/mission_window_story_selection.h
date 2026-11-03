#pragma once

#include "pal4/mission_window_lifecycle.h"

#include <cstdint>

namespace pal4 {

class IMissionWindowStorySelectionRuntime {
public:
    virtual ~IMissionWindowStorySelectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FirstSelectedItem(
        std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t ItemId(
        std::uint32_t item, std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::int32_t RowCount(
        std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t GridItem(
        std::uint32_t list, std::uint32_t row,
        std::uint32_t column) = 0;
    [[nodiscard]] virtual float StoryScrollPosition(
        std::uint32_t list) = 0;
    virtual void SetHoverRow(std::uint32_t list,
                             std::uint32_t row) = 0;
    virtual void ScrollRowToVisible(std::uint32_t list,
                                    std::uint32_t row) = 0;
    virtual void SetStoryScrollPosition(std::uint32_t list,
                                        float position) = 0;
};

class MissionWindowStorySelection final {
public:
    static void HandleClick(const MissionWindowState& state,
                            std::uint32_t event,
                            IMissionWindowStorySelectionRuntime& runtime);
};

}  // namespace pal4
