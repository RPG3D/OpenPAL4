#include "pal4/minimap_scene_ui.h"
#include "pal4/minimap_scene_ui_buttons.h"

#include <utility>

namespace pal4 {
namespace {

struct CallbackBinding {
    std::uint32_t window_offset;
    MinimapUiEvent event;
    std::uint32_t handler;
    bool inline_constructor;
};

constexpr std::array<CallbackBinding, 10> kSubscriptions{{
    {0x60u, MinimapUiEvent::Clicked, 0x4791f0u, true},
    {0x60u, MinimapUiEvent::MouseDown, 0x47a940u, true},
    {0x60u, MinimapUiEvent::MouseUp, 0x47a950u, false},
    {0x68u, MinimapUiEvent::Clicked, 0x47a790u, false},
    {0x68u, MinimapUiEvent::MouseDown, 0x47a940u, false},
    {0x68u, MinimapUiEvent::MouseUp, 0x47a950u, false},
    {0x64u, MinimapUiEvent::Clicked, 0x47a900u, false},
    {0x64u, MinimapUiEvent::MouseDown, 0x47a940u, false},
    {0x64u, MinimapUiEvent::MouseUp, 0x47a950u, false},
    {0x54u, MinimapUiEvent::MouseMove, 0x479210u, false},
}};

std::uint32_t CreateCallback(
    const std::uint32_t owner, const std::uint32_t handler,
    const bool inline_constructor, const bool command_callback,
    IMinimapSceneUiRuntime& runtime) {
    const auto storage = runtime.Allocate(0x18u);
    if (storage == 0u) return 0u;
    if (!inline_constructor) {
        return runtime.ConstructCallback(
            storage, handler, 0u, owner, command_callback);
    }
    runtime.WriteCallbackWord(storage + 8u, handler);
    runtime.WriteCallbackWord(storage + 12u, 0u);
    runtime.WriteCallbackWord(storage + 16u, owner);
    runtime.WriteCallbackWord(storage, MinimapSceneUi::kInlineCallbackVtable);
    return storage;
}

template <class Action>
void WithCallback(std::uint32_t callback,
                  IMinimapSceneUiRuntime& runtime, Action&& action) {
    try {
        action();
    } catch (...) {
        if (callback != 0u) runtime.ReleaseCallback(callback);
        throw;
    }
    if (callback != 0u) runtime.ReleaseCallback(callback);
}

void ReleaseText(MinimapSceneUiState& state,
                 const std::uint32_t data_offset,
                 IMinimapSceneUiRuntime& runtime) {
    const auto data = state.ReadWord(data_offset);
    if (data != 0u) {
        const auto ref_address = data - 1u;
        const auto references = runtime.ReadByte(ref_address);
        if (references == 0u || references == 0xffu) {
            runtime.FreeText(ref_address);
        } else {
            runtime.WriteByte(ref_address,
                              static_cast<std::uint8_t>(references - 1u));
        }
    }
    state.WriteWord(data_offset, 0u);
    state.WriteWord(data_offset + 4u, 0u);
    state.WriteWord(data_offset + 8u, 0u);
}

}  // namespace

std::uint32_t MinimapSceneUi::Construct(
    MinimapSceneUiState& state, const std::uint32_t address,
    const std::uint32_t layout_name,
    ISceneUiComponentRuntime& component_runtime,
    IMinimapSceneUiButtonRuntime& button_runtime,
    IMinimapSceneUiRuntime& runtime) {
    runtime.LoadLayout(state, address, layout_name, 0u, 0u);
    std::uint32_t stage = 0u;
    try {
        state.WriteWord(0x28u, kBaseComponentVtable);
        (void)SceneUiComponent::Construct(
            state.component, address + 0x2cu, "minimap", component_runtime);
        state.SyncComponent();
        stage = 1u;
        runtime.ProcessUtf8(address + 0x6cu);
        state.WriteWord(0xc0u, 0u);
        runtime.ResetText(address + 0xd0u, 0u);
        stage = 2u;
        if (runtime.AllocateText(address + 0xd0u, "SceneTimer", 1u)) {
            runtime.SetTextSize(address + 0xd0u, 10u);
        }
        runtime.ResetText(address + 0xe0u, 0u);
        stage = 4u;
        state.WriteWord(0u, kVtable);
        state.WriteWord(0x28u, kComponentVtable);
        (void)MinimapSceneUiButtons::Initialize(
            state, address, button_runtime);
        state.WriteByte(0xf7u, 1u);
        runtime.SetClippedByParent(state.ReadWord(0x24u), false);

        for (const auto& binding : kSubscriptions) {
            const auto callback = CreateCallback(
                address, binding.handler, binding.inline_constructor,
                false, runtime);
            WithCallback(callback, runtime, [&] {
                runtime.Subscribe(
                    state.ReadWord(binding.window_offset), binding.event,
                    callback);
            });
        }

        state.WriteWord(0xccu, 0u);
        state.WriteByte(0xf4u, 0u);
        state.WriteByte(0xf5u, 1u);
        for (const auto& command :
             std::array<std::pair<std::uint32_t, std::uint32_t>, 2>{{
                 {kUpdateSceneKeyCell, 0x47a970u},
                 {kJumpHintKeyCell, 0x47add0u}}}) {
            const auto callback = CreateCallback(
                address, command.second, false, true, runtime);
            WithCallback(callback, runtime, [&] {
                runtime.InsertCommand(
                    runtime.FrameManager() + 0x104u,
                    command.first, callback);
            });
        }
    } catch (...) {
        runtime.UnwindConstruction(address, stage);
        throw;
    }
    return address;
}

std::uint32_t MinimapSceneUi::DestroyUtf8Thunk(
    const std::uint32_t address, IMinimapSceneUiRuntime& runtime) {
    return runtime.DestroyUtf8(address);
}

std::uint32_t MinimapSceneUi::Destroy(
    MinimapSceneUiState& state, const std::uint32_t address,
    IMinimapSceneUiRuntime& runtime) {
    ReleaseText(state, 0xe4u, runtime);
    ReleaseText(state, 0xd4u, runtime);
    (void)runtime.DestroyUtf8(address + 0x6cu);
    (void)runtime.DestroyBase(address + 0x2cu);
    return runtime.DestroyBase(address);
}

}  // namespace pal4
