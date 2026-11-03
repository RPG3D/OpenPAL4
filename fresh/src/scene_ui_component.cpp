#include "pal4/scene_ui_component.h"

namespace pal4 {
namespace {

struct ControlBinding {
    std::string_view suffix;
    std::uint32_t offset;
    bool inline_append;
};

constexpr std::array<ControlBinding, 6> kControls{{
    {"ImgMinimap", 0x28u, false},
    {"BtnMinimap", 0x34u, false},
    {"BtnMarker", 0x38u, false},
    {"StaticScene", 0x2cu, false},
    {"BtnSkill", 0x3cu, true},
    {"dituanniudi1", 0x30u, true},
}};

}  // namespace

std::uint32_t SceneUiComponent::Construct(
    SceneUiComponentState& state, const std::uint32_t address,
    const std::string_view layout, ISceneUiComponentRuntime& runtime) {
    runtime.LoadLayout(address, layout, 0u, 0u);
    state.Write(0u, kVtable);
    try {
        (void)InitializeControls(state, address, runtime);
    } catch (...) {
        runtime.DestroyBase(address);
        throw;
    }
    return address;
}

std::uint32_t SceneUiComponent::InitializeControls(
    SceneUiComponentState& state, const std::uint32_t address,
    ISceneUiComponentRuntime& runtime) {
    runtime.InitializeNameBuffer();
    try {
        for (const auto& binding : kControls) {
            runtime.AssignPrefix(address + kNameBufferOffset, 0u, 0xffffffffu);
            runtime.AppendSuffix(binding.suffix, binding.inline_append);
            const auto window = runtime.FindWindow(
                address, runtime.CurrentName());
            state.Write(binding.offset, window);
            if (binding.offset == 0x2cu) {
                runtime.SetHorizontalFormatting(window, 2u);
            }
        }
    } catch (...) {
        (void)runtime.CleanupNameBuffer();
        throw;
    }
    return runtime.CleanupNameBuffer();
}

void SceneUiComponent::SetParent(
    const std::uint32_t address, const std::uint8_t enabled,
    const std::uint32_t parent, ISceneUiComponentRuntime& runtime) {
    runtime.SetParent(address, enabled, parent);
}

}  // namespace pal4
