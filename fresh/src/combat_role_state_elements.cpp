#include "pal4/combat_role_state_window.h"
#include <array>
#include <bit>

namespace pal4 {
namespace {
constexpr std::array<std::string_view, 28> kStatusImages{{
    "ding", "feng", "jing", "mian", "luan", "kuang", "shui",
    "huo", "lei", "fen", "tu", "jin", "bi", "jie", "ying",
    "ning", "bi2", "sheng", "wu1", "fang1", "su1", "yun1",
    "ling1", "wu2", "fang2", "su2", "yun2", "ling2",
}};
constexpr std::array<std::string_view, 4> kPortraitImages{{
    "tianhezi", "lingshazi", "menlizi", "murongzi",
}};
std::uint32_t FindControl(const std::uint32_t window,
                          const std::string_view suffix,
                          const std::uint32_t role,
                          ICombatRoleStateElementsRuntime& runtime) {
    const auto name = std::string("CombatRoleState/") +
                      std::string(suffix) + std::to_string(role);
    return runtime.GetWindowByName(window, runtime.ConvertToUtf8(name));
}

void Subscribe(const std::uint32_t control,
               const CombatRoleStateClickEvent event,
               const std::uint32_t callback_function,
               const std::uint32_t window,
               ICombatRoleStateElementsRuntime& runtime) {
    const auto callback = runtime.Allocate(12u);
    if (callback) {
        runtime.WriteWord(callback, CombatRoleStateWindow::kClickCallbackVtable);
        runtime.WriteWord(callback + 4u, callback_function);
        runtime.WriteWord(callback + 8u, window);
    }
    const auto subscription = runtime.SubscribeClick(control, event, callback);
    if (subscription) runtime.ReleaseSubscription(subscription);
}

}
std::uint32_t CombatRoleStateWindow::InitializeControls(
    const std::uint32_t window, ICombatRoleStateElementsRuntime& runtime) {
    for (std::uint32_t index = 0u; index < 29u; ++index)
        runtime.WriteWord(window + 288u + 4u * index, 0u);
    for (std::uint32_t index = 0u; index < kStatusImages.size(); ++index)
        runtime.WriteWord(window + 292u + 4u * index,
                          runtime.GetImage("kuang0", kStatusImages[index]));
    for (std::uint32_t index = 0u; index < kPortraitImages.size(); ++index)
        runtime.WriteWord(window + 272u + 4u * index,
                          runtime.GetImage("zi0", kPortraitImages[index]));

    for (std::uint32_t role = 1u; role <= 3u; ++role) {
        const auto slot = role - 1u;
        const auto button = FindControl(window, "BtnSubstitution", role,
                                        runtime);
        runtime.WriteWord(window + 256u + 4u * slot, button);
        Subscribe(button, CombatRoleStateClickEvent::ButtonClicked,
                  kButtonCallback, window, runtime);

        runtime.WriteWord(window + 232u + 4u * slot,
                          FindControl(window, "ImageFrame", role, runtime));
        runtime.WriteWord(window + 244u + 4u * slot,
                          FindControl(window, "ImgName", role, runtime));
        constexpr std::array<std::string_view, 3> kTextNames{{
            "StaticRoleHp", "StaticRoleRage", "StaticRoleMp",
        }};
        constexpr std::array<std::uint32_t, 3> kTextOffsets{{
            148u, 160u, 172u,
        }};
        for (std::uint32_t index = 0u; index < kTextNames.size(); ++index) {
            const auto control = FindControl(window, kTextNames[index], role,
                                             runtime);
            runtime.WriteWord(window + kTextOffsets[index] + 4u * slot,
                              control);
            runtime.SetFont(control, "Figures5");
        }

        constexpr std::array<std::string_view, 3> kBarNames{{
            "ImageHp", "ImageMp", "ImageRage",
        }};
        constexpr std::array<std::uint32_t, 3> kBarOffsets{{
            196u, 208u, 220u,
        }};
        constexpr std::array<std::uint32_t, 3> kWidthOffsets{{
            112u, 124u, 136u,
        }};
        for (std::uint32_t index = 0u; index < kBarNames.size(); ++index) {
            const auto control = FindControl(window, kBarNames[index], role,
                                             runtime);
            runtime.WriteWord(window + kBarOffsets[index] + 4u * slot,
                              control);
            runtime.WriteWord(window + kWidthOffsets[index] + 4u * slot,
                              std::bit_cast<std::uint32_t>(
                                  runtime.GetWidth(control)));
        }

        const auto portrait = FindControl(window, "ImageRole", role, runtime);
        runtime.WriteWord(window + 184u + 4u * slot, portrait);
        runtime.WriteWord(portrait + 280u, slot);
        for (std::uint32_t status = 0u; status < 10u; ++status) {
            const auto name = std::string("CombatRoleState/ImgStatus") +
                              std::to_string(role) + std::to_string(status);
            const auto control = runtime.GetWindowByName(
                window, runtime.ConvertToUtf8(name));
            runtime.WriteWord(window + 404u + 40u * slot + 4u * status,
                              control);
        }
    }

    const auto panel = runtime.GetWindowByName(
        window, runtime.ConvertToUtf8("CombatRoleState/PanelRoleState"));
    runtime.WriteWord(window + 268u, panel);
    Subscribe(runtime.ReadWord(window + 196u),
              CombatRoleStateClickEvent::MouseClicked,
              kMouseCallback, window, runtime);
    return runtime.CleanupTemporaryString();
}

}
