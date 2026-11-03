#include "pal4/magic_window_panel.h"

#include <array>
#include <string>

namespace pal4 {
namespace {

std::uint32_t Bind(std::string_view prefix, std::string_view suffix,
                   std::uint32_t offset, MagicWindowPanelState& state,
                   IMagicWindowPanelRuntime& runtime) {
    const auto window = runtime.FindWindow(std::string(prefix) + std::string(suffix));
    state.At(offset) = window;
    return window;
}

void Subscribe(std::uint32_t object, std::uint32_t window,
               MagicPanelEvent event, std::uint32_t handler,
               IMagicWindowPanelRuntime& runtime) {
    const auto callback = runtime.AllocateCallback(0x18u);
    if (callback != 0) {
        runtime.WriteWord(callback + 8u, handler);
        runtime.WriteWord(callback + 0xcu, 0u);
        runtime.WriteWord(callback + 0x10u, object);
        runtime.WriteWord(callback, MagicWindowPanel::kCallbackVtable);
    }
    const auto reference = runtime.Subscribe(window, event, callback);
    if (reference != 0) runtime.ReleaseReference(reference);
}

}  // namespace

void MagicWindowPanel::Initialize(std::uint32_t object, std::string_view prefix,
                                  MagicWindowPanelState& state,
                                  IMagicWindowPanelRuntime& runtime) {
    constexpr std::array<std::string_view, 6> categories{
        "BtnStunt", "BtnMagicWater", "BtnMagicFire", "BtnMagicThunder",
        "BtnMagicAir", "BtnMagicEarth"};
    for (std::uint32_t i = 0; i < categories.size(); ++i) {
        const auto button = Bind(prefix, categories[i], 0xa8u + 4u * i,
                                 state, runtime);
        runtime.SetUserIndex(button, i);
        Subscribe(object, button, MagicPanelEvent::Clicked,
                  kStuntClickHandler, runtime);
        runtime.SetupMouseHandlers(object + 0x28u, button);
    }

    for (std::uint32_t i = 0; i < 6u; ++i) {
        const auto suffix = std::to_string(i);
        const auto name = Bind(prefix, "StaticMagicName" + suffix,
                               0xc0u + 4u * i, state, runtime);
        runtime.SetUserIndex(name, i);
        runtime.SetAlwaysOnTop(name, true);
        Subscribe(object, name, MagicPanelEvent::MouseMove,
                  kMagicHoverHandler, runtime);
        runtime.SetButtonTextColorsBlack(name);
        runtime.ClearButtonField(name);

        const auto point = Bind(prefix, "BtnMagicPoint" + suffix,
                                0xf0u + 4u * i, state, runtime);
        runtime.SetUserIndex(point, i);
        runtime.SetHoverTextColorFromNormal(point);
        Subscribe(object, point, MagicPanelEvent::Clicked,
                  kMagicPointClickHandler, runtime);
        Bind(prefix, "Magicpic" + suffix, 0xd8u + 4u * i, state, runtime);
    }

    for (std::uint32_t i = 0; i < 7u; ++i) {
        const auto suffix = std::to_string(i);
        const auto button = Bind(prefix, "BtnStunt" + suffix,
                                 0x108u + 4u * i, state, runtime);
        runtime.SetUserIndex(button, i);
        Subscribe(object, button, MagicPanelEvent::MouseClick,
                  kStuntButtonClickHandler, runtime);
        Subscribe(object, button, MagicPanelEvent::MouseMove,
                  kStuntButtonHoverHandler, runtime);
        runtime.SetButtonField(button, object + 0x58u);
        Bind(prefix, "Stuntpic" + suffix, 0x124u + 4u * i, state, runtime);
    }

    constexpr std::array<std::pair<std::string_view, std::uint32_t>, 11> controls{{
        {"StaticRoleName", 0x7cu}, {"BtnLeft", 0x80u},
        {"BtnRight", 0x84u}, {"EditInfo", 0x88u},
        {"EditProp", 0x8cu}, {"PanelStunt", 0x164u},
        {"PanelMagic", 0x15cu}, {"ImageWuling", 0x160u},
        {"MagicInfoPanel", 0x158u}, {"EditMagicInfo", 0x168u},
        {"EditMagicProp", 0x16cu}}};
    for (const auto& [name, offset] : controls) {
        const auto window = Bind(prefix, name, offset, state, runtime);
        if (offset == 0x88u || offset == 0x8cu)
            runtime.SetAlwaysOnTop(window, true);
    }
    Subscribe(object, state.At(0x80u), MagicPanelEvent::Clicked,
              kPreviousCharacterHandler, runtime);
    Subscribe(object, state.At(0x84u), MagicPanelEvent::Clicked,
              kNextCharacterHandler, runtime);

    constexpr std::array<std::pair<std::string_view, std::uint32_t>, 12> labels{{
        {"zia", 0x90u}, {"zib", 0x94u}, {"zi1", 0x98u},
        {"dianshuzi", 0x9cu}, {"dianshuzi1", 0xa0u},
        {"dianshuzi12", 0xa4u}, {"zuozi", 0x154u},
        {"PicWater", 0x140u}, {"PicFire", 0x144u},
        {"PicThurder", 0x148u}, {"PicWind", 0x14cu},
        {"PicEarth", 0x150u}}};
    for (const auto& [name, offset] : labels) {
        const auto window = Bind(prefix, name, offset, state, runtime);
        if (offset == 0x90u || offset == 0x94u || offset == 0x98u)
            runtime.SetStaticTextColorsBlack(window);
    }
}

}  // namespace pal4
