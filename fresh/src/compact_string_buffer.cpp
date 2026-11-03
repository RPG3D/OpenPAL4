#include "pal4/compact_string_buffer.h"

#include <new>

namespace pal4 {

std::uint32_t CompactStringRuntimeStub::Allocate(std::uint32_t) { return 0u; }
std::uint32_t CompactStringRuntimeStub::Free(std::uint32_t) { return 0u; }
std::uint32_t CompactStringRuntimeStub::ReadWord(std::uint32_t) { return 0u; }
std::uint8_t CompactStringRuntimeStub::ReadByte(std::uint32_t) { return 0u; }
void CompactStringRuntimeStub::WriteWord(std::uint32_t, std::uint32_t) {}
void CompactStringRuntimeStub::WriteByte(std::uint32_t, std::uint8_t) {}
void CompactStringRuntimeStub::Copy(
    std::uint32_t, std::uint32_t, std::uint32_t) {}

namespace {

std::uint32_t AllocateBlock(const std::uint32_t length,
                            const std::uint32_t capacity,
                            ICompactStringRuntime& runtime) {
    const auto block = runtime.Allocate(((capacity + 15u) >> 2u) << 2u);
    if (block == 0u) throw std::bad_alloc{};
    runtime.WriteWord(block, length);
    runtime.WriteByte(block + 8u + length, 0u);
    runtime.WriteWord(block + 4u, capacity);
    return block;
}

}  // namespace

std::uint32_t CompactStringBuffer::Reallocate(
    const std::uint32_t object, const std::uint32_t capacity,
    ICompactStringRuntime& runtime) {
    const auto old_block = runtime.ReadWord(object);
    if (capacity <= runtime.ReadWord(old_block + 4u)) return old_block;
    const auto length = runtime.ReadWord(old_block);
    const auto block = capacity == 0u ? kEmptyBlock :
                       AllocateBlock(length, capacity, runtime);
    runtime.Copy(block + 8u, old_block + 8u, length);
    runtime.WriteWord(object, block);
    if (old_block != kEmptyBlock) return runtime.Free(old_block);
    return block;
}

std::uint32_t CompactStringBuffer::Assign(
    const std::uint32_t object, const std::uint32_t source,
    const std::uint32_t length, ICompactStringRuntime& runtime) {
    const auto old_block = runtime.ReadWord(object);
    const auto capacity = runtime.ReadWord(old_block + 4u);
    if (length <= capacity && capacity <= 3u * length + 24u) {
        runtime.Copy(old_block + 8u, source, length);
        runtime.WriteWord(old_block, length);
        runtime.WriteByte(old_block + 8u + length, 0u);
        return object;
    }
    const auto block = length == 0u ? kEmptyBlock :
                       AllocateBlock(length, length, runtime);
    runtime.Copy(block + 8u, source, length);
    runtime.WriteWord(object, block);
    if (old_block != kEmptyBlock) (void)runtime.Free(old_block);
    return object;
}

std::uint32_t CompactStringBuffer::Append(
    const std::uint32_t object, const std::uint32_t source,
    const std::uint32_t length, ICompactStringRuntime& runtime) {
    auto block = runtime.ReadWord(object);
    const auto old_length = runtime.ReadWord(block);
    const auto capacity = runtime.ReadWord(block + 4u);
    const auto new_length = old_length + length;
    if (new_length > capacity) {
        (void)Reallocate(object, new_length + capacity, runtime);
        block = runtime.ReadWord(object);
    }
    runtime.Copy(block + 8u + old_length, source, length);
    runtime.WriteWord(block, new_length);
    runtime.WriteByte(block + 8u + new_length, 0u);
    return object;
}

void CompactStringBuffer::Initialize(
    const std::uint32_t object, const std::uint32_t length,
    const std::uint32_t capacity, ICompactStringRuntime& runtime) {
    if (capacity == 0u) {
        runtime.WriteWord(object, kEmptyBlock);
        return;
    }
    const auto block = AllocateBlock(length, capacity, runtime);
    runtime.WriteWord(object, block);
}

std::uint32_t CompactStringBuffer::FindChar(
    const std::uint32_t object, const std::uint8_t character,
    const std::uint32_t start, ICompactStringRuntime& runtime) {
    const auto block = runtime.ReadWord(object);
    if (start >= runtime.ReadWord(block)) return 0xffffffffu;
    auto offset = start;
    for (;;) {
        const auto current = runtime.ReadByte(block + 8u + offset);
        if (current == 0u) return 0xffffffffu;
        if (current == character) return offset;
        ++offset;
    }
}

}  // namespace pal4
