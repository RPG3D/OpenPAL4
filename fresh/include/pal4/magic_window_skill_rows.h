#pragma once

#include "pal4/magic_window_panel.h"
#include "pal4/magic_window_skill_catalog.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IMagicWindowSkillRowsRuntime {
public:
    virtual ~IMagicWindowSkillRowsRuntime() = default;
    virtual std::string ReadNameUtf8(std::uint32_t address) = 0;
    virtual std::int32_t CurrentValue(std::uint32_t player,
                                      const MagicDbSkillRecord& record) = 0;
    virtual bool SkillStateIsThree(const MagicDbSkillRecord& record) = 0;
    virtual std::int32_t TotalValue(std::uint32_t player,
                                    std::uint32_t category_key) = 0;
    virtual std::int32_t RemainingValue(std::uint32_t player) = 0;
    virtual std::string ActionText(std::uint32_t trigger_id,
                                   std::optional<std::int32_t> format_value) = 0;
    virtual void SetText(std::uint32_t window, std::string_view text) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetIcon(std::uint32_t window, std::string_view imageset,
                         std::string_view image) = 0;
    virtual void SetButtonImages(std::uint32_t window,
                                 const MagicSkillButtonImages& images) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    virtual void SetButtonRecordKey(std::uint32_t window,
                                    std::uint32_t key) = 0;
    virtual void UpdateDetails(std::uint32_t object) = 0;
};

class MagicWindowSkillRows final {
public:
    static void Bind(std::uint32_t object, std::uint32_t player,
                     std::uint32_t category_key,
                     const MagicWindowPanelState& panel,
                     std::span<const MagicDbSkillRecord> records,
                     const MagicSkillSelection& selection,
                     IMagicWindowSkillRowsRuntime& runtime);
};

}  // namespace pal4
