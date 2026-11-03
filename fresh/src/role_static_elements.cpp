#include "pal4/role_static_elements.h"

#include <array>
#include <cstddef>
#include <string>

namespace pal4 {

std::uint32_t RoleStaticElements::Create(
    const std::uint32_t object, const std::uint32_t,
    const std::uint32_t prefix_data, const std::uint32_t,
    const std::uint32_t, const std::int32_t role_index,
    IRoleStaticElementsRuntime& runtime) {
    bool format_cleanup_attempted = false;
    bool prefix_release_attempted = false;
    try {
        const auto prefix = prefix_data == 0u
            ? std::string{} : runtime.ReadPrefix(prefix_data);
        const auto number = std::to_string(role_index);
        constexpr std::array<const char*, 9> suffixes{
            "StaticHp", "StaticMp", "StaticRage", "ImgHp", "ImgMp",
            "ImgRage", "BtnRole", "PanelRole", "ImgRole"};
        constexpr std::array<std::uint32_t, 9> offsets{
            4u, 8u, 12u, 16u, 20u, 24u, 28u, 0u, 32u};

        std::array<std::uint32_t, 9> windows{};
        for (std::size_t index = 0u; index < suffixes.size(); ++index) {
            const auto name = runtime.ConvertToUtf8(
                prefix + suffixes[index] + number);
            windows[index] = runtime.GetWindow(name);
            runtime.WriteWord(object + offsets[index], windows[index]);
            if (index == 6u)
                runtime.WriteWord(windows[index] + 280u,
                                  static_cast<std::uint32_t>(role_index));
        }
        for (std::size_t index = 0u; index < 3u; ++index)
            runtime.WriteFloat(object + 36u +
                                   static_cast<std::uint32_t>(index) * 4u,
                               runtime.GetWidth(windows[index + 3u]));

        format_cleanup_attempted = true;
        const auto result = runtime.CleanupFormatBuffer();
        if (prefix_data != 0u) {
            prefix_release_attempted = true;
            return runtime.ReleasePrefix(prefix_data);
        }
        return result;
    } catch (...) {
        if (!format_cleanup_attempted)
            (void)runtime.CleanupFormatBuffer();
        if (prefix_data != 0u && !prefix_release_attempted)
            (void)runtime.ReleasePrefix(prefix_data);
        throw;
    }
}

std::uint32_t RoleStaticElements::SetCharacter(
    const std::uint32_t object, const std::int32_t role_index,
    IRoleStaticCharacterRuntime& runtime) {
    if (role_index < 0 || role_index >= 4)
        return static_cast<std::uint32_t>(role_index);
    runtime.WriteWord(object + 48u,
                      static_cast<std::uint32_t>(role_index));
    const auto player = runtime.GetPlayer(
        static_cast<std::uint32_t>(role_index));
    if (player == 0u) return 0u;

    const auto hp_max = runtime.GetMaximum(player, 0u);
    const auto hp = runtime.GetValue(player, 7u);
    runtime.SetText(runtime.ReadWord(object + 4u),
                    std::to_string(hp) + "/" + std::to_string(hp_max));

    const auto mp_max = runtime.GetMaximum(player, 2u);
    const auto mp = runtime.GetValue(player, 9u);
    runtime.SetText(runtime.ReadWord(object + 8u),
                    std::to_string(mp) + "/" + std::to_string(mp_max));

    const auto rage = runtime.GetValue(player, 8u);
    runtime.SetText(runtime.ReadWord(object + 12u),
                    std::to_string(rage) + "/100");

    const auto hp_width_value = runtime.GetValue(player, 7u);
    const auto hp_width_max = runtime.GetMaximum(player, 0u);
    runtime.SetWidth(runtime.ReadWord(object + 16u),
                     static_cast<float>(hp_width_value) /
                         static_cast<float>(hp_width_max) *
                         runtime.ReadFloat(object + 36u));

    const auto mp_width_value = runtime.GetValue(player, 9u);
    const auto mp_width_max = runtime.GetMaximum(player, 2u);
    runtime.SetWidth(runtime.ReadWord(object + 20u),
                     static_cast<float>(mp_width_value) /
                         static_cast<float>(mp_width_max) *
                         runtime.ReadFloat(object + 40u));

    const auto rage_width_value = runtime.GetValue(player, 8u);
    runtime.SetWidth(runtime.ReadWord(object + 24u),
                     static_cast<float>(rage_width_value) *
                         0.0099999998f *
                         runtime.ReadFloat(object + 44u));

    constexpr std::array<const char*, 4> images{
        "tianhe2", "lingsha2", "mengli2", "ziying2"};
    runtime.SetImage(runtime.ReadWord(object + 32u),
                     "kuang0", images[static_cast<std::size_t>(role_index)]);
    return runtime.CleanupTextBuffer();
}

}  // namespace pal4
