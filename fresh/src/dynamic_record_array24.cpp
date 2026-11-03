#include "pal4/dynamic_record_array24.h"

#include <algorithm>
#include <array>
#include <limits>
#include <new>
#include <stdexcept>

namespace pal4 {
namespace {

void CopyRecord(const std::uint32_t destination,
                const std::uint32_t source,
                IDynamicRecordArray24Memory& memory) {
    std::array<std::uint32_t, 6> words{};
    for (std::uint32_t index = 0u; index < words.size(); ++index)
        words[index] = memory.ReadDword(source + index * 4u);
    for (std::uint32_t index = 0u; index < words.size(); ++index)
        memory.WriteDword(destination + index * 4u, words[index]);
}

}  // namespace

std::uint32_t DynamicRecordArray24::Insert(
    const std::uint32_t array, const std::uint32_t position,
    const std::uint32_t count, const std::uint32_t source,
    IDynamicRecordArray24Memory& memory) {
    const auto begin = memory.ReadDword(array + 4u);
    const auto end = memory.ReadDword(array + 8u);
    const auto capacity_end = memory.ReadDword(array + 12u);
    const auto free_count = (capacity_end - end) / kStride;
    if (count > std::numeric_limits<std::uint32_t>::max() / kStride)
        throw std::length_error("24-byte array insertion overflows 32-bit memory");
    const auto bytes = count * kStride;

    if (free_count < count) {
        const auto size = begin == 0u ? 0u : (end - begin) / kStride;
        const auto new_capacity_wide =
            static_cast<std::uint64_t>(size) + std::max(size, count);
        if (new_capacity_wide > std::numeric_limits<std::uint32_t>::max() / kStride)
            throw std::length_error("24-byte array capacity overflows 32-bit memory");
        const auto new_capacity = static_cast<std::uint32_t>(new_capacity_wide);
        const auto replacement = memory.Allocate(new_capacity * kStride);
        if (replacement == 0u) throw std::bad_alloc();

        auto destination = replacement;
        for (auto cursor = begin; cursor != position; cursor += kStride) {
            CopyRecord(destination, cursor, memory);
            destination += kStride;
        }
        for (std::uint32_t index = 0u; index < count; ++index) {
            CopyRecord(destination, source, memory);
            destination += kStride;
        }
        for (auto cursor = position; cursor != end; cursor += kStride) {
            CopyRecord(destination, cursor, memory);
            destination += kStride;
        }
        memory.Free(begin);
        memory.WriteDword(array + 4u, replacement);
        memory.WriteDword(array + 8u, replacement + (size + count) * kStride);
        memory.WriteDword(array + 12u, replacement + new_capacity * kStride);
        return 0u;
    }

    const auto suffix_count = (end - position) / kStride;
    if (suffix_count < count) {
        for (auto cursor = position; cursor != end; cursor += kStride)
            CopyRecord(cursor + bytes, cursor, memory);
        for (auto cursor = end; cursor != position + bytes; cursor += kStride)
            CopyRecord(cursor, source, memory);
        for (auto cursor = position; cursor != end; cursor += kStride)
            CopyRecord(cursor, source, memory);
        memory.WriteDword(array + 8u, end + bytes);
        return end + bytes;
    }

    if (count == 0u) return 0u;
    for (auto cursor = end - bytes; cursor != end; cursor += kStride)
        CopyRecord(cursor + bytes, cursor, memory);
    for (auto cursor = end - bytes; cursor != position; ) {
        cursor -= kStride;
        CopyRecord(cursor + bytes, cursor, memory);
    }
    for (auto cursor = position; cursor != position + bytes; cursor += kStride)
        CopyRecord(cursor, source, memory);
    memory.WriteDword(array + 8u, end + bytes);
    return position + bytes;
}

std::uint32_t DynamicRecordArray24MemoryStub::Allocate(const std::uint32_t bytes) {
    const auto result = next_address_;
    next_address_ += bytes + 0x100u;
    return result;
}

void DynamicRecordArray24MemoryStub::Free(const std::uint32_t address) {
    freed_blocks_.push_back(address);
}

std::uint32_t DynamicRecordArray24MemoryStub::ReadDword(
    const std::uint32_t address) const {
    const auto entry = words_.find(address);
    return entry == words_.end() ? 0u : entry->second;
}

void DynamicRecordArray24MemoryStub::WriteDword(
    const std::uint32_t address, const std::uint32_t value) {
    words_[address] = value;
}

}  // namespace pal4
