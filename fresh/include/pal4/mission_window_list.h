#pragma once

#include "pal4/mission_window_lifecycle.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

struct MissionRecord {
    std::uint32_t id{};
    std::int32_t availability{};
    std::string title;
};

using MissionRecordSlots = std::vector<std::optional<MissionRecord>>;

class IMissionWindowListRuntime {
public:
    virtual ~IMissionWindowListRuntime() = default;
    virtual void SetImage(std::uint32_t image, std::string_view image_set,
                          std::string_view image_name) = 0;
    virtual void SetSelected(std::uint32_t button, bool selected) = 0;
    [[nodiscard]] virtual std::uint32_t ColumnCount(std::uint32_t list) = 0;
    virtual void RemoveColumn(std::uint32_t list, std::uint32_t index) = 0;
    virtual void ResetList(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::string TriggerTitle(std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    virtual void AddColumn(std::uint32_t list, std::string_view title,
                           std::uint32_t id, float width) = 0;
    [[nodiscard]] virtual MissionRecordSlots LoadMissionRecords() = 0;
    virtual void ReleaseMissionRecords() = 0;
    virtual void AddRow(std::uint32_t list, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t CreateItem(
        std::uint32_t list, std::string_view title,
        std::uint32_t bytes, std::uint32_t vtable) = 0;
    virtual void WriteItemId(std::uint32_t item, std::uint32_t id) = 0;
    virtual void WriteItemEnabled(std::uint32_t item, bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(std::uint32_t list) = 0;
    virtual void SetItem(std::uint32_t list, std::uint32_t item,
                         std::uint32_t column, std::uint32_t row) = 0;
    virtual void SetHoverRow(std::uint32_t list, std::int32_t row) = 0;
    virtual void SetMissionPanelVisibility(MissionWindowState& state,
                                           bool visible) = 0;
};

class MissionWindowList final {
public:
    static constexpr std::uint32_t kItemBytes = 0x210u;
    static constexpr std::uint32_t kItemVtable = 0x8438d0u;

    static void Populate(MissionWindowState& state,
                         IMissionWindowListRuntime& runtime);
};

}  // namespace pal4
