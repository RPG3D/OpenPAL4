#include "pal4/ol_default_item.h"

namespace pal4 {

std::uint32_t OlDefaultItem::Construct(
    const std::uint32_t item, const std::uint32_t list,
    const std::uint32_t text, const bool equipped,
    IOlDefaultItemRuntime& runtime) {
    runtime.ConstructBase(item, list, text);
    runtime.WriteWord(item, kVtable);
    runtime.WriteWord(item + 528u,
                      equipped ? runtime.FindImage("gy0", "equiped") : 0u);
    return item;
}

void OlDefaultItem::Destruct(const std::uint32_t item,
                             IOlDefaultItemRuntime& runtime) {
    runtime.WriteWord(item, runtime.BaseTextVtable());
    runtime.WriteWord(item, runtime.BaseItemVtable());
    runtime.DestroyString(item + 156u);
    runtime.DestroyString(item + 4u);
}

void OlDefaultItem::Render(
    const std::uint32_t item, const std::uint32_t cache,
    const OlRect& rect, const float alpha, const float modulation,
    const std::uint32_t clip, const std::uint32_t format,
    IOlDefaultItemRuntime& runtime) {
    runtime.DrawBase(item, cache, rect, alpha, modulation, clip, format);
    const auto image = runtime.ReadWord(item + 528u);
    if (image == 0u) return;

    OlRect image_rect = rect;
    image_rect[2] += 104.0f;
    image_rect[3] = image_rect[2] + runtime.ReadFloat(image + 28u);
    const float height = runtime.ReadFloat(image + 32u);
    const float remaining = rect[1] - rect[0] - height;
    if (remaining > 0.0f) {
        image_rect[0] += remaining * 0.5f - 2.0f;
    }
    image_rect[1] = image_rect[0] + height;
    runtime.CacheImage(cache, image, image_rect, alpha, clip,
                       {1.0f, 1.0f, 1.0f, 1.0f}, false, false);
}

}  // namespace pal4
