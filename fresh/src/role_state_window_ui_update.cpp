#include "pal4/role_state_window_ui_update.h"
#include "pal4/role_state_window_interaction.h"
#include "pal4/role_state_window_members.h"

namespace pal4 {

void RoleStateWindowUiUpdate::Update(
    RoleStateWindowState& state, const std::uint32_t owner,
    const std::uint8_t showing, const std::uint32_t parent,
    IRoleStateWindowPlayerRuntime& player_runtime,
    IRoleStateWindowUiRuntime& runtime) {
    runtime.SetParent(state, showing, parent);
    runtime.ProcessEvent(runtime.FrameManager() + 0x104u,
                         kFrameToolEvent, showing, 0u);
    runtime.ProcessEvent(runtime.FrameManager() + 0x104u,
                         kGameInfoEvent, showing, state.Read(36u));
    state.Write(592u, 0u);
    state.Write(596u, runtime.ActivePlayerCount(
        player_runtime.PlayerManager()));

    if (showing != 0u) {
        const auto arena = runtime.ArenaString(0xe0u);
        const auto object = runtime.ArenaString(8u);
        const auto scene_property = runtime.ResolveSceneProperty(object, arena);
        std::vector<std::uint32_t> matches;
        try {
            const auto collection = runtime.ItemPropertyCollection();
            if (runtime.ItemPropertyCount(collection + 0x5cu) != 0u) {
                const auto entries = player_runtime.ReadWord(collection + 0x60u);
                std::uint32_t i = 0u;
                do {
                    const auto entry = entries + i * 0x3cu;
                    if (runtime.MatchesSceneProperty(scene_property, entry))
                        matches.push_back(entry);
                    ++i;
                } while (i < runtime.ItemPropertyCount(collection + 0x5cu));
            }
        } catch (...) {
            runtime.CleanupSceneProperty(scene_property);
            throw;
        }
        runtime.CleanupSceneProperty(scene_property);
        if (!matches.empty()) state.Write(592u, matches.front());

        runtime.ProcessEvent(runtime.FrameManager() + 0x104u,
                             kHelpTipsEvent, 5u, 0u);
        std::vector<std::uint32_t> players;
        for (std::uint32_t i = 0u;
             i < RoleStateWindowMembers::kCount; ++i) {
            auto member = RoleStateWindowMembers::Load(state, i);
            RoleStateWindowInteraction::SetVisible(member, false,
                                                   player_runtime);
            RoleStateWindowMembers::Store(state, i, member);
            const auto player = player_runtime.PlayerByIndex(
                player_runtime.PlayerManager(), i);
            if (player != 0u &&
                player_runtime.ReadWord(player + 0xb08u) != 0u)
                players.push_back(player);
        }
        runtime.SortPlayers(players, kPlayerComparator);
        for (std::size_t i = 0; i < players.size(); ++i) {
            auto member = RoleStateWindowMembers::Load(
                state, static_cast<std::uint32_t>(i));
            RoleStateWindowPlayerInfo::Update(
                member,
                player_runtime.ReadWord(players[i] + 0xa48u),
                player_runtime);
            RoleStateWindowMembers::Store(
                state, static_cast<std::uint32_t>(i), member);
        }
    }

    state.Write(588u, 0u);
    const auto manager = runtime.FrameManager();
    if (showing != 0u) {
        runtime.InsertActive(manager + 0x134u, owner + 0x28u);
        runtime.UpdateSystemDialogVisibility(runtime.FrameManager(), 1u);
    } else {
        runtime.RemoveActive(manager + 0x134u, owner + 0x28u);
    }
}

}  // namespace pal4
