#include "pal4/role_state_window_player_info.h"
#include "pal4/role_state_window_level_text.h"
#include "pal4/role_state_window_interaction.h"

#include <array>
#include <cstring>
#include <initializer_list>
#include <string>

namespace pal4 {
namespace {

std::string ConvertOptionalText(IRoleStateWindowPlayerRuntime& runtime,
                                std::uint32_t address) {
    return runtime.ConvertToUtf8(address == 0u
                                     ? RoleStateWindowPlayerInfo::kEmptyTextAddress
                                     : address);
}

void SetDecimal(IRoleStateWindowPlayerRuntime& runtime,
                const RoleStateWindowState& state,
                std::uint32_t offset, std::int32_t value) {
    runtime.SetText(state.Read(offset), std::to_string(value));
}

float ScaleWidth(std::int32_t current, std::int32_t maximum,
                 std::uint32_t stored_bits) {
    float original;
    static_assert(sizeof(original) == sizeof(stored_bits));
    std::memcpy(&original, &stored_bits, sizeof(original));
    return static_cast<float>(
        static_cast<double>(static_cast<float>(current)) /
        static_cast<double>(static_cast<float>(maximum)) * original);
}

}  // namespace

void RoleStateWindowPlayerInfo::Update(
    RoleStateWindowState& state, const std::uint32_t player_index,
    IRoleStateWindowPlayerRuntime& runtime) {
    state.Write(0u, player_index);
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(),
                                              player_index);
    if (player == 0u) return;
    if (runtime.ReadWord(player + 0xb08u) == 0u) {
        RoleStateWindowInteraction::SetVisible(state, false, runtime);
        RoleStateWindowSelection::SetButtonImage(state, runtime);
        return;
    }

    const auto data = player + 0x654u;
    runtime.PreparePlayerData(data, state.Read(8u));
    runtime.BeginTextBuffer();
    try {
        const auto portrait_index = runtime.ReadWord(player + 0xa48u);
        runtime.SetImage(state.Read(12u),
                         state.Read(120u + 4u * portrait_index));
        runtime.SetText(state.Read(100u), ConvertOptionalText(
            runtime, runtime.ReadWord(player + 0x9fcu)));

        runtime.SetText(state.Read(20u),
                        RoleStateWindowLevelText::FormatText(
                            runtime.CurrentValue(data, 4u)));
        for (std::uint32_t element = 0u; element < 5u; ++element) {
            SetDecimal(runtime, state, 24u + element * 4u,
                       runtime.ElementValue(data, element));
        }

        const auto jing_base = runtime.BaseValue(data, 0u);
        const auto jing = runtime.CurrentValue(data, 7u);
        runtime.SetText(state.Read(48u),
                        std::to_string(jing) + "/" +
                            std::to_string(jing_base));
        const auto shen_base = runtime.BaseValue(data, 2u);
        const auto shen = runtime.CurrentValue(data, 9u);
        runtime.SetText(state.Read(52u),
                        std::to_string(shen) + "/" +
                            std::to_string(shen_base));
        runtime.SetText(state.Read(56u),
                        std::to_string(runtime.CurrentValue(data, 8u)) +
                            "/100");

        runtime.SetText(state.Read(80u), ConvertOptionalText(
            runtime, runtime.TriggerText(0x60u)));
        runtime.SetText(state.Read(88u), ConvertOptionalText(
            runtime, runtime.TriggerText(0x61u)));
        SetDecimal(runtime, state, 84u,
                   runtime.CurrentValue(data, 5u));

        if (runtime.CurrentValue(data, 4u) >= 99) {
            runtime.SetText(state.Read(92u), "--");
        } else {
            const auto role_id = runtime.ReadWord(player + 0xa48u);
            const auto next_level = runtime.CurrentValue(data, 4u) + 1;
            const auto threshold = runtime.NextLevelThreshold(
                role_id + 1001u, next_level);
            const auto remaining = threshold - runtime.CurrentValue(data, 5u);
            SetDecimal(runtime, state, 92u,
                       remaining < 0 ? 0 : remaining);
        }

        for (std::uint32_t offset : {80u, 84u, 88u, 92u}) {
            runtime.SetEnabled(state.Read(offset), false);
        }

        runtime.SetWidth(state.Read(60u), ScaleWidth(
            runtime.CurrentValue(data, 7u), runtime.BaseValue(data, 0u),
            state.Read(104u)));
        runtime.SetWidth(state.Read(64u), ScaleWidth(
            runtime.CurrentValue(data, 9u), runtime.BaseValue(data, 2u),
            state.Read(108u)));
        float rage_width;
        const auto rage_bits = state.Read(112u);
        std::memcpy(&rage_width, &rage_bits, sizeof(rage_width));
        runtime.SetWidth(state.Read(68u), static_cast<float>(
            static_cast<double>(runtime.CurrentValue(data, 8u)) *
            0.009999999776482582 * rage_width));
        RoleStateWindowInteraction::SetVisible(state, true, runtime);
    } catch (...) {
        runtime.EndTextBuffer();
        throw;
    }
    runtime.EndTextBuffer();
    RoleStateWindowSelection::SetButtonImage(state, runtime);
}

}  // namespace pal4
