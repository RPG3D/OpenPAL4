#include "pal4/d3dx_shader_comment_block.h"

#include <cstdlib>
#include <cstring>
#include <limits>

namespace pal4 {
namespace {

constexpr std::int32_t kFailure = static_cast<std::int32_t>(0x80004005u);
constexpr std::int32_t kNoMemory = static_cast<std::int32_t>(0x8007000eu);

}  // namespace

D3DXShaderCommentBlock::D3DXShaderCommentBlock(
    const std::uint32_t tag) noexcept : tag_(tag) {}

D3DXShaderCommentBlock::~D3DXShaderCommentBlock() {
    while (first_) {
        Fragment* const fragment = first_;
        first_ = fragment->next;
        if ((fragment->flags & 8u) != 0 || (fragment->flags & 1u) == 0)
            std::free(const_cast<void*>(fragment->data));
        std::free(fragment);
    }
}

std::int32_t D3DXShaderCommentBlock::Append(
    const void* const data, std::uint32_t size, std::uint32_t flags,
    std::uint32_t* const offset) {
    if (size != 0 && !data) return kFailure;
    if (size == UINT32_MAX) {
        if (!data) return kFailure;
        const std::size_t length = std::strlen(static_cast<const char*>(data));
        if (length >= UINT32_MAX) return kFailure;
        size = static_cast<std::uint32_t>(length + 1);
    }
    if ((flags & 1u) == 0) flags |= 2u;
    if ((flags & 2u) != 0) {
        for (Fragment* fragment = first_; fragment; fragment = fragment->next) {
            if ((fragment->flags & 2u) != 0 && fragment->size == size &&
                (size == 0 || std::memcmp(data, fragment->data, size) == 0)) {
                if (offset) *offset = fragment->offset;
                if ((flags & 9u) == 9u) std::free(const_cast<void*>(data));
                return 0;
            }
        }
    }
    auto* const fragment = static_cast<Fragment*>(std::malloc(sizeof(Fragment)));
    if (!fragment) return kNoMemory;
    const void* stored = data;
    if ((flags & 1u) == 0) {
        void* const copy = std::malloc(size == 0 ? 1u : size);
        if (!copy) {
            std::free(fragment);
            return kNoMemory;
        }
        if (size) std::memcpy(copy, data, size);
        stored = copy;
    }
    const std::uint32_t start = (flags & 4u) != 0 ? size_ : (size_ + 3u) & ~3u;
    if (start < size_ || size > UINT32_MAX - start) {
        if ((flags & 1u) == 0) std::free(const_cast<void*>(stored));
        std::free(fragment);
        return kFailure;
    }
    *fragment = {stored, size, flags, start, nullptr};
    *tail_ = fragment;
    tail_ = &fragment->next;
    size_ = start + size;
    if (offset) *offset = start;
    return 0;
}

std::uint32_t D3DXShaderCommentBlock::SizeInDwords() const noexcept {
    return ((size_ + 3u) >> 2) + 2u;
}

std::int32_t D3DXShaderCommentBlock::Serialize(
    const std::span<std::uint32_t> output,
    std::uint32_t word_count) const {
    if (size_ > (0x8000u - 2u) * 4u) return kFailure;
    const std::uint32_t minimum = SizeInDwords();
    if (word_count == UINT32_MAX) word_count = minimum;
    if (word_count < minimum || word_count > 0x8000u ||
        output.size() < word_count) return kFailure;
    output[0] = ((word_count - 1u) << 16) | 0xfffeu;
    output[1] = tag_;
    auto* const bytes = reinterpret_cast<std::uint8_t*>(output.data() + 2);
    std::uint32_t cursor = 0;
    for (const Fragment* fragment = first_; fragment; fragment = fragment->next) {
        if ((fragment->flags & 4u) == 0) {
            const std::uint32_t aligned = (cursor + 3u) & ~3u;
            std::memset(bytes + cursor, 0xabu, aligned - cursor);
            cursor = aligned;
        }
        if (fragment->size)
            std::memcpy(bytes + cursor, fragment->data, fragment->size);
        cursor += fragment->size;
    }
    std::memset(bytes + cursor, 0xabu, 4u * word_count - 8u - cursor);
    return 0;
}

}  // namespace pal4
