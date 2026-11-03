#include "pal4/smith_window.h"

#include <array>
#include <cstdint>
#include <new>
#include <string>
#include <string_view>

namespace pal4 {
namespace {

constexpr std::array<std::string_view, 7> kCategoryImages{
    "jian", "shuangjian", "qing", "toubufanju",
    "shengtifangju", "zhubufangju", "peidaia"};

class TemporaryText final {
public:
    TemporaryText(ISmithWindowEquipmentListRuntime& runtime,
                  const std::uint32_t handle)
        : runtime_(runtime), handle_(handle) {}
    ~TemporaryText() {
        if (active_) (void)runtime_.ReleaseText(handle_);
    }
    TemporaryText(const TemporaryText&) = delete;
    TemporaryText& operator=(const TemporaryText&) = delete;

    [[nodiscard]] std::uint32_t Get() const { return handle_; }
    [[nodiscard]] std::uint32_t Release() {
        active_ = false;
        return runtime_.ReleaseText(handle_);
    }

private:
    ISmithWindowEquipmentListRuntime& runtime_;
    std::uint32_t handle_;
    bool active_ = true;
};

}  // namespace

std::uint32_t SmithWindow::UpdateEquipmentList(
    const std::uint32_t object, const std::uint32_t category,
    ISmithWindowEquipmentListRuntime& runtime) {
    const auto list = runtime.ReadWord(object + 0x40u);
    runtime.SetSortDirection(list, 0u);
    runtime.ResetList(list);

    const auto hover_id = runtime.RightHoverItemId(object);
    if (hover_id < 0) return static_cast<std::uint32_t>(hover_id);

    TemporaryText empty_text(runtime, runtime.CreateEmptyText());
    const auto count = runtime.InventorySize(category);
    for (std::int32_t index = 0; index < count; ++index) {
        const auto item = runtime.InventoryAt(category,
                                             static_cast<std::uint32_t>(index));
        runtime.AddRow(list, static_cast<std::uint32_t>(index));

        const auto name = runtime.Utf8Text(runtime.ReadWord(item + 0x9cu));
        const auto enabled = runtime.CheckPlayerCondition(item);
        const auto name_item = runtime.CreateNameItem(
            list, empty_text.Get(), name, enabled);
        if (name_item == 0u) throw std::bad_alloc();
        runtime.WriteWord(name_item + 0x138u,
                          runtime.ReadWord(item + 0x190u));
        runtime.SetItem(list, name_item, 0u, runtime.RowCount(list) - 1u);

        const auto value = SmithWindowItemQuantity::FirstPositive(item,
                                                                 runtime);
        const auto number_item = runtime.CreateNumberItem(
            list, std::to_string(value));
        runtime.SetItem(list, number_item, 1u,
                        runtime.RowCount(list) - 1u);
    }

    if (category < kCategoryImages.size())
        runtime.SetImage(runtime.ReadWord(object + 0x6cu),
                         "zi0", kCategoryImages[category]);

    if (runtime.RowCount(list) != 0u) runtime.SetHoverRow(list, 0u);
    const auto hover_item = runtime.HoverItem(list);
    const auto selected = hover_item == 0u
        ? 0u
        : runtime.FindInventoryItem(
              runtime.ReadWord(hover_item + 0x138u), 7u);
    UpdateEquipmentItemName(object, selected, runtime);
    UpdateEquipmentItemDescription(object, selected, runtime);
    runtime.Refresh(object);
    return empty_text.Release();
}

}  // namespace pal4
