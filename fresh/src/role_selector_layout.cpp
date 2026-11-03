#include "pal4/role_selector_layout.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

namespace {

bool Less(const std::uint32_t lhs, const std::uint32_t rhs,
          IRoleSelectorLayoutRuntime& runtime) {
    const auto rhs_key = runtime.GetSortKey(rhs);
    return runtime.GetSortKey(lhs) < rhs_key;
}

void SortPlayers(std::array<std::uint32_t, 4>& players,
                 const std::size_t count,
                 IRoleSelectorLayoutRuntime& runtime) {
    for (std::size_t index = 1u; index < count; ++index) {
        const auto value = players[index];
        if (Less(value, players[0], runtime)) {
            for (auto cursor = index; cursor != 0u; --cursor)
                players[cursor] = players[cursor - 1u];
            players[0] = value;
        } else {
            auto cursor = index;
            while (cursor > 1u &&
                   Less(value, players[cursor - 1u], runtime)) {
                players[cursor] = players[cursor - 1u];
                --cursor;
            }
            players[cursor] = value;
        }
    }
}

std::array<std::uint32_t, 2> ReadPosition(
    const std::uint32_t address,
    IRoleSelectorLayoutRuntime& runtime) {
    return {runtime.ReadWord(address),
            runtime.ReadWord(address + 4u)};
}

}  // namespace

std::uint32_t RoleSelectorLayout::Layout(
    const std::uint32_t object,
    IRoleSelectorLayoutRuntime& runtime) {
    runtime.WriteWord(object + 52u,
                      runtime.ReadWord(object + 48u));
    runtime.WriteWord(object + 60u, 0u);
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto child = object + 72u + index * 52u;
        runtime.SetVisible(runtime.ReadWord(child), false);
        runtime.SetVisible(runtime.ReadWord(child + 28u), false);
    }

    std::array<std::uint32_t, 4> players{};
    std::size_t player_count = 0u;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto player = runtime.GetPlayer(index);
        if (player != 0u &&
            runtime.ReadWord(player + 0xb08u) != 0u)
            players[player_count++] = player;
    }
    SortPlayers(players, player_count, runtime);

    std::array<std::uint32_t, 4> displayed{};
    for (std::size_t index = 0u; index < player_count; ++index) {
        const auto child = object + 72u +
            static_cast<std::uint32_t>(index) * 52u;
        const auto role = runtime.ReadWord(players[index] + 2632u);
        (void)RoleStaticElements::SetCharacter(
            child, static_cast<std::int32_t>(role), runtime);
        displayed[index] = child;
        runtime.AppendNavigation(object + 44u,
                                 runtime.ReadWord(child + 28u));
    }
    (void)runtime.ReleaseScratch(RoleSelectorScratch::Players);

    if (player_count != 0u) {
        const auto first_slot = player_count <= 2u ? 1u : 0u;
        std::uint32_t first_panel = 0u;
        std::uint32_t last_panel = 0u;
        for (std::size_t index = 0u; index < player_count; ++index) {
            const auto child = displayed[index];
            const auto panel = runtime.ReadWord(child);
            const auto button = runtime.ReadWord(child + 28u);
            const auto slot = first_slot +
                static_cast<std::uint32_t>(index);
            runtime.SetPosition(panel, ReadPosition(
                object + 280u + slot * 8u, runtime));
            runtime.SetPosition(button, ReadPosition(
                object + 312u + slot * 8u, runtime));
            runtime.SetVisible(panel, true);
            runtime.SetVisible(button, true);
            if (index == 0u) first_panel = panel;
            last_panel = panel;
        }
        const auto first_x = runtime.GetX(first_panel);
        const auto last_width = runtime.GetWidth(last_panel);
        const auto last_x = runtime.GetX(last_panel);
        const auto close = runtime.ReadWord(object + 344u);
        const auto close_width = runtime.GetWidth(close);
        const auto center =
            ((static_cast<double>(last_x) + last_width - first_x) *
                 0.5 + first_x) -
            static_cast<double>(close_width) * 0.5;
        runtime.SetX(close, static_cast<float>(center));
    }
    return runtime.ReleaseScratch(RoleSelectorScratch::Displayed);
}

}  // namespace pal4
