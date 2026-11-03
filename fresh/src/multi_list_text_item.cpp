#include "pal4/multi_list_text_item.h"

namespace pal4 {

std::uint32_t MultiListTextItem::Construct(
    const std::uint32_t item, const std::uint32_t text,
    IMultiListTextItemRuntime& runtime) {
    runtime.ConstructBase(item, text, 0u, 0u, 0u, 1u);
    runtime.InitColourRect(item + 524u, {0.5f, 0.5f, 0.5f, 1.0f});
    runtime.WriteWord(item, kVtable);
    runtime.SetTextColours(item, {0.0f, 1.0f, 0.0f, 1.0f});
    runtime.SetSelectionBrush(item, "OiramLook", "MultiListSelectionBrush");
    return item;
}

void MultiListTextItem::Destruct(
    const std::uint32_t item, IMultiListTextItemRuntime& runtime) {
    runtime.WriteWord(item, runtime.BaseTextVtable());
    runtime.WriteWord(item, runtime.BaseItemVtable());
    runtime.DestroyString(item + 156u);
    runtime.DestroyString(item + 4u);
}

std::uint32_t MultiListTextItem::Render(
    const std::uint32_t item, const std::uint32_t area,
    const std::uint32_t text_target, const std::uint32_t cache,
    const std::uint32_t mode, const float alpha,
    IMultiListTextItemRuntime& runtime) {
    if (runtime.ReadByte(item + 420u) != 0u) {
        const auto image = runtime.ReadWord(item + 416u);
        if (image != 0u) {
            const auto colours = runtime.ModulateColours(
                item, item + 320u, cache, mode);
            runtime.CacheImage(cache, image, area, 7.0f,
                               alpha, colours);
        }
    }
    const auto font = runtime.GetFont(item);
    if (font == 0u) return 0u;
    const auto colour_rect = runtime.ReadByte(item + 421u) == 0u
                                 ? item + 424u : item + 524u;
    const auto x = runtime.ReadFloat(area + 8u) + 7.0f;
    const auto y = runtime.ReadFloat(area + 12u);
    const auto colours = runtime.ModulateColours(
        item, colour_rect, cache, mode);
    return runtime.CacheText(text_target, item + 4u, font, x, y,
                             mode, colours);
}

}  // namespace pal4
