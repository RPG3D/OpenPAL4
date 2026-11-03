#pragma once

#include "pal4/native_player_slot_collection_runtime.h"

namespace pal4 {
class NativePlayerSkillRuntime;
class NativePlayerScriptRefreshRuntime;

// Borrows the game host's skill and refresh services. Clear the skill binding
// before destroying it; constructing this adapter does not construct SkillSystem.
class NativePlayerCollectionScriptRuntime final : public INativePlayerCollectionScriptRuntime {
public:
    explicit NativePlayerCollectionScriptRuntime(NativePlayerScriptRefreshRuntime& refresh)
        : refresh_(refresh) {}
    void BindSkills(NativePlayerSkillRuntime* skills) noexcept { skills_=skills; }
    void HandleFade(std::uint32_t script,std::uint32_t skill,std::uint32_t context) override;
    void FinalizeScript(std::uint32_t script) override;
private:
    NativePlayerScriptRefreshRuntime& refresh_;
    NativePlayerSkillRuntime* skills_{};
};
}  // namespace pal4
