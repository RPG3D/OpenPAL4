#pragma once

#include "pal4/combat_result_tree.h"
#include "pal4/combat_result_data_lifecycle.h"

#include <cstdint>

namespace pal4 {

class ICombatTreeCopyRuntime : public ICombatResultTreeRuntime {
public:
    virtual void DestroyTree(std::uint32_t tree) = 0;
};

class CombatTreeCopyConstructor final {
public:
    static constexpr std::uint32_t kSharedRefCountAddress =
        CombatResultDataLifecycle::kSharedRefCountAddress;
    static constexpr std::uint32_t kSharedNilAddress =
        CombatResultDataLifecycle::kSharedNilAddress;

    // IDA 0x573450.
    [[nodiscard]] static std::uint32_t Copy(
        std::uint32_t destination, std::uint32_t source,
        ICombatTreeCopyRuntime& runtime);
    // IDA 0x5735b0. Destination is uninitialized.
    [[nodiscard]] static std::uint32_t Assign(
        std::uint32_t destination, std::uint32_t source,
        ICombatTreeCopyRuntime& runtime);
    // IDA 0x573830. Copies nodes into an already initialized head.
    [[nodiscard]] static std::uint32_t CopyAssignedNodes(
        std::uint32_t destination, std::uint32_t source,
        ICombatTreeCopyRuntime& runtime);
    // IDA 0x573920. Returns the newly allocated circular head.
    [[nodiscard]] static std::uint32_t InitializeTree(
        std::uint32_t tree, ICombatTreeCopyRuntime& runtime);
};

}  // namespace pal4
