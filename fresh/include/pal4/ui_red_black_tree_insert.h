#pragma once

#include "pal4/ui_red_black_tree_rotation.h"

#include <cstdint>

namespace pal4 {

class IUiRedBlackTreeInsertRuntime : public IUiRedBlackTreeLinkRuntime {
public:
    virtual ~IUiRedBlackTreeInsertRuntime() = default;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(std::uint32_t size) = 0;
    virtual void CopyText(std::uint32_t destination,
                          std::uint32_t source) = 0;
    [[nodiscard]] virtual std::int32_t CompareText(
        std::uint32_t first, std::uint32_t second) = 0;
};

class UiRedBlackTreeInsert final {
public:
    // IDA 0x4c0d30. Output receives the newly allocated node address.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t tree, std::uint32_t output,
        std::uint32_t child, std::uint32_t parent,
        std::uint32_t key, IUiRedBlackTreeInsertRuntime& runtime);
};

}  // namespace pal4
