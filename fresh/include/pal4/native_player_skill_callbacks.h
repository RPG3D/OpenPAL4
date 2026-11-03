#pragma once

#include "pal4/native_player_slot_process_runtime.h"
#include "pal4/player_manager_slot_script_data.h"

namespace pal4 {
class NativeSkillRecordCatalog;

// Borrowed owners must outlive this adapter and all callbacks through it.
// Only the reviewed Player script and its cleanup vtable are supported.
class NativePlayerSkillCallbacks final : public INativePlayerSkillCleanupCallbacks {
public:
    NativePlayerSkillCallbacks(NativeSkillRecordCatalog& catalog,
                              IPlayerManagerSlotScriptRuntime& refresh)
        : catalog_(catalog), refresh_(refresh) {}
    std::uint32_t ResolveSkillRecord(std::uint32_t record) override;
    // Notification entries are void in the original; their return is unused.
    std::uint32_t InvokeVirtual(std::uint32_t receiver, std::uint32_t offset,
                               std::span<const std::uint32_t> arguments) override;
private:
    NativeSkillRecordCatalog& catalog_;
    IPlayerManagerSlotScriptRuntime& refresh_;
};
}  // namespace pal4
