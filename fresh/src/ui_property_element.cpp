#include "pal4/ui_property_element.h"

#include "pal4/indexed_property_grid.h"

#include <bit>
#include <cstdlib>
#include <limits>
#include <stdexcept>

namespace pal4 {
namespace {

std::int32_t WordAsSigned(const std::int64_t value) {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value));
}

}  // namespace

std::uint32_t UiPropertyElement::Initialize(
    const std::uint32_t object, const std::uint32_t property,
    const std::array<std::int32_t, 2>& origin,
    const std::array<std::int32_t, 2>& extent,
    const std::array<std::uint32_t, 3>& images,
    IUiPropertyElementRuntime& runtime) {
    runtime.WriteWord(object, static_cast<std::uint32_t>(origin[0]));
    runtime.WriteWord(object + 4u, static_cast<std::uint32_t>(origin[1]));
    runtime.WriteWord(object + 8u, static_cast<std::uint32_t>(extent[0]));
    runtime.WriteWord(object + 12u, static_cast<std::uint32_t>(extent[1]));
    runtime.WriteWord(object + 20u, 32u);
    runtime.WriteWord(object + 24u, 0u);
    runtime.WriteWord(object + 32u, 0u);
    runtime.WriteWord(object + 28u, 0u);
    runtime.WriteWord(object + 16u, 0u);
    runtime.WriteWord(object + 36u, 0u);
    runtime.WriteWord(object + 168u, images[0]);
    runtime.WriteByte(object + 196u, 0u);
    runtime.WriteWord(object + 172u, images[1]);
    runtime.WriteWord(object + 176u, images[2]);
    runtime.WriteWord(object + 192u, property);
    runtime.WriteWord(object + 180u, 0u);
    runtime.WriteWord(object + 184u, 0u);
    runtime.WriteWord(object + 188u, 0u);
    return object;
}

std::uint32_t UiPropertyElement::SetOverlayImages(
    const std::uint32_t object,
    const std::array<std::uint32_t, 3>& images,
    IUiPropertyElementRuntime& runtime) {
    runtime.WriteWord(object + 180u, images[0]);
    runtime.WriteWord(object + 184u, images[1]);
    runtime.WriteWord(object + 188u, images[2]);
    return images[2];
}

std::uint32_t UiPropertyElement::AssignText(
    const std::uint32_t object, const std::uint32_t source,
    IUiPropertyElementRuntime& runtime) {
    return runtime.AssignCeguiString(object + 16u, source, 0u, 0xffffffffu);
}

void UiPropertyElement::CopyConstruct(
    const std::uint32_t destination, const std::uint32_t source,
    IUiPropertyElementRuntime& runtime) {
    if (destination == 0) return;
    for (std::uint32_t offset = 0; offset < 16; offset += 4) {
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    }
    runtime.WriteWord(destination + 20u, 32u);
    for (const std::uint32_t offset : {16u, 24u, 28u, 32u, 36u}) {
        runtime.WriteWord(destination + offset, 0);
    }
    runtime.CopyConstructCeguiString(destination + 16u, source + 16u);
    for (std::uint32_t offset = 168; offset <= 192; offset += 4) {
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    }
    runtime.WriteByte(destination + 196u,
                      runtime.ReadByte(source + 196u));
}

std::uint32_t UiPropertyElement::Assign(
    const std::uint32_t destination, const std::uint32_t source,
    IUiPropertyElementRuntime& runtime) {
    for (std::uint32_t offset = 0; offset < 16; offset += 4) {
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    }
    static_cast<void>(runtime.AssignCeguiString(
        destination + 16u, source + 16u, 0u, 0xffffffffu));
    for (std::uint32_t offset = 168; offset <= 192; offset += 4) {
        runtime.WriteWord(destination + offset,
                          runtime.ReadWord(source + offset));
    }
    runtime.WriteByte(destination + 196u,
                      runtime.ReadByte(source + 196u));
    return destination;
}

void UiPropertyElement::DestroyRange(
    const std::uint32_t begin, const std::uint32_t end,
    IUiPropertyElementRuntime& runtime) {
    if (end < begin || (end - begin) % 200u != 0) {
        throw std::invalid_argument("UI element destruction range");
    }
    for (std::uint32_t address = begin; address != end; address += 200u) {
        runtime.DestroyElementString(address);
    }
}

