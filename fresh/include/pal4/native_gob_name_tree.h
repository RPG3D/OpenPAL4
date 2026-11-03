#pragma once

#include "pal4/native_base_object_memory.h"
#include "pal4/pal_game_object_tree.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>

namespace pal4 {

// Owns the original 16-byte name-tree holder and 36-byte STL nodes.
class NativeGobNameTree final : private IPalGameObjectNameInsertRuntime,
                                private IUiRedBlackTreeLookupRuntime,
                                private IUiRedBlackTreeStorageRuntime {
public:
    explicit NativeGobNameTree(NativeBaseObjectMemory& memory);
    NativeGobNameTree(NativeBaseObjectMemory& memory,
                      std::uint32_t embedded_tree);
    ~NativeGobNameTree() override;
    NativeGobNameTree(const NativeGobNameTree&) = delete;
    NativeGobNameTree& operator=(const NativeGobNameTree&) = delete;

    [[nodiscard]] bool Insert(std::string_view name, std::uint32_t object);
    [[nodiscard]] std::uint32_t Find(std::string_view name);
    [[nodiscard]] std::size_t Size() const;
    void Visit(const std::function<bool(std::uint32_t)>& visitor);
    // Clear borrowed object keys/nodes, retaining header and shared nil.
    void Clear();
    [[nodiscard]] std::uint32_t FirstNode();
    [[nodiscard]] std::uint32_t EndNode();
    [[nodiscard]] std::uint32_t NextNode(std::uint32_t node);
    [[nodiscard]] std::uint32_t Address() const noexcept { return tree_; }

private:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    [[nodiscard]] std::uint32_t SharedNil() override;
    void SetSharedNil(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t SharedNilUsers() override;
    void SetSharedNilUsers(std::uint32_t users) override;
    [[nodiscard]] std::uint32_t AllocateMemory(std::uint32_t size) override;
    void FreeMemory(std::uint32_t address) override;
    void CopyText(std::uint32_t destination, std::uint32_t source) override;
    [[nodiscard]] std::int32_t CompareText(std::uint32_t first,
                                           std::uint32_t second) override;
    [[nodiscard]] std::uint32_t EmptyText() override;
    [[nodiscard]] std::int32_t CompareRange(
        std::uint32_t node_text, std::uint32_t node_length,
        std::uint32_t text, std::uint32_t query_length) override;
    void CleanupData(std::uint32_t address) override;
    [[nodiscard]] std::string_view Text(std::uint32_t record);

    NativeBaseObjectMemory& memory_;
    std::uint32_t tree_ = 0u;
    bool owns_tree_ = true;
    std::uint32_t nil_ = 0u;
    std::uint32_t nil_users_ = 0u;
};

}  // namespace pal4
