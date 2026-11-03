#pragma once

#include "pal4/magic_window.h"
#include "pal4/magic_window_panel.h"
#include "pal4/magic_window_skill_catalog.h"

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

class IMagicWindowClickRuntime {
public:
    virtual ~IMagicWindowClickRuntime() = default;
    virtual std::uint32_t ButtonKey(std::uint32_t button) = 0;
    virtual std::optional<MagicDbSkillRecord> FindSkill(std::uint32_t key) = 0;
    virtual std::uint32_t GetPlayer(std::int32_t index) = 0;
    virtual std::int32_t SkillValue(std::uint32_t player,
                                    std::uint32_t category,
                                    std::uint32_t key) = 0;
    virtual std::int32_t RemainingValue(std::uint32_t player) = 0;
    virtual void AddSkillDelta(std::uint32_t player, std::uint32_t category,
                               std::uint32_t key, std::uint32_t amount) = 0;
    virtual void UpdatePlayerCollection(std::uint32_t player,
                                        std::uint32_t category,
                                        std::uint32_t amount) = 0;
    virtual std::uint32_t PlayerStatBase(std::uint32_t player,
                                         std::uint32_t category) = 0;
    virtual void SetPlayerStat(std::uint32_t player,
                               std::uint32_t category,
                               std::uint32_t value) = 0;
    virtual void InsertLearnedSkill(std::uint32_t player,
                                    std::uint32_t category,
                                    std::uint32_t key) = 0;
    virtual void ShowLearningHint(std::uint32_t name_address,
                                  std::uint32_t trigger_id,
                                  float x, float y) = 0;
    virtual void PlaySound(std::string_view path) = 0;
    virtual std::uint32_t AllocateCallback(std::uint32_t bytes) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void ShowCastDialog(bool visible, std::uint32_t& callback,
                                std::uint32_t skill_key) = 0;
    virtual void DeleteCallback(std::uint32_t callback,
                                 std::uint32_t flags) = 0;
    virtual void InitializeImages(std::uint32_t object) = 0;
    virtual void UpdateDetails(std::uint32_t object,
                                std::int32_t row) = 0;
};

class MagicWindowClickRuntimeStub : public IMagicWindowClickRuntime {
public:
    std::uint32_t ButtonKey(std::uint32_t) override { return 0; }
    std::optional<MagicDbSkillRecord> FindSkill(std::uint32_t) override {
        return std::nullopt;
    }
    std::uint32_t GetPlayer(std::int32_t) override { return 0; }
    std::int32_t SkillValue(std::uint32_t, std::uint32_t,
                            std::uint32_t) override { return 0; }
    std::int32_t RemainingValue(std::uint32_t) override { return 0; }
    void AddSkillDelta(std::uint32_t, std::uint32_t,
                       std::uint32_t, std::uint32_t) override {}
    void UpdatePlayerCollection(std::uint32_t, std::uint32_t,
                                std::uint32_t) override {}
    std::uint32_t PlayerStatBase(std::uint32_t, std::uint32_t) override { return 0; }
    void SetPlayerStat(std::uint32_t, std::uint32_t,
                       std::uint32_t) override {}
    void InsertLearnedSkill(std::uint32_t, std::uint32_t,
                            std::uint32_t) override {}
    void ShowLearningHint(std::uint32_t, std::uint32_t,
                          float, float) override {}
    void PlaySound(std::string_view) override {}
    std::uint32_t AllocateCallback(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void ShowCastDialog(bool, std::uint32_t&, std::uint32_t) override {}
    void DeleteCallback(std::uint32_t, std::uint32_t) override {}
    void InitializeImages(std::uint32_t) override {}
    void UpdateDetails(std::uint32_t, std::int32_t) override {}
};

class MagicWindowClickHandler final {
public:
    static constexpr std::uint32_t kCastCallbackHandler = 0x459fb0u;
    static constexpr std::uint32_t kCastCallbackVtable = 0x843914u;

    static bool Handle(std::uint32_t object, std::uint32_t clicked_button,
                       const MagicWindowState& state,
                       const MagicWindowPanelState& panel,
                       IMagicWindowClickRuntime& runtime);
};

}  // namespace pal4
