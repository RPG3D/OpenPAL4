#include "pal4/help_list_item.h"

namespace pal4 {

std::uint32_t HelpListItem::Construct(
    const std::uint32_t item, const std::uint32_t list,
    const std::u32string_view text, const std::uint8_t unread,
    IHelpListItemRuntime& runtime) {
    runtime.ConstructBase(item, list, text);
    try {
        runtime.WriteWord(item, kVtable);
        SetImage(item, unread, runtime);
    } catch (...) {
        runtime.DestroyBase(item);
        throw;
    }
    return item;
}

void HelpListItem::Destroy(
    const std::uint32_t item, IHelpListItemRuntime& runtime) {
    runtime.WriteWord(item, runtime.TextItemVtable());
    runtime.WriteWord(item, runtime.ItemVtable());
    runtime.DestroyString(item + 0x9cu);
    runtime.DestroyString(item + 4u);
}

void HelpListItem::SetImage(
    const std::uint32_t item, const std::uint8_t unread,
    IHelpListItemRuntime& runtime) {
    const auto image = unread == 0u
        ? 0u : runtime.ResolveImage("gy0", "equiped");
    runtime.WriteWord(item + 0x210u, image);
}

HelpItemRect HelpListItem::UnreadImageRect(const HelpItemRect& rect,const HelpImageSize size) {
    // CEGUI0.4 Rect's memory order is top,bottom,left,right (not x0,x1,y0,y1).
    // IDA4A8A60 adds140 to [2] (left), and centers [0] in the row height.
    const auto available=rect.bottom-rect.top-size.height;
    const auto top=available>0.0f?rect.top+available*0.5f-2.0f:rect.top;
    const auto left=rect.left+140.0f;
    return {left,left+size.width,top,top+size.height};
}
void HelpListItem::Draw(
    const std::uint32_t item, const std::uint32_t cache,
    const HelpItemRect& rect, const float alpha,
    const std::uint32_t mode, const std::uint32_t clip,
    const std::uint32_t flags, IHelpListItemRuntime& runtime) {
    runtime.DrawBase(item, cache, rect, alpha, mode, clip, flags);
    const auto image = runtime.ReadWord(item + 0x210u);
    if (image == 0u) return;

    runtime.CacheImage(cache,image,UnreadImageRect(rect,runtime.ImageSize(image)),alpha);
}

}  // namespace pal4
