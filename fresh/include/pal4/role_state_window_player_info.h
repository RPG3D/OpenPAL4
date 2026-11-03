#pragma once

#include "pal4/role_state_window_initializer.h"
#include "pal4/role_state_window_selection.h"

#include <cstdint>
#include <string>

namespace pal4 {

class IRoleStateWindowPlayerRuntime :
    public IRoleStateWindowSelectionRuntime {
public:
    virtual ~IRoleStateWindowPlayerRuntime() = default;
    using IRoleStateWindowSelectionRuntime::SetImage;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void PreparePlayerData(std::uint32_t player_data,
                                   std::uint32_t role_index) = 0;
    virtual void BeginTextBuffer() = 0;
    virtual void EndTextBuffer() = 0;
    virtual void SetImage(std::uint32_t window, std::uint32_t image) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::uint32_t text_address) = 0;
    [[nodiscard]] virtual std::int32_t ElementValue(
        std::uint32_t player_data, std::uint32_t element) = 0;
    [[nodiscard]] virtual std::int32_t BaseValue(
        std::uint32_t player_data, std::uint32_t field) = 0;
    [[nodiscard]] virtual std::int32_t CurrentValue(
        std::uint32_t player_data, std::uint32_t field) = 0;
    [[nodiscard]] virtual std::uint32_t TriggerText(
        std::uint32_t trigger) = 0;
    [[nodiscard]] virtual std::int32_t NextLevelThreshold(
        std::uint32_t role_id, std::int32_t next_level) = 0;
    virtual void SetText(std::uint32_t window,
                         const std::string& text) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    virtual void SetWidth(std::uint32_t window, float width) = 0;
};

class RoleStateWindowPlayerInfo final {
public:
    static constexpr std::uint32_t kEmptyTextAddress = 0x841964u;

    static void Update(RoleStateWindowState& state,
                       std::uint32_t player_index,
                       IRoleStateWindowPlayerRuntime& runtime);
};

}  // namespace pal4
