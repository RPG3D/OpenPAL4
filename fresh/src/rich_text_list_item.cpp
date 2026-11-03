#include "pal4/rich_text_list_item.h"

namespace pal4 {

std::uint32_t RichTextListItem::Construct(
    const std::uint32_t item, const std::uint32_t text,
    const float available_width, IRichTextListItemRuntime& runtime) {
    runtime.ConstructBase(item, text, 0u, 0u, false, true);
    runtime.WriteWord(item, kVtable);
    const auto frame = runtime.CreateFrame();
    runtime.WriteWord(item + 424u, frame);
    const auto font = runtime.FindFont("system");
    runtime.SetFormatForSC(frame, true);
    runtime.AssignFrameText(frame, text, font,
                            {0.0f, 0.0f, 0.0f, 1.0f});
    runtime.SetFrameRect(frame,
                         {0.0f, 0.0f, available_width - 20.0f, 0.0f},
                         font);
    runtime.WriteWord(item + 432u,
                      runtime.FindImage("gy0", "fenglanxian"));
    return item;
}

void RichTextListItem::Destruct(
    const std::uint32_t item, IRichTextListItemRuntime& runtime) {
    runtime.WriteWord(item, kVtable);
    runtime.DestroyFrame(runtime.ReadWord(item + 424u));
    runtime.WriteWord(item + 424u, 0u);
    runtime.WriteWord(item, runtime.BaseItemVtable());
    runtime.DestroyString(item + 156u);
    runtime.DestroyString(item + 4u);
}

void RichTextListItem::Render(
    const std::uint32_t item, const std::uint32_t context,
    const std::array<float, 4>& rect, const float alpha,
    const float modulation, const std::uint32_t clip,
    IRichTextListItemRuntime& runtime) {
    (void)modulation; // Original virtual draw uses the first RenderCache argument for both phases.
    auto frame_rect = rect;
    frame_rect[2] += 10.0f;
    frame_rect[3] -= 10.0f;
    runtime.RenderFrame(runtime.ReadWord(item + 424u), context,
                        frame_rect, alpha, 10.0f, clip);

    auto image_rect = rect;
    const auto image = runtime.ReadWord(item + 432u);
    image_rect[0] = image_rect[1] - runtime.ReadFloat(image + 32u);
    runtime.CacheImage(context, image, image_rect, alpha, clip,
                       {1.0f, 1.0f, 1.0f, 1.0f}, false, false);
}

std::uint32_t RichTextListItem::GetSize(
    const std::uint32_t item, const std::uint32_t output,
    IRichTextListItemRuntime& runtime) {
    const auto frame = runtime.ReadWord(item + 424u);
    const auto text_height = runtime.GetTextHeight(frame, 10.0f) + 10.0f;
    const auto parent_rect = runtime.GetParentRect(
        runtime.ReadWord(item + 316u), 276u);
    const auto image = runtime.ReadWord(item + 432u);
    const auto content_height = image != 0u
                                    ? runtime.ReadFloat(image + 32u) +
                                          text_height
                                    : text_height;
    const auto span = parent_rect[1] - parent_rect[0];
    runtime.WriteFloat(output, runtime.ReadFloat(item + 428u));
    const auto rows = runtime.FloatToLong(content_height / span);
    runtime.WriteFloat(output + 4u,
                       (static_cast<float>(rows) + 1.0f) * span);
    return output;
}

}  // namespace pal4
