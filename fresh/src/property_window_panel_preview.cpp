#include "pal4/property_window_panel_preview.h"

#include <bit>

namespace pal4 {
namespace {

const std::array<float, 3> kEffectPosition{{
    std::bit_cast<float>(0xc26eae14u),
    std::bit_cast<float>(0x4348dc29u),
    std::bit_cast<float>(0x43952148u),
}};
const std::array<float, 3> kObjectPosition{{
    std::bit_cast<float>(0xc26eae14u),
    std::bit_cast<float>(0x439aee14u),
    std::bit_cast<float>(0x43952148u),
}};
constexpr std::array<float, 3> kEffectScale{{2.2f, 2.2f, 2.2f}};
constexpr std::array<float, 3> kObjectScale{{3.5f, 3.5f, 3.5f}};

}  // namespace

void PropertyWindowPanelPreview::SetItemPanel(
    PropertyWindowState& state, std::uint32_t visible,
    std::uint32_t event, IPropertyWindowPanelPreviewRuntime& runtime) {
    for (std::uint32_t offset = 0x8cu; offset <= 0x94u; offset += 4u)
        runtime.SetVisible(state.Read(offset), visible);
    if (static_cast<std::uint8_t>(visible) == 0u) {
        for (std::uint32_t offset = 0x40u; offset <= 0x54u; offset += 4u)
            runtime.SetSelected(state.Read(offset), false);
        const auto image = runtime.Image("zi0", "wupin");
        runtime.SetImage(state.Read(0x68u), image);
    }
    ToggleModel(state, static_cast<std::uint8_t>(visible), event, runtime);
}

void PropertyWindowPanelPreview::ToggleModel(
    PropertyWindowState& state, std::uint8_t visible,
    std::uint32_t event, IPropertyWindowPanelPreviewRuntime& runtime) {
    const auto collection = runtime.GameObjectCollection();
    if (visible != 0u) {
        if (runtime.FindObject(collection, state.Read(0x7cu)) != 0u) return;
        const auto effect = runtime.CreateEffect(
            "LH", kEffectPosition, 0xffffffffu, 0u);
        state.Write(0x84u, effect.first);
        state.Write(0x88u, effect.second);
        runtime.ResetTemporaryEffect();
        runtime.UpdateEffectProperties(effect, kEffectScale, 0u);
        runtime.MarkEffectForUpdate(effect);
        runtime.PrepareScripts();
        try {
            const auto object = runtime.CreateObject(
                collection, "ui\\model\\", "_ui_flower", 1u);
            if (object != 0u && runtime.ReadWord(object + 0x14u) != 0u) {
                state.Write(0x7cu, runtime.ReadWord(object + 0x170u));
                runtime.SetScale(object, kObjectScale);
                runtime.SetPosition(object, kObjectPosition);
            }
        } catch (...) {
            runtime.CleanupScripts();
            throw;
        }
        runtime.CleanupScripts();
    } else {
        runtime.RemoveObject(collection, state.Read(0x7cu));
        runtime.RemoveObject(collection, state.Read(0x80u));
        runtime.RemoveObject(collection, state.Read(0x78u));
        state.Write(0x7cu, 0u);
        state.Write(0x80u, 0u);
        state.Write(0x78u, 0u);
        runtime.HandleEffectEvent(
            {state.Read(0x84u), state.Read(0x88u)}, event, 0u);
    }
}

}  // namespace pal4
