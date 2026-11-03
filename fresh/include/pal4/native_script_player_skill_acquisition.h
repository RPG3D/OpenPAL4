#pragma once

#include "pal4/script_player_skill_acquisition.h"
#include "pal4/native_magic_window_records.h"

#include <string>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeGameDatabase;
class IScriptInventoryChangeRuntime;
struct PlayerManagerObjectState;

// Borrows live player slots, their collection allocator and existing hint/audio
// services. Owns the original Stunt record projection and temporary hint text.
class NativeScriptPlayerSkillAcquisition final : public IScriptPlayerSkillAcquisitionRuntime {
public:
    NativeScriptPlayerSkillAcquisition(PlayerManagerObjectState&,
        NativeBaseObjectMemory&, const NativeGameDatabase&, IScriptInventoryChangeRuntime&);
    std::uint32_t PlayerManager() override;
    std::uint32_t PlayerByIndex(std::uint32_t, std::int32_t) override;
    std::uint32_t UiFrameManager() override;
    std::uint32_t CreateHintWindow(std::uint32_t) override;
    std::uint32_t GameDatabase() override;
    std::uint8_t HasSkill(std::uint32_t, std::uint32_t) override;
    void InsertSkill(std::uint32_t, std::uint32_t) override;
    std::uint32_t FindSkill(std::uint32_t, std::uint32_t) override;
    void InitializeText() override;
    std::string_view SkillName(std::uint32_t) override;
    std::string_view PlayerName(std::uint32_t) override;
    std::string_view GameString(std::uint32_t, std::uint32_t) override;
    void FormatText(std::string_view, std::string_view, std::string_view) override;
    std::string_view HintText() override;
    void ShowHint(std::uint32_t, std::string_view, float, float, float,
                  std::uint32_t, std::uint32_t) override;
    std::uint32_t AudioManager() override;
    void PlaySound(std::uint32_t, std::uint8_t, std::string_view, std::uint32_t) override;
    std::uint8_t CleanupText() noexcept override;
private:
    void ValidatePlayer(std::uint32_t) const;
    PlayerManagerObjectState& players_;
    NativeBaseObjectMemory& memory_;
    IScriptInventoryChangeRuntime& hints_;
    NativeMagicWindowRecords records_;
    std::string text_;
};
}  // namespace pal4