std::uint32_t UiPropertyElement::AssignRange(
    const std::uint32_t source_begin, const std::uint32_t source_end,
    const std::uint32_t destination, IUiPropertyElementRuntime& runtime) {
    if (source_end < source_begin ||
        (source_end - source_begin) % 200u != 0 ||
        destination > std::numeric_limits<std::uint32_t>::max() -
                          (source_end - source_begin)) {
        throw std::invalid_argument("UI element assignment range");
    }
    const auto bytes = source_end - source_begin;
    for (std::uint32_t offset = 0; offset < bytes; offset += 200u) {
        static_cast<void>(Assign(destination + offset, source_begin + offset,
                                 runtime));
    }
    return destination + bytes;
}

bool UiPropertyElement::CanOccupy(
    const std::uint32_t object,
    const std::array<std::int32_t, 2>& origin,
    const IndexedPropertyGrid& grid,
    IUiPropertyElementRuntime& runtime) {
    const auto old_x = static_cast<std::int32_t>(runtime.ReadWord(object));
    const auto old_y = static_cast<std::int32_t>(runtime.ReadWord(object + 4u));
    if (std::llabs(static_cast<long long>(origin[0]) - old_x) +
            std::llabs(static_cast<long long>(origin[1]) - old_y) > 1) {
        return false;
    }
    const auto width = static_cast<std::int32_t>(runtime.ReadWord(object + 8u));
    const auto height = static_cast<std::int32_t>(runtime.ReadWord(object + 12u));
    const auto property = static_cast<std::int32_t>(
        runtime.ReadWord(object + 192u));
    for (std::int64_t x = origin[0];
         x < static_cast<std::int64_t>(origin[0]) + width; ++x) {
        for (std::int64_t y = origin[1];
             y < static_cast<std::int64_t>(origin[1]) + height; ++y) {
            const auto value = grid.Get(WordAsSigned(x), WordAsSigned(y));
            if (value != 0 && value != property) return false;
        }
    }
    return true;
}

std::int32_t UiPropertyElement::MoveAndPaint(
    const std::uint32_t object,
    const std::array<std::int32_t, 2>& origin,
    IndexedPropertyGrid& grid,
    IUiPropertyElementRuntime& runtime) {
    const auto old_x = static_cast<std::int32_t>(runtime.ReadWord(object));
    const auto old_y = static_cast<std::int32_t>(runtime.ReadWord(object + 4u));
    const auto width = static_cast<std::int32_t>(runtime.ReadWord(object + 8u));
    const auto height = static_cast<std::int32_t>(runtime.ReadWord(object + 12u));
    const auto property = static_cast<std::int32_t>(
        runtime.ReadWord(object + 192u));
    for (std::int64_t x = old_x;
         x < static_cast<std::int64_t>(old_x) + width; ++x) {
        for (std::int64_t y = old_y;
             y < static_cast<std::int64_t>(old_y) + height; ++y) {
            static_cast<void>(grid.Set(WordAsSigned(x), WordAsSigned(y), 0));
        }
    }
    runtime.WriteWord(object, static_cast<std::uint32_t>(origin[0]));
    runtime.WriteWord(object + 4u, static_cast<std::uint32_t>(origin[1]));
    for (std::int64_t x = origin[0];
         x < static_cast<std::int64_t>(origin[0]) + width; ++x) {
        for (std::int64_t y = origin[1];
             y < static_cast<std::int64_t>(origin[1]) + height; ++y) {
            static_cast<void>(grid.Set(WordAsSigned(x), WordAsSigned(y),
                                       property));
        }
    }
    return WordAsSigned(static_cast<std::int64_t>(origin[0]) + width);
}

