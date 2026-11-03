#include "pal4/strip_system_effect.h"

#include <array>
#include <string>

namespace pal4 {

bool StripSystemEffect::InitializeFromConfig(
    std::uint32_t effect, std::uint32_t config,
    std::uint32_t clump, std::uint32_t context,
    IStripSystemConfigRuntime& runtime) {
    runtime.Reset(effect);
    if (config == 0u || clump == 0u || context == 0u) return false;
    if (runtime.ReadInt(config, "Hook_Tag", 0) != 1) return false;

    const auto path = std::string{"gamedata\\effect\\tex\\"} +
        runtime.ReadText(config, "Attr_StripSystem_Texture");
    const bool alpha_blend = runtime.ReadInt(
        config, "Attr_StripSystem_AlphaBlend", 0) > 0;
    const auto lifetime = runtime.ReadFloat(
        config, "Attr_StripSystem_Life", 0.0f);
    constexpr auto kColorKeys = std::array<std::string_view, 4u>{
        "Attr_StripSystem_Color_Red",
        "Attr_StripSystem_Color_Green",
        "Attr_StripSystem_Color_Blue",
        "Attr_StripSystem_Color_Alpha"};
    std::uint32_t color = 0u;
    for (std::uint32_t index = 0u; index < kColorKeys.size(); ++index) {
        const auto channel = static_cast<std::uint8_t>(
            runtime.ReadInt(config, kColorKeys[index], 255));
        color |= static_cast<std::uint32_t>(channel) << (index * 8u);
    }

    auto texture = runtime.FindTexture(path);
    if (texture == 0u) texture = runtime.LoadTexture(path);
    if (texture == 0u) return false;

    const auto first = effect + 8u;
    runtime.WriteByte(first + 1u, alpha_blend ? 1u : 0u);
    runtime.WriteWord(first + 52u, color);
    runtime.SetDivision(first, 0u);
    if (!runtime.SetupBlock(first, texture, clump, lifetime, 1u)) {
        runtime.DestroyTexture(texture);
        return false;
    }

    const auto second = effect + 144u;
    runtime.WriteByte(second + 1u, alpha_blend ? 1u : 0u);
    runtime.WriteByte(effect + 6u, 0u);
    runtime.WriteWord(second + 52u, color);
    runtime.SetDivision(second, 0u);
    if (runtime.SetupBlock(second, texture, clump, lifetime, 0u))
        runtime.WriteByte(effect + 6u, 1u);
    runtime.DestroyTexture(texture);
    runtime.WriteByte(effect + 4u, 1u);
    return true;
}

}  // namespace pal4
