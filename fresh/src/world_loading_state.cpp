#include "pal4/world_loading_state.h"

#include <cstddef>
#include <initializer_list>

namespace pal4 {

std::uint32_t WorldLoadingState::Construct(
    std::uint32_t object, IWorldLoadingStateRuntime& runtime) {
    runtime.WriteWord(object, kBaseVtable);
    unsigned initialized = 0;
    try {
        for (const auto offset : {12u, 24u, 36u}) {
            runtime.InitializeString(object + offset);
            ++initialized;
        }
    } catch (...) {
        while (initialized != 0u) {
            --initialized;
            static_cast<void>(runtime.CleanupString(object + 12u + initialized * 12u));
        }
        runtime.DestroyBase(object);
        throw;
    }
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteWord(object + 48u, 0u);
    runtime.WriteWord(object + 52u, 0u);
    runtime.WriteWord(object + 4u, 0u);
    runtime.WriteWord(object, kVtable);
    return object;
}

std::uint32_t WorldLoadingState::Destroy(
    std::uint32_t object, IWorldLoadingStateRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    static_cast<void>(runtime.CleanupString(object + 36u));
    static_cast<void>(runtime.CleanupString(object + 24u));
    const auto result = runtime.CleanupString(object + 12u);
    runtime.WriteWord(object, kBaseVtable);
    return result;
}

std::uint32_t WorldLoadingState::LoadConfig(
    std::uint32_t object, std::uint32_t,
    std::uint32_t config, IWorldLoadingConfigRuntime& runtime) {
    if (config == 0u) return 0u;
    runtime.WriteWord(runtime.GameState() + 40u, 0u);
    runtime.ActivateCamera(runtime.CameraManager(), runtime.DefaultCameraName());
    runtime.SetLoadingWindowVisible(
        runtime.LoadingWindow(runtime.UIFrameManager()), true);

    const auto type = runtime.ReadType(config, "PALGAMEIV_LOADING_TYPE")
                          .value_or(0u);
    runtime.WriteWord(object + 8u, type);
    if (type == 0u) {
        constexpr std::string_view keys[] = {
            "PALGAMEIV_LOADING_ARENA", "PALGAMEIV_LOADING_AREA",
            "PALGAMEIV_LOADING_AREA_RES"};
        constexpr std::uint32_t offsets[] = {12u, 24u, 36u};
        for (std::size_t i = 0; i < 3u; ++i) {
            const auto value = runtime.ReadString(config, keys[i]);
            runtime.AssignString(object + offsets[i], value.value_or(""));
        }
        runtime.WriteWord(object + 52u,
            runtime.ReadTreeInteger(config, "PALGAMEIV_LOADING_AUTOREADY")
                   .value_or(0u));
    } else if (type == 1u) {
        runtime.WriteWord(object + 48u,
            runtime.ReadTreeInteger(config, "PALGAMEIV_LOADING_ARCHIVE")
                   .value_or(0u));
    }
    runtime.WriteWord(object + 4u, 0u);
    return 1u;
}

std::uint32_t WorldLoadingState::RenderUI(
    std::uint32_t frame, IWorldLoadingFlowRuntime& runtime) {
    return runtime.RenderUI(runtime.UIFrameManager(), frame);
}

std::uint32_t WorldLoadingState::Advance(
    std::uint32_t object, IWorldLoadingFlowRuntime& runtime) {
    const auto loaded = runtime.ReadWord(object + 4u);
    if (loaded != 0u) return loaded;

    const auto type = runtime.ReadWord(object + 8u);
    if (type == 1u) {
        runtime.LoadSave(runtime.ReadWord(object + 48u));
        const auto result = runtime.InitializeStateAndUI(runtime.GameInstance(), 1u);
        runtime.WriteWord(object + 4u, 1u);
        return result;
    }
    std::uint32_t result = type - 1u;
    if (type == 0u) {
        const auto scene = runtime.ReadWord(object + 12u);
        const auto area = runtime.ReadWord(object + 24u);
        runtime.LoadSceneResources(runtime.ArenaManager(), scene, area);
        const auto resources = runtime.ReadWord(object + 36u);
        runtime.LoadSceneRuntime(runtime.ArenaManager(), resources);
        runtime.InitializePlayerScene(runtime.ArenaManager());
        static_cast<void>(runtime.InitializeStateAndUI(runtime.GameInstance(), 1u));
        result = runtime.ReadWord(object + 52u);
        if (result != 0u) {
            result = runtime.ArenaManager();
            runtime.WriteWord(result + 196u, 1u);
        }
    }
    runtime.WriteWord(object + 4u, 1u);
    return result;
}

std::uint32_t WorldLoadingState::HideLoadingWindow(
    std::uint32_t, IWorldLoadingFlowRuntime& runtime) {
    runtime.SetLoadingWindowVisible(
        runtime.LoadingWindow(runtime.UIFrameManager()), false);
    return 1u;
}

std::uint32_t WorldLoadingState::HideLoadingWindow(
    std::uint32_t, IWorldLoadingConfigRuntime& runtime) {
    runtime.SetLoadingWindowVisible(
        runtime.LoadingWindow(runtime.UIFrameManager()), false);
    return 1u;
}

}  // namespace pal4
