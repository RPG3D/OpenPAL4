#include "pal4/role_state_window_initializer.h"

#include <array>

namespace pal4 {
namespace {

struct WindowBinding {
    std::string_view pattern;
    std::uint32_t offset;
    bool full_index;
};

constexpr std::array<WindowBinding, 21> kWindows{{
    {"roleStateWindow/RolePanel%d", 12u, true},
    {"roleStateWindow/RoleName%d", 100u, false},
    {"roleStateWindow/RoleLevel%d", 20u, true},
    {"roleStateWindow/RoleWater%d", 24u, true},
    {"roleStateWindow/RoleFire%d", 28u, false},
    {"roleStateWindow/RoleThunder%d", 32u, false},
    {"roleStateWindow/RoleWind%d", 36u, false},
    {"roleStateWindow/RoleDust%d", 40u, false},
    {"roleStateWindow/jing%d", 48u, false},
    {"roleStateWindow/shen%d", 52u, false},
    {"roleStateWindow/qi%d", 56u, false},
    {"roleStateWindow/ImgHp%d", 60u, false},
    {"roleStateWindow/ImgMp%d", 64u, false},
    {"roleStateWindow/ImgRage%d", 68u, false},
    {"roleStateWindow/xiakuang%d", 72u, false},
    {"roleStateWindow/RoleInfo%d_1", 80u, false},
    {"roleStateWindow/RoleInfo%d_2", 84u, false},
    {"roleStateWindow/RoleInfo%d_3", 88u, false},
    {"roleStateWindow/RoleInfo%d_4", 92u, false},
    {"roleStateWindow/baise%d", 96u, false},
    {"roleStateWindow/ImgCurtain%d", 76u, false},
}};

constexpr std::array<std::string_view, 4> kPortraits{{
    "tianhe1", "hanlinsha1", "liumenli1", "ziying1"
}};

}  // namespace

std::uint32_t RoleStateWindowInitializer::Initialize(
    RoleStateWindowState& state, const std::uint32_t owner,
    const std::uint32_t role_index, const std::uint32_t unused,
    const std::uint32_t argument4, IRoleStateWindowInitRuntime& runtime) {
    static_cast<void>(unused);
    state.Write(4u, 0u);
    state.Write(8u, role_index);
    state.Write(116u, argument4);

    for (std::size_t i = 0; i < kPortraits.size(); ++i) {
        state.Write(120u + static_cast<std::uint32_t>(i) * 4u,
                    runtime.FindImage("kuang0", kPortraits[i]));
    }

    runtime.InitializeNameBuffer();
    try {
        const auto display_index = role_index + 1u;
        const auto byte_index = static_cast<std::int32_t>(
            static_cast<std::int8_t>(display_index));
        for (std::size_t i = 0; i < kWindows.size(); ++i) {
            const auto& binding = kWindows[i];
            const auto name = runtime.FormatName(
                binding.pattern,
                binding.full_index
                    ? static_cast<std::int32_t>(display_index)
                    : byte_index);
            const auto window = runtime.FindWindow(name);
            state.Write(binding.offset, window);
            if (i == 0u || i == 19u || i == 20u)
                runtime.SetRoleIndex(window, display_index);
            if (i == 1u) runtime.SetEnabled(window, false);
            if (i == 2u) {
                runtime.SetFont(window, "Level_font");
                runtime.SetTextFormatting(window, 2u, 2u);
            }
        }

        for (std::uint32_t i = 0u; i < 3u; ++i) {
            state.Write(104u + i * 4u,
                        runtime.WidthBits(state.Read(60u + i * 4u)));
        }
        const auto panel = state.Read(12u);
        const auto background = state.Read(96u);
        runtime.Subscribe(panel, RoleMouseEvent::Move,
                          kMouseMoveHandler, owner, false);
        runtime.Subscribe(background, RoleMouseEvent::Move,
                          kMouseMoveHandler, owner, false);
        runtime.Subscribe(panel, RoleMouseEvent::Leave,
                          kMouseLeaveHandler, owner, false);
        runtime.Subscribe(background, RoleMouseEvent::Leave,
                          kMouseLeaveHandler, owner, true);
    } catch (...) {
        static_cast<void>(runtime.CleanupNameBuffer());
        throw;
    }
    return runtime.CleanupNameBuffer();
}

}  // namespace pal4
