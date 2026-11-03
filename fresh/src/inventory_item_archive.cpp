#include "pal4/inventory_item_archive.h"

#include <new>
#include <limits>
#include <stdexcept>

namespace pal4 {

std::uint8_t InventoryItemArchive::Serialize(
    const std::uint32_t object, const std::uint32_t stream,
    IInventoryItemArchiveRuntime& runtime) {
    runtime.BeforeSerialize(object, stream);
    std::uint8_t result = 0u;
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        result = InventoryItemCollection::SerializeSlot(
            object + 8u + index * 16u, stream, runtime);
        (void)runtime.WriteStreamWord(
            stream, runtime.ReadWord(object + 88u + index * 4u));
    }
    const auto begin = runtime.ReadWord(object + 112u);
    const auto end = runtime.ReadWord(object + 116u);
    const auto count = begin == 0u ? 0u :
        static_cast<std::uint32_t>(
            static_cast<std::int32_t>(end - begin) >> 2);
    (void)runtime.WriteStreamWord(stream, count);
    runtime.WriteStreamWords(stream, begin, count);
    (void)runtime.WriteStreamWord(stream, 0u);
    return result;
}

std::uint8_t InventoryItemArchive::Deserialize(
    const std::uint32_t object, const std::uint32_t stream,
    IInventoryItemArchiveRuntime& runtime) {
    runtime.BeforeDeserialize(object, stream);
    std::uint8_t result = 0u;
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        result = InventoryItemCollection::DeserializeSlot(
            object + 8u + index * 16u, stream, runtime);
        runtime.WriteWord(object + 88u + index * 4u,
                          runtime.ReadStreamWord(stream));
    }

    const auto recent = object + 108u;
    auto begin = runtime.ReadWord(recent + 4u);
    const auto capacity = runtime.ReadWord(recent + 12u);
    runtime.WriteWord(recent + 8u, begin);
    const auto count = static_cast<std::int32_t>(
        runtime.ReadStreamWord(stream));
    if (count > static_cast<std::int32_t>(
                    std::numeric_limits<std::uint32_t>::max() / 4u))
        throw std::length_error("PAL4 item recent array size overflows");
    if (count > 0 &&
        (begin == 0u ||
         static_cast<std::int32_t>(capacity - begin) / 4 < count)) {
        const auto replacement = runtime.AllocateMemory(
            static_cast<std::uint32_t>(count) * 4u);
        if (replacement == 0u) throw std::bad_alloc();
        runtime.FreeMemory(begin);
        begin = replacement;
        runtime.WriteWord(recent + 12u,
                          begin + static_cast<std::uint32_t>(count) * 4u);
        runtime.WriteWord(recent + 4u, begin);
    }
    const auto word_count = count > 0 ?
        static_cast<std::uint32_t>(count) : 0u;
    for (std::uint32_t index = 0u; index < word_count; ++index)
        runtime.WriteWord(begin + index * 4u, 0u);
    runtime.WriteWord(recent + 8u, begin + word_count * 4u);
    runtime.ReadStreamWords(stream, begin, word_count);

    const auto tree_count = static_cast<std::int32_t>(
        runtime.ReadStreamWord(stream));
    for (std::int32_t index = 0; index < tree_count; ++index)
        (void)runtime.ReadStreamWord(stream);
    return result;
}

}  // namespace pal4
