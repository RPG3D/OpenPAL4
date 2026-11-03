#include "pal4/base_object_activity.h"

namespace pal4 {

std::uint32_t BaseObjectActivity::Reset(
    BaseObjectState& state, IBaseObjectActivityRuntime& runtime) {
    runtime.ApplyMaterialTexture(state);
    runtime.CleanupObject(state);
    for (auto offset : {0x534u, 0x53cu}) {
        if (runtime.DialogNeedsUpdate(state, offset)) {
            runtime.UpdateDialog(state, offset);
        }
    }
    state.Write(0x530u, 0u);
    runtime.CleanupAnimationData(state, 0x170u);
    const auto game = runtime.GameInstance();
    runtime.CleanupClump(state, game + 0x1acu);
    const auto result = runtime.GetProperty(state);
    for (auto offset : {0x1f0u, 0x608u, 0x604u, 0x550u, 0x560u}) {
        state.Write(offset, 0u);
    }
    return result;
}

std::uint32_t BaseObjectActivity::Initialize(
    BaseObjectState& state, std::uint32_t kind,
    std::string_view model, std::uint32_t option,
    IBaseObjectActivityRuntime& runtime) {
    auto path = runtime.CreateResourcePath();
    try {
        path.append(model);
        path.push_back('\\');
        const auto game = runtime.GameInstance();
        if (runtime.LoadClump(state, kind, path, model,
                              game + 0x1acu, option, 0u, 1u)) {
            const auto clump = state.Read(0x14u);
            runtime.HideHookAtomics(clump);
            runtime.LoadAnimation(state, clump, path, model);
            state.Write(0x530u,
                        runtime.HookCount(clump, "hook_ef_bottom"));
        }
        const auto second_game = runtime.GameInstance();
        if (runtime.LoadShadowClump(state, second_game + 0x1acu)) {
            const auto shadow = state.Read(0x30u);
            runtime.HideHookAtomics(shadow);
            runtime.AttachAnimation(state, shadow);
        }
        for (auto offset : {0x608u, 0x604u, 0x550u, 0x560u}) {
            state.Write(offset, 0u);
        }
        runtime.ProcessAtomics(state);
        runtime.ProcessAtomics2(state);
    } catch (...) {
        (void)runtime.CleanupPath(path);
        throw;
    }
    return runtime.CleanupPath(path);
}

std::uint32_t BaseObjectActivity::SetStateFlag(
    BaseObjectState& state, std::uint32_t value) {
    state.Write(0x1f0u, value);
    return value;
}

std::uint32_t BaseObjectActivity::UpdateState(
    BaseObjectState& state, std::uint32_t address,
    std::uint32_t argument, IBaseObjectActivityRuntime& runtime) {
    const auto flag = state.Read(0x1f0u);
    if (flag == 0u) {
        return flag;
    }
    if (runtime.RenderClump(state, argument)) {
        runtime.ProcessAnimation(state, 0x170u, argument);
    }
    return runtime.UpdateCharacter(state, 0x564u, address);
}

std::uint32_t BaseObjectActivity::SetProperty(
    BaseObjectState& state, std::uint32_t address,
    std::uint32_t value, IBaseObjectActivityRuntime& runtime) {
    return runtime.SetPropertyVirtual(state, address, value, 0u);
}

}  // namespace pal4
