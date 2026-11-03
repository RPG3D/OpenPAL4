#include "pal4/numbered_list_item.h"

#include <bit>
#include <string>

namespace pal4 {

std::uint32_t NumberedListItem::Construct(
    const std::uint32_t item, const std::int32_t index,
    const std::uint32_t record, const std::uint32_t secondary,
    const bool alternate_image, const std::uint32_t name,
    INumberedListItemConstructionRuntime& runtime) {
    const auto number = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(index) + 1u);
    runtime.ConstructBase(item, std::to_string(number),
                          0u, 0u, false, true);
    runtime.WriteByte(item + 424u, runtime.InitialNameByte());
    runtime.SetErrorFlag(item + 424u, 0u);
    runtime.WriteWord(item, kVtable);
    runtime.AssignName(item + 424u, name, 0u, 0xffffffffu);
    runtime.WriteWord(item + 440u,
                      static_cast<std::uint32_t>(index));
    runtime.CopyBytes(item + 444u, record, 0x34u);
    runtime.WriteWord(item + 500u,
                      runtime.FindImage("gy0", "xuanzhekuang2"));
    runtime.WriteWord(item + 496u,
                      runtime.FindImage("kuang0", "kuang21"));
    runtime.WriteWord(item + 504u,
                      runtime.FindImage("kuang20",
                                        alternate_image ? "kuang21"
                                                        : "kuang20"));
    runtime.WriteWord(item + 508u, secondary);
    return item;
}

std::uint32_t NumberedListItem::ConditionalDestruct(
    const std::uint32_t item, const std::uint8_t flags,
    INumberedListItemRuntime& runtime) {
    Destruct(item, runtime);
    if ((flags & 1u) != 0u) runtime.Free(item);
    return item;
}

void NumberedListItem::Destruct(
    const std::uint32_t item, INumberedListItemRuntime& runtime) {
    const auto data = runtime.ReadWord(item + 428u);
    if (data != 0u) {
        const auto references = runtime.ReadByte(data - 1u);
        if (references == 0u || references == 0xffu)
            runtime.Free(data - 1u);
        else
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(references - 1u));
    }
    runtime.WriteWord(item + 428u, 0u);
    runtime.WriteWord(item + 432u, 0u);
    runtime.WriteWord(item + 436u, 0u);
    runtime.WriteWord(item, runtime.BaseItemVtable());
    runtime.DestroyString(item + 156u);
    runtime.DestroyString(item + 4u);
}

}  // namespace pal4
