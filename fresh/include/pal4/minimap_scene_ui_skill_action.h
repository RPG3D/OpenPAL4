#pragma once

#include <cstdint>

namespace pal4 {

class IMinimapSceneUiSkillRuntime {
public:
    virtual ~IMinimapSceneUiSkillRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t PlayerStat(
        std::uint32_t state, std::uint32_t index) = 0;
    virtual void AddPlayerStat(std::uint32_t state,
                               std::uint32_t index,
                               std::int32_t amount) = 0;
    [[nodiscard]] virtual std::uint32_t Arena() = 0;
    virtual void PlayerAction(std::uint32_t arena,
                              std::uint32_t action) = 0;
    virtual void PlayerActionSecondary(std::uint32_t arena,
                                       std::uint32_t action) = 0;
    [[nodiscard]] virtual std::uint32_t TriggerItemText(
        std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyString() = 0;
    [[nodiscard]] virtual std::uint32_t ConvertToUtf8(
        std::uint32_t text) = 0;
    virtual void ShowHint(std::uint32_t utf8_text,
                          std::uint32_t x_bits,
                          std::uint32_t y_bits,
                          std::uint32_t scale_bits,
                          std::uint32_t flag,
                          std::uint32_t enabled) = 0;
};

class MinimapSceneUiSkillAction final {
public:
    [[nodiscard]] static std::uint8_t Activate(
        std::uint32_t ignored_event,
        IMinimapSceneUiSkillRuntime& runtime);
    [[nodiscard]] static std::uint8_t UpdateState(
        std::uint32_t ignored_event,
        IMinimapSceneUiSkillRuntime& runtime);
};

}  // namespace pal4
