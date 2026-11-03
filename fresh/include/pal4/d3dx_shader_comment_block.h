#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace pal4 {

class D3DXShaderCommentBlock final {
public:
    explicit D3DXShaderCommentBlock(std::uint32_t tag) noexcept;
    ~D3DXShaderCommentBlock();

    D3DXShaderCommentBlock(const D3DXShaderCommentBlock&) = delete;
    D3DXShaderCommentBlock& operator=(const D3DXShaderCommentBlock&) = delete;

    // A source with flag 1 is borrowed; flag 8 transfers ownership to this block.
    [[nodiscard]] std::int32_t Append(const void* data, std::uint32_t size,
                                      std::uint32_t flags,
                                      std::uint32_t* offset = nullptr);
    [[nodiscard]] std::uint32_t SizeInDwords() const noexcept;
    [[nodiscard]] std::int32_t Serialize(
        std::span<std::uint32_t> output,
        std::uint32_t word_count = UINT32_MAX) const;

    [[nodiscard]] std::uint32_t Tag() const noexcept { return tag_; }
    [[nodiscard]] std::uint32_t DataSize() const noexcept { return size_; }

private:
    struct Fragment {
        const void* data;
        std::uint32_t size;
        std::uint32_t flags;
        std::uint32_t offset;
        Fragment* next;
    };

    std::uint32_t tag_;
    std::uint32_t size_ = 0;
    Fragment* first_ = nullptr;
    Fragment** tail_ = &first_;
};

}  // namespace pal4
