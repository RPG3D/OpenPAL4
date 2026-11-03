#pragma once

#include "pal4/magic_window.h"
#include "pal4/magic_window_skill_catalog.h"
#include "pal4/magic_window_stunt_image_updater.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IMagicWindowCastRuntime {
public:
    virtual ~IMagicWindowCastRuntime() = default;
    virtual std::uint32_t GetPlayer(std::int32_t index) = 0;
    virtual std::int32_t PlayerValue(std::uint32_t player,
                                     std::uint32_t stat_index) = 0;
    virtual void AdjustPlayerValue(std::uint32_t player,
                                   std::uint32_t stat_index,
                                   std::int32_t delta) = 0;
    virtual std::int32_t SkillType(const MagicDbSkillRecord& skill) = 0;
    virtual std::int32_t StuntType(const MagicDbStuntRecord& stunt) = 0;
    virtual bool IsActive(std::uint32_t player) = 0;
    virtual void CastEffect(std::uint32_t caster, std::uint32_t effect_id,
                            std::uint32_t target) = 0;
    virtual void PlaySound(std::string_view name) = 0;
    virtual std::string_view TriggerText(std::uint32_t trigger_id) = 0;
    virtual void ShowInsufficientMessage(std::string_view text, float x,
                                         float y, std::uint32_t style,
                                         std::uint32_t flags) = 0;
};

class MagicWindowCastRuntimeStub : public IMagicWindowCastRuntime {
public:
    std::uint32_t GetPlayer(std::int32_t) override { return 0; }
    std::int32_t PlayerValue(std::uint32_t, std::uint32_t) override { return 0; }
    void AdjustPlayerValue(std::uint32_t, std::uint32_t,
                           std::int32_t) override {}
    std::int32_t SkillType(const MagicDbSkillRecord&) override { return 0; }
    std::int32_t StuntType(const MagicDbStuntRecord&) override { return 0; }
    bool IsActive(std::uint32_t) override { return false; }
    void CastEffect(std::uint32_t, std::uint32_t, std::uint32_t) override {}
    void PlaySound(std::string_view) override {}
    std::string_view TriggerText(std::uint32_t) override { return {}; }
    void ShowInsufficientMessage(std::string_view, float, float,
                                 std::uint32_t, std::uint32_t) override {}
};

class MagicWindowCaster final {
public:
    static bool Cast(const MagicWindowState& state,
                     std::int32_t target_character_index,
                     const MagicDbSkillRecord& skill,
                     IMagicWindowCastRuntime& runtime);
};

}  // namespace pal4
