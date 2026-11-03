#pragma once

#include "pal4/native_base_object_memory.h"
#include "pal4/native_inventory_session.h"
#include "pal4/player_manager_slot_script_data.h"

namespace pal4 {
class NativePlayerSkillRuntime;

// Binds the rewritten player script refresh to its native x86 slot image.
class NativePlayerScriptRefreshRuntime final :
    public IPlayerManagerSlotScriptRuntime {
public:
    NativePlayerScriptRefreshRuntime(NativeBaseObjectMemory& memory,
                                     NativeInventorySession& inventory)
        : memory_(memory), inventory_(inventory) {}

    [[nodiscard]] std::uint32_t Refresh(std::uint32_t script);
    // Borrowed binding; clear before destroying the skill runtime.
    void BindSkills(NativePlayerSkillRuntime* skills) noexcept { skills_ = skills; }
    std::uint32_t GetValue(std::uint32_t object,
                           std::uint32_t index) override;
    std::uint32_t GetMaximum(std::uint32_t object,
                             std::uint32_t index) override;
    std::uint32_t SetValue(std::uint32_t object, std::uint32_t index,
                           std::uint32_t value) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    void ProcessInventory(std::uint32_t slots, std::uint32_t copy,
                          std::uint32_t original) override;
    void ProcessSkills(std::uint32_t object, std::uint32_t mode,
                       std::uint32_t marker) override;

private:
    NativeBaseObjectMemory& memory_;
    NativeInventorySession& inventory_;
    NativePlayerSkillRuntime* skills_{};
};

}  // namespace pal4
