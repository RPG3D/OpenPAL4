#include "pal4/minimap_collection.h"

#include <new>

namespace pal4 {

std::uint32_t MinimapCollection::AllocateAndConstruct(
    const std::uint8_t stack_byte, IMinimapCollectionRuntime& runtime) {
    const auto address = runtime.Allocate(kSize);
    if (address == 0u) return 0u;
    try {
        return ConstructAt(address, stack_byte, runtime);
    } catch (...) {
        runtime.Free(address);
        throw;
    }
}

std::uint32_t MinimapCollection::ConstructAt(
    const std::uint32_t address, const std::uint8_t stack_byte,
    IMinimapCollectionRuntime& runtime) {
    runtime.WriteByte(address + 4u, stack_byte);
    runtime.WriteDword(address + 8u, 0u);
    runtime.WriteDword(address + 12u, 0u);
    runtime.WriteDword(address + 16u, 0u);
    runtime.WriteDword(address, kVtableAddress);
    runtime.WriteDword(address + 20u, 0u);
    return address;
}

std::uint32_t MinimapCollection::CleanupElements(
    const std::uint32_t address, IMinimapCollectionRuntime& runtime) {
    const auto begin = runtime.ReadDword(address + 8u);
    const auto end = runtime.ReadDword(address + 12u);
    for (auto cursor = begin; cursor != end; cursor += 4u) {
        const auto element = runtime.ReadDword(cursor);
        if (element == 0u) continue;
        runtime.Free(runtime.ReadDword(element + 4u));
        runtime.WriteDword(element + 4u, 0u);
        runtime.WriteDword(element + 8u, 0u);
        runtime.WriteDword(element + 12u, 0u);
        runtime.Free(element);
    }
    runtime.WriteDword(address + 12u, begin);
    runtime.WriteDword(address + 20u, 0u);
    return end;
}

void MinimapCollection::Cleanup(
    const std::uint32_t address, IMinimapCollectionRuntime& runtime) {
    runtime.WriteDword(address, kVtableAddress);
    (void)CleanupElements(address, runtime);
    runtime.Free(runtime.ReadDword(address + 8u));
    runtime.WriteDword(address + 8u, 0u);
    runtime.WriteDword(address + 12u, 0u);
    runtime.WriteDword(address + 16u, 0u);
    SetBaseVtable(address, runtime);
}

void MinimapCollection::SetBaseVtable(
    const std::uint32_t address, IMinimapCollectionRuntime& runtime) {
    runtime.WriteDword(address, kBaseVtableAddress);
}

std::uint32_t MinimapCollection::DestroyBase(
    const std::uint32_t address, const std::uint8_t flags,
    IMinimapCollectionRuntime& runtime) {
    SetBaseVtable(address, runtime);
    if ((flags & 1u) != 0u) runtime.Free(address);
    return address;
}

std::uint32_t MinimapCollection::Destroy(
    const std::uint32_t address, const std::uint8_t flags,
    IMinimapCollectionRuntime& runtime) {
    Cleanup(address, runtime);
    if ((flags & 1u) != 0u) runtime.Free(address);
    return address;
}

std::uint32_t MinimapCollection::Serialize(
    const std::uint32_t address, IMinimapOutputRuntime& runtime) {
    const auto begin = runtime.ReadDword(address + 8u);
    const auto end = runtime.ReadDword(address + 12u);
    const auto count = begin == 0u ? 0u :
        static_cast<std::uint32_t>(static_cast<std::int32_t>(end - begin) >> 2);
    const auto header = (runtime.ReadDword(address + 20u) << 16u) | count;
    const auto header_result = runtime.WriteValue(header);
    if (begin == 0u) return header_result;

    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto element = runtime.ReadDword(begin + index * 4u);
        const auto data_begin = runtime.ReadDword(element + 4u);
        const auto data_end = runtime.ReadDword(element + 8u);
        const auto element_count = data_begin == 0u ? 0u :
            static_cast<std::uint32_t>(
                static_cast<std::int32_t>(data_end - data_begin) >> 2);
        (void)runtime.WriteValue(element_count);
        (void)runtime.WriteBytes(data_begin, element_count * 4u);
    }
    return count;
}

std::uint32_t MinimapCollection::Deserialize(
    const std::uint32_t address, const std::uint32_t stream_address,
    IMinimapInputRuntime& runtime) {
    (void)CleanupElements(address, runtime);
    const auto header = runtime.ReadValue();
    runtime.WriteDword(address + 20u, header >> 16u);
    const auto count = header & 0xffffu;
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto element = runtime.Allocate(16u);
        if (element == 0u) throw std::bad_alloc();
        runtime.WriteByte(element, static_cast<std::uint8_t>(stream_address));
        runtime.WriteDword(element + 4u, 0u);
        runtime.WriteDword(element + 8u, 0u);
        runtime.WriteDword(element + 12u, 0u);

        const auto element_count = runtime.ReadValue();
        const auto old_begin = runtime.ReadDword(element + 4u);
        const auto old_end = runtime.ReadDword(element + 8u);
        const auto old_size = old_begin == 0u ? 0u :
            (old_end - old_begin) / 4u;
        if (old_size < element_count) {
            const auto new_begin = runtime.Allocate(element_count * 4u);
            if (new_begin == 0u) throw std::bad_alloc();
            for (std::uint32_t i = 0u; i < old_size; ++i) {
                runtime.WriteDword(new_begin + i * 4u,
                                   runtime.ReadDword(old_begin + i * 4u));
            }
            runtime.Free(old_begin);
            runtime.WriteDword(element + 12u,
                               new_begin + element_count * 4u);
            runtime.WriteDword(element + 4u, new_begin);
            runtime.WriteDword(element + 8u,
                               new_begin + old_size * 4u);
        }
        runtime.ResizeInner(element, element_count, 0u);
        runtime.ReadBytes(runtime.ReadDword(element + 4u),
                          element_count * 4u);
        runtime.InsertElement(address + 4u, element);
    }
    return count;
}

}  // namespace pal4
