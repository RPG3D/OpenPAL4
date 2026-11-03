#pragma once

#include "pal4/mission_window_hover_lookup.h"
#include "pal4/mission_window_quest_details.h"

#include <cstdint>

namespace pal4 {

class IMissionWindowDetailsPanelRuntime :
    public IMissionWindowHoverLookupRuntime,
    public IMissionWindowQuestDetailsRuntime {
public:
    [[nodiscard]] virtual std::uint32_t RecordId(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::int32_t RowCount(
        std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t GridItem(
        std::uint32_t list, std::uint32_t row,
        std::uint32_t column) = 0;
    virtual void SetHoverRow(std::uint32_t list,
                             std::uint32_t row) = 0;
    virtual void ScrollRowToTop(std::uint32_t list,
                                std::uint32_t row) = 0;
};

class MissionWindowDetailsPanel final {
public:
    static void Update(const MissionWindowState& state,
                       IMissionWindowDetailsPanelRuntime& runtime);
};

}  // namespace pal4
