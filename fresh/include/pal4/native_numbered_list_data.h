#pragma once

#include "pal4/numbered_list_item_render.h"
#include "pal4/native_cegui_widget_images.h"
#include "pal4/ui_icon_catalog.h"

namespace pal4 {
class NativeGameDatabase;

// Database and DLL must outlive this provider and every item borrowing it.
class NativeNumberedListData final : public INumberedListRenderDataRuntime,
                                     private IUiIconImagesRuntime {
public:
    NativeNumberedListData(HMODULE module, const NativeGameDatabase& database);
    std::uint32_t GetIconImage(std::uint32_t slot, std::uint32_t category,
                              std::uint32_t variant) override;
    std::string FormatItemCount(std::uint32_t trigger, std::uint32_t count) override;
    std::vector<NumberedGameRecord> GetGameRecords() override;
    bool TriggerObject(std::uint32_t id) override;
    void TriggerPlayer(std::uint32_t id) override;
private:
    std::uint32_t GetImageset(std::string_view name) override;
    std::uint32_t GetImage(std::uint32_t set, std::string_view name) override;
    NativeCeguiWidgetImages images_;
    const NativeGameDatabase& database_;
};
}  // namespace pal4
