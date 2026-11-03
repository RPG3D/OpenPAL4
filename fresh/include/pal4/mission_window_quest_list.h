#pragma once

#include "pal4/mission_window_lifecycle.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

struct MissionQuestRecord {
    std::int32_t id{};
    std::string title;
    std::string description;
};

class IMissionWindowQuestListRuntime {
public:
    virtual ~IMissionWindowQuestListRuntime() = default;
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
    [[nodiscard]] virtual float AbsoluteColumnWidth(std::uint32_t list,
                                                    std::uint32_t index) = 0;
    [[nodiscard]] virtual std::int32_t StoryVisibilityLimit() = 0;
    [[nodiscard]] virtual std::vector<MissionQuestRecord> LoadQuestRecords() = 0;
    virtual void ReleaseQuestRecords() = 0;
    virtual void AddRow(std::uint32_t list, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t CreatePlainItem(
        std::uint32_t list, std::string_view title,
        std::uint32_t bytes, std::uint32_t vtable) = 0;
    [[nodiscard]] virtual std::uint32_t CreateRichItem(
        std::string_view text, float width,
        std::uint32_t bytes) = 0;
    virtual void WriteItemId(std::uint32_t item, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(std::uint32_t list) = 0;
    virtual void SetItem(std::uint32_t list, std::uint32_t item,
                         std::uint32_t column, std::uint32_t row) = 0;
    virtual void SetHoverRow(std::uint32_t list, std::uint32_t row) = 0;
    virtual void ScrollRowToVisible(std::uint32_t list,
                                    std::uint32_t row) = 0;
    virtual void SetMissionPanelVisibility(MissionWindowState& state,
                                           bool visible) = 0;
};

class MissionWindowQuestList final {
public:
    static constexpr std::uint32_t kPlainItemBytes = 0x210u;
    static constexpr std::uint32_t kRichItemBytes = 0x1b4u;
    static constexpr std::uint32_t kPlainItemVtable = 0x8438d0u;

    [[nodiscard]] static constexpr bool IsVisible(
        std::int32_t id, std::int32_t limit) noexcept {
        return id < limit;
    }

    static void Populate(MissionWindowState& state,
                         IMissionWindowQuestListRuntime& runtime);
};

}  // namespace pal4
