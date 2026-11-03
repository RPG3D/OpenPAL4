#pragma once

#include "pal4/role_state_window_player_info.h"

#include <cstdint>
#include <string>
#include <vector>

namespace pal4 {

class IRoleStateWindowUiRuntime {
public:
    virtual ~IRoleStateWindowUiRuntime() = default;
    virtual void SetParent(RoleStateWindowState& state, std::uint8_t showing,
                           std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void ProcessEvent(std::uint32_t event_tree,
                              std::uint32_t command,
                              std::uint32_t first,
                              std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t ActivePlayerCount(
        std::uint32_t player_manager) = 0;
    [[nodiscard]] virtual std::string ArenaString(
        std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveSceneProperty(
        const std::string& object, const std::string& arena) = 0;
    virtual void CleanupSceneProperty(std::uint32_t property) = 0;
    [[nodiscard]] virtual std::uint32_t ItemPropertyCollection() = 0;
    [[nodiscard]] virtual std::uint32_t ItemPropertyCount(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual bool MatchesSceneProperty(
        std::uint32_t scene_property, std::uint32_t entry) = 0;
    virtual void SortPlayers(std::vector<std::uint32_t>& players,
                             std::uint32_t comparator) = 0;
    virtual void InsertActive(std::uint32_t array,
                              std::uint32_t member) = 0;
    virtual void RemoveActive(std::uint32_t array,
                              std::uint32_t member) = 0;
    virtual void UpdateSystemDialogVisibility(
        std::uint32_t manager, std::uint32_t value) = 0;
};

class RoleStateWindowUiUpdate final {
public:
    static constexpr std::uint32_t kFrameToolEvent = 0x8b1394u;
    static constexpr std::uint32_t kGameInfoEvent = 0x8b13a0u;
    static constexpr std::uint32_t kHelpTipsEvent = 0x8b13a4u;
    static constexpr std::uint32_t kPlayerComparator = 0x4419d0u;

    static void Update(RoleStateWindowState& state,
                       std::uint32_t owner, std::uint8_t showing,
                       std::uint32_t parent,
                       IRoleStateWindowPlayerRuntime& player_runtime,
                       IRoleStateWindowUiRuntime& runtime);
};

}  // namespace pal4
