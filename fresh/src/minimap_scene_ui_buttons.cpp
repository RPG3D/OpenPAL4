#include "pal4/minimap_scene_ui_buttons.h"

#include <array>

namespace pal4 {
namespace {

struct ButtonBinding {
    std::uint32_t handler;
    MinimapUiEvent event;
};

constexpr std::array<ButtonBinding, 3> kSystemEvents{{
    {0x479030u, MinimapUiEvent::Clicked},
    {0x47a940u, MinimapUiEvent::MouseDown},
    {0x47a950u, MinimapUiEvent::MouseUp},
}};
constexpr std::array<ButtonBinding, 3> kHelpEvents{{
    {0x479090u, MinimapUiEvent::Clicked},
    {0x47a940u, MinimapUiEvent::MouseDown},
    {0x47a950u, MinimapUiEvent::MouseUp},
}};
constexpr std::array<ButtonBinding, 3> kRunEvents{{
    {0x4797c0u, MinimapUiEvent::Clicked},
    {0x47a940u, MinimapUiEvent::MouseDown},
    {0x47a950u, MinimapUiEvent::MouseUp},
}};

std::uint32_t FindNamedWindow(
    const std::uint32_t owner, const std::string_view suffix,
    const bool inline_append, IMinimapSceneUiButtonRuntime& runtime) {
    runtime.AssignPrefix(
        owner + MinimapSceneUiButtons::kNameBufferOffset, 0u, 0xffffffffu);
    runtime.AppendSuffix(suffix, inline_append);
    return runtime.FindWindow(owner, runtime.CurrentName());
}

void BindEvents(
    const std::uint32_t window, const std::uint32_t owner,
    const std::array<ButtonBinding, 3>& events,
    IMinimapSceneUiButtonRuntime& runtime) {
    for (const auto& binding : events) {
        const auto storage = runtime.AllocateCallback(0x18u);
        const auto callback = storage == 0u ? 0u :
            runtime.ConstructCallback(storage, binding.handler, 0u, owner);
        try {
            runtime.Subscribe(window, binding.event, callback);
        } catch (...) {
            if (callback != 0u) runtime.ReleaseCallback(callback);
            throw;
        }
        if (callback != 0u) runtime.ReleaseCallback(callback);
    }
}

}  // namespace

std::uint32_t MinimapSceneUiButtons::Initialize(
    MinimapSceneUiState& state, const std::uint32_t address,
    IMinimapSceneUiButtonRuntime& runtime) {
    runtime.InitializeNameBuffer();
    try {
        const auto system = FindNamedWindow(
            address, "BtnSystem", false, runtime);
        state.WriteWord(0xb4u, system);
        BindEvents(system, address, kSystemEvents, runtime);

        state.WriteWord(0xc0u, FindNamedWindow(
            address, "ImgJumpHint", false, runtime));
        const auto help = FindNamedWindow(
            address, "BtnHelp", false, runtime);
        BindEvents(help, address, kHelpEvents, runtime);

        const auto run = FindNamedWindow(
            address, "BtnRun", false, runtime);
        state.WriteWord(0xb8u, run);
        BindEvents(run, address, kRunEvents, runtime);

        state.WriteWord(0xc4u, FindNamedWindow(
            address, "ImgHpMask", false, runtime));
        state.WriteWord(0xc8u, FindNamedWindow(
            address, "ImgRageMask", false, runtime));
        state.WriteWord(0xbcu, FindNamedWindow(
            address, "StaticTimer", true, runtime));
    } catch (...) {
        (void)runtime.CleanupNameBuffer();
        throw;
    }
    return runtime.CleanupNameBuffer();
}

}  // namespace pal4