void UiPropertyElement::Render(
    const std::uint32_t object,
    const std::array<float, 2>& origin,
    const std::array<float, 2>& scale,
    const float depth, const std::uint32_t font, const bool draw_front,
    IUiPropertyRenderRuntime& runtime) {
    UiFloatRect rect = ScreenRect(object, origin, scale, runtime);
    runtime.PrepareWhiteColour();
    const bool selected = runtime.ReadByte(object + 196u) != 0u;
    if (selected) {
        const auto image = runtime.ReadWord(object + 172u);
        if (image != 0u) runtime.DrawImage(image, rect, depth);
        const auto overlay = runtime.ReadWord(object + 184u);
        if (overlay != 0u) runtime.DrawImage(overlay, rect, depth);
    } else {
        runtime.DrawImage(runtime.ReadWord(object + 168u), rect, depth);
        const auto overlay = runtime.ReadWord(object + 180u);
        if (overlay != 0u) runtime.DrawImage(overlay, rect, depth);
    }
    if (draw_front) {
        const auto image = runtime.ReadWord(object + 176u);
        if (image != 0u) runtime.DrawImage(image, rect, depth);
        const auto overlay = runtime.ReadWord(object + 188u);
        if (overlay != 0u) runtime.DrawImage(overlay, rect, depth);
    }
    if (font == 0u || runtime.ReadWord(object + 16u) == 0u) return;

    const float metric = runtime.ReadFontMetric(font);
    // IDA5AB114..5AB13D adjusts the first two PAL4 CEGUI Rect words:
    // top/bottom. Font+208 is its height; drawText's centred formatting
    // handles the horizontal axis using the unchanged cell width.
    const double spare_height = static_cast<double>(rect.bottom) - rect.top - metric;
    if (spare_height > 0.0) {
        const double new_top = spare_height * 0.5 + rect.top;
        rect.top = static_cast<float>(new_top);
        rect.bottom = static_cast<float>(new_top + metric);
    }
    runtime.PrepareDefaultFontColours();
    runtime.DrawText(font, object + 16u, rect, depth, 2u);
}

bool UiPropertyElement::ContainsPoint(
    const std::uint32_t object,
    const std::array<std::int32_t, 2>& point,
    IUiPropertyElementRuntime& runtime) {
    const auto left = WordAsSigned(runtime.ReadWord(object));
    if (point[0] < left) return false;
    const auto top = WordAsSigned(runtime.ReadWord(object + 4u));
    if (point[1] < top) return false;
    const auto width = WordAsSigned(runtime.ReadWord(object + 8u));
    if (static_cast<std::int64_t>(point[0]) >=
        static_cast<std::int64_t>(left) + width) return false;
    const auto height = WordAsSigned(runtime.ReadWord(object + 12u));
    return static_cast<std::int64_t>(point[1]) <
           static_cast<std::int64_t>(top) + height;
}

UiFloatRect UiPropertyElement::ScreenRect(
    const std::uint32_t object,
    const std::array<float, 2>& origin,
    const std::array<float, 2>& scale,
    IUiPropertyElementRuntime& runtime) {
    const float left = static_cast<float>(
        static_cast<double>(WordAsSigned(runtime.ReadWord(object))) *
            scale[0] + origin[0]);
    const float top = static_cast<float>(
        static_cast<double>(WordAsSigned(runtime.ReadWord(object + 4u))) *
            scale[1] + origin[1]);
    const float right = static_cast<float>(
        static_cast<double>(WordAsSigned(runtime.ReadWord(object + 8u))) *
            scale[0] + left);
    const float bottom = static_cast<float>(
        static_cast<double>(WordAsSigned(runtime.ReadWord(object + 12u))) *
            scale[1] + top);
    return {left, right, top, bottom};
}

std::array<std::int32_t, 2> UiPropertyElement::AdjustPosition(
    const std::uint32_t object,
    const std::array<std::int32_t, 2>& target,
    IUiPropertyElementRuntime& runtime) {
    auto result = target;
    const auto left = WordAsSigned(runtime.ReadWord(object));
    if (target[0] > left) {
        result[0] = WordAsSigned(static_cast<std::int64_t>(target[0]) -
                                WordAsSigned(runtime.ReadWord(object + 8u)) + 1);
    }
    const auto top = WordAsSigned(runtime.ReadWord(object + 4u));
    if (target[1] > top) {
        result[1] = WordAsSigned(static_cast<std::int64_t>(target[1]) -
                                WordAsSigned(runtime.ReadWord(object + 12u)) + 1);
    }
    return result;
}

}  // namespace pal4
