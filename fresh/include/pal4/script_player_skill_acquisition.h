#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptPlayerSkillAcquisitionRuntime {
public:
    virtual ~IScriptPlayerSkillAcquisitionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateHintWindow(
        std::uint32_t frames) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint8_t HasSkill(
        std::uint32_t table, std::uint32_t id) = 0;
    virtual void InsertSkill(std::uint32_t collection,
                             std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t FindSkill(
        std::uint32_t table, std::uint32_t id) = 0;
    virtual void InitializeText() = 0;
    [[nodiscard]] virtual std::string_view SkillName(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::string_view PlayerName(
        std::uint32_t player) = 0;
    [[nodiscard]] virtual std::string_view GameString(
        std::uint32_t table, std::uint32_t id) = 0;
    virtual void FormatText(std::string_view format,
                            std::string_view player,
                            std::string_view skill) = 0;
    [[nodiscard]] virtual std::string_view HintText() = 0;
    virtual void ShowHint(std::uint32_t window,
                          std::string_view text, float x, float y,
                          float scale, std::uint32_t mode,
                          std::uint32_t enabled) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    virtual void PlaySound(std::uint32_t audio, std::uint8_t flag,
                           std::string_view path, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint8_t CleanupText() noexcept = 0;
};

class ScriptPlayerSkillAcquisition final {
public:
    // IDA 0x5d5f00, giPlayerAddSkill.
    [[nodiscard]] static std::uint8_t Add(
        std::int32_t player_index, std::uint32_t skill_id,
        std::uint8_t show_hint,
        IScriptPlayerSkillAcquisitionRuntime& runtime);
};

}  // namespace pal4
