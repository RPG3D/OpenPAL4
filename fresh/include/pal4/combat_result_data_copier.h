#pragma once

#include "pal4/combat_reward_record.h"
#include "pal4/combat_result_data_lifecycle.h"

#include <cstdint>

namespace pal4 {

class ICombatResultDataCopyRuntime : public ICombatRewardRecordRuntime {
public:
    [[nodiscard]] virtual std::uint32_t CreateNode(
        std::uint32_t parent, std::uint32_t color) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyAssignTree(std::uint32_t destination,
                                std::uint32_t source) = 0;
    virtual void DestroyTree(std::uint32_t tree) = 0;
    virtual void CleanupRecord(std::uint32_t record) = 0;
};

class CombatResultDataCopier final {
public:
    static constexpr std::uint32_t kSharedRefCountAddress =
        CombatResultDataLifecycle::kSharedRefCountAddress;
    static constexpr std::uint32_t kSharedNilAddress =
        CombatResultDataLifecycle::kSharedNilAddress;

    // IDA 0x573220.
    [[nodiscard]] static std::uint32_t Copy(
        std::uint32_t destination, std::uint32_t source,
        ICombatResultDataCopyRuntime& runtime);
};

}  // namespace pal4
