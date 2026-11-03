#include "pal4/collection_slot_group_codec.h"

#include <new>
#include <limits>
#include <stdexcept>

namespace pal4 {

std::uint8_t CollectionSlotGroupCodec::Serialize(
    const std::uint32_t object, const std::uint32_t stream,
    ICollectionSlotGroupCodecRuntime& runtime) {
    runtime.ReleaseForWrite(object, stream);
    std::uint8_t result = 0u;
    for (std::uint32_t index = 0u; index < 7u; ++index) {
        result = SerializeSlot(object + 8u + index * 16u, stream,
                               runtime);
    }

    const auto begin = runtime.ReadWord(object + 0x7cu);
    const auto end = runtime.ReadWord(object + 0x80u);
    const auto count = begin != 0u ? (end - begin) / 4u : 0u;
    runtime.WriteStreamWord(stream, count);
    runtime.WriteStreamSpan(stream, begin, count * 4u);

    const auto tree = runtime.CreateEmptyTemporaryTree();
    runtime.WriteStreamWord(stream, 0u);
    runtime.DestroyTemporaryTree(tree);
    return result;
}

std::uint8_t CollectionSlotGroupCodec::SerializeSlot(
    const std::uint32_t slot, const std::uint32_t stream,
    ICollectionSlotGroupCodecRuntime& runtime) {
    runtime.WriteSerializationFlag(slot, stream, "EC_SERIALIZE_FLAG");
    try {
        const auto begin = runtime.ReadWord(slot + 4u);
        const auto end = runtime.ReadWord(slot + 8u);
        const auto count = begin != 0u ? (end - begin) / 4u : 0u;
        runtime.WriteStreamWord(stream, count);
        for (auto cursor = begin; cursor != end; cursor += 4u) {
            const auto record = runtime.ReadWord(cursor);
            (void)InventoryConditionRecord::Serialize(
                record, stream, runtime.RecordRuntime());
        }
    } catch (...) {
        runtime.ReleaseSerializationFlag(slot);
        throw;
    }
    runtime.ReleaseSerializationFlag(slot);
    return 1u;
}

std::uint8_t CollectionSlotGroupCodec::Deserialize(
    const std::uint32_t object, const std::uint32_t stream,
    ICollectionSlotGroupCodecRuntime& runtime) {
    runtime.ReleaseForRead(object, stream);
    std::uint8_t result = 0u;
    for (std::uint32_t index = 0u; index < 7u; ++index) {
        result = DeserializeSlot(object + 8u + index * 16u, stream,
                                 runtime);
    }

    auto begin = runtime.ReadWord(object + 0x7cu);
    runtime.WriteWord(object + 0x80u, begin);
    const auto count = runtime.ReadStreamWord(stream);
    if (count > std::numeric_limits<std::uint32_t>::max() / 4u)
        throw std::length_error("PAL4 equipment recent array size overflows");
    auto capacity = runtime.ReadWord(object + 0x84u);
    const auto available = begin != 0u ? (capacity - begin) / 4u : 0u;
    if (available < count) {
        const auto replacement = runtime.AllocateMemory(count * 4u);
        if (replacement == 0u && count != 0u) throw std::bad_alloc();
        runtime.FreeMemory(begin);
        begin = replacement;
        capacity = replacement + count * 4u;
        runtime.WriteWord(object + 0x7cu, begin);
        runtime.WriteWord(object + 0x84u, capacity);
    }
    for (std::uint32_t index = 0u; index < count; ++index) {
        runtime.WriteWord(begin + index * 4u, 0u);
    }
    runtime.WriteWord(object + 0x80u, begin + count * 4u);
    runtime.ReadStreamSpan(stream, begin, count * 4u);

    const auto tree_count = static_cast<std::int32_t>(
        runtime.ReadStreamWord(stream));
    for (std::int32_t index = 0; index < tree_count; ++index) {
        (void)runtime.ReadStreamWord(stream);
    }
    return result;
}

std::uint8_t CollectionSlotGroupCodec::DeserializeSlot(
    const std::uint32_t slot, const std::uint32_t stream,
    ICollectionSlotGroupCodecRuntime& runtime) {
    if (!runtime.ReadSerializationFlag(slot, stream,
                                       "EC_SERIALIZE_FLAG")) {
        runtime.MarkSerializationError(slot);
        runtime.ReleaseSerializationFlag(slot);
        return 0u;
    }
    try {
        const auto begin = runtime.ReadWord(slot + 4u);
        const auto end = runtime.ReadWord(slot + 8u);
        for (auto cursor = begin; cursor != end; cursor += 4u) {
            const auto record = runtime.ReadWord(cursor);
            if (record != 0u) {
                runtime.DestroyRecord(record);
                runtime.FreeMemory(record);
            }
        }
        runtime.WriteWord(slot + 8u, begin);
        const auto count = static_cast<std::int32_t>(
            runtime.ReadStreamWord(stream));
        std::uint8_t result = 1u;
        for (std::int32_t index = 0; index < count; ++index) {
            const auto record = runtime.AllocateMemory(0x194u);
            if (record == 0u) throw std::bad_alloc();
            runtime.ConstructRecord(record);
            result = static_cast<std::uint8_t>(
                InventoryConditionRecord::Deserialize(
                    record, stream, runtime.RecordRuntime()));
            runtime.AppendRecord(slot, record);
        }
        runtime.ReleaseSerializationFlag(slot);
        return result;
    } catch (...) {
        runtime.ReleaseSerializationFlag(slot);
        throw;
    }
}

}  // namespace pal4
