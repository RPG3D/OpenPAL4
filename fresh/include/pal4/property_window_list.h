#pragma once

#include "pal4/property_window_lifecycle.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

struct PropertyInventoryRecord {
    std::uint32_t address{};
    std::uint32_t id{};
    std::uint32_t count{};
    std::optional<std::string> name;
};

class IPropertyWindowListRuntime {
public:
    virtual ~IPropertyWindowListRuntime() = default;
    [[nodiscard]] virtual float ScrollPosition(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t FirstSelectedItem(
        std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t SelectedRow(
        std::uint32_t list, std::uint32_t item) = 0;
    virtual void InitializeScratch() = 0;
    virtual void CleanupScratch() = 0;
    virtual void SetItemPanel(PropertyWindowState& state, bool visible) = 0;
    virtual void ResetList(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::vector<std::uint32_t> RecentItemIds() = 0;
    virtual void ReleaseRecentItems() = 0;
    [[nodiscard]] virtual std::optional<PropertyInventoryRecord> RecentItem(
        std::uint32_t id) = 0;
    [[nodiscard]] virtual std::optional<PropertyInventoryRecord> FeaturedItem(
        std::uint32_t category) = 0;
    [[nodiscard]] virtual std::uint32_t CategoryItemCount(
        std::uint32_t category) = 0;
    [[nodiscard]] virtual std::optional<PropertyInventoryRecord> CategoryItem(
        std::uint32_t category, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    [[nodiscard]] virtual std::string FormatCount(
        std::int32_t count) = 0;
    virtual void AddRow(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t CreateItem(
        std::uint32_t list, std::string_view text,
        std::uint32_t bytes, std::uint32_t vtable) = 0;
    virtual void WriteItemId(std::uint32_t item, std::uint32_t id) = 0;
    virtual void SetItem(std::uint32_t list, std::uint32_t item,
                         std::uint32_t column, std::uint32_t row) = 0;
    virtual void SetImage(std::uint32_t window,
                          std::string_view imageset,
                          std::string_view image) = 0;
    [[nodiscard]] virtual std::uint32_t RowCount(std::uint32_t list) = 0;
    virtual void SetHoverRow(std::uint32_t list,
                             std::uint32_t row) = 0;
    virtual void SetScrollPosition(std::uint32_t list,
                                   float position) = 0;
    virtual void SetSelected(std::uint32_t button, bool selected) = 0;
    virtual void ClearDetails(PropertyWindowState& state) = 0;
};

class PropertyWindowList final {
public:
    static constexpr std::uint32_t kItemBytes = 0x210u;
    static constexpr std::uint32_t kItemVtable = 0x8438d0u;

    static void Populate(PropertyWindowState& state,
                         std::uint32_t category, bool preserve_scroll,
                         IPropertyWindowListRuntime& runtime);
};

}  // namespace pal4
