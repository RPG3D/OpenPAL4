#include "pal4/numbered_list_item.h"

#include <bit>

#include "pal4/numbered_list_item_render.h"
#include "pal4/time_formatter.h"

namespace pal4 {

void NumberedListItem::Render(
    const std::uint32_t item, const std::uint32_t cache,
    const std::array<float, 4>& rect, const float alpha,
    const float modulation, const std::array<float, 4>& clip,
    INumberedListRenderRuntime& runtime) {
    auto adjusted_clip = clip;
    adjusted_clip[1] += 4.0f;
    const auto detail_image = runtime.ReadWord(item + 504u);
    const std::array<float, 4> frame_rect{
        rect[0], rect[0] + runtime.ReadFloat(detail_image + 32u),
        rect[2], rect[2] + runtime.ReadFloat(detail_image + 28u)};
    const auto font = runtime.GetSystemFont();
    if (font == 0u) return;

    const auto detailed = runtime.ReadWord(item + 492u) != 0u;
    if (detailed) {
        const auto secondary_image = runtime.ReadWord(item + 508u);
        if (secondary_image != 0u) {
            const std::array<float, 4> secondary_rect{
                rect[0] + 7.0f, rect[0] + 82.0f,
                rect[2] + 495.0f, rect[2] + 593.0f};
            runtime.CacheWhiteImage(cache, secondary_image,
                                    secondary_rect, alpha,
                                    adjusted_clip, false, false);
        }
        runtime.CacheWhiteImage(cache, detail_image, frame_rect,
                                alpha, adjusted_clip, true, false);
    } else {
        runtime.CacheWhiteImage(cache, runtime.ReadWord(item + 496u),
                                frame_rect, alpha, adjusted_clip,
                                true, false);
    }

    const std::array<float, 4> number_rect{
        rect[0] + 37.0f, rect[0] + 50.0f,
        rect[2] + 7.0f, rect[2] + 21.0f};
    const auto number_colour = runtime.ModulatedBlack(item, modulation);
    runtime.CacheItemText(cache, item + 4u, font, 2u,
                          number_rect, alpha, adjusted_clip,
                          number_colour);

    if (detailed) {
        float icon_x = rect[2] + 27.0f;
        const auto variant = runtime.ReadWord(item + 444u) != 0u
                                 ? 2u : 1u;
        for (std::uint32_t slot = 0u; slot < 4u; ++slot) {
            const auto present = runtime.ReadWord(item + 448u + 4u * slot);
            if (present == 0u) continue;
            const auto icon = runtime.GetIconImage(slot, 6u, variant);
            const std::array<float, 4> icon_rect{
                rect[0] + 3.0f, rect[0] + 74.0f,
                icon_x, icon_x + 53.0f};
            auto icon_clip = runtime.IntersectRect(icon_rect,
                                                   adjusted_clip);
            icon_clip[1] -= 1.0f;
            runtime.CacheWhiteImage(cache, icon, icon_rect, alpha,
                                    icon_clip, false, false);
            const auto level = runtime.ReadWord(item + 464u + 4u * slot);
            const auto formatted = runtime.FormatItemCount(0xc9u, level);
            const auto utf8 = runtime.ConvertToUtf8(formatted);
            const std::array<float, 4> count_rect{
                rect[0] + 70.0f, rect[0] + 81.0f,
                icon_x, icon_x + 53.0f};
            runtime.CacheBlackText(cache, utf8,
                                   NumberedTextEncoding::Utf8,
                                   font, 2u, count_rect, alpha,
                                   adjusted_clip);
            icon_x += 53.0f;
        }

        const auto threshold = std::bit_cast<std::int32_t>(
            runtime.ReadWord(item + 480u));
        const auto records = runtime.GetGameRecords();
        const NumberedGameRecord* last = nullptr;
        for (const auto& record : records)
            if (record.threshold <= threshold) last = &record;
        if (last != nullptr) {
            const std::array<float, 4> first_rect{
                rect[0] + 38.0f, rect[0] + 52.0f,
                rect[2] + 259.0f, rect[2] + 303.0f};
            runtime.CacheBlackText(
                cache, runtime.ConvertToUtf8(last->first_text),
                NumberedTextEncoding::Utf8, font, 0u,
                first_rect, alpha, adjusted_clip);
            const std::array<float, 4> second_rect{
                rect[0] + 64.0f, rect[0] + 77.0f,
                rect[2] + 259.0f, rect[2] + 471.0f};
            runtime.CacheBlackText(
                cache, runtime.ConvertToUtf8(last->second_text),
                NumberedTextEncoding::Utf8, font, 0u,
                second_rect, alpha, adjusted_clip);
        }

        const auto seconds = std::bit_cast<std::int32_t>(
            runtime.ReadWord(item + 484u));
        const std::array<float, 4> time_rect{
            rect[0] + 38.0f, rect[0] + 52.0f,
            rect[2] + 325.0f, rect[2] + 400.0f};
        runtime.CacheBlackText(
            cache, runtime.ConvertToUtf8(
                TimeFormatter::FormatSeconds(seconds)),
            NumberedTextEncoding::Utf8, font, 0u,
            time_rect, alpha, adjusted_clip);

        const auto trigger = runtime.ReadWord(item + 488u);
        if (runtime.TriggerObject(trigger)) {
            runtime.TriggerPlayer(trigger);
            const auto name_data = runtime.ReadWord(item + 428u);
            const std::array<float, 4> name_rect{
                rect[0] + 11.0f, rect[0] + 27.0f,
                rect[2] + 259.0f, rect[2] + 315.0f};
            runtime.CacheBlackText(
                cache, runtime.ConvertToUtf8(
                    name_data != 0u ? runtime.ReadCString(name_data)
                                    : std::string{}),
                NumberedTextEncoding::Utf8, font, 0u,
                name_rect, alpha, adjusted_clip);
        }
    }

    if (runtime.ReadByte(item + 423u) != 0u)
        runtime.CacheWhiteImage(cache, runtime.ReadWord(item + 500u),
                                frame_rect, alpha, adjusted_clip,
                                true, false);
}

std::uint32_t NumberedListItem::GetSize(
    const std::uint32_t item, const std::uint32_t output,
    INumberedListRenderRuntime& runtime) {
    runtime.WriteWord(output, runtime.ReadWord(item + 512u));
    runtime.WriteWord(output + 4u,
                      runtime.ReadWord(item + 516u));
    return output;
}

std::uint32_t NumberedListItem::SetSize(
    const std::uint32_t item, const std::uint32_t second,
    INumberedListRenderRuntime& runtime) {
    const auto image = runtime.ReadWord(item + 504u);
    runtime.WriteWord(item + 512u,
                      std::bit_cast<std::uint32_t>(
                          runtime.ReadFloat(image + 28u)));
    runtime.WriteWord(item + 516u, second);
    return image;
}

}  // namespace pal4
