#include "pal4/game_arena_scene_runtime_loader.h"

#include "pal4/uppercase_string_pair.h"

#include <bit>
#include <cctype>

namespace pal4 {
namespace {

std::uint32_t Invoke(IGameArenaSceneRuntime& runtime,
                     const GameArenaRuntimeOperation operation,
                     const std::uint32_t first = 0u,
                     const std::uint32_t second = 0u,
                     const std::string_view first_text = {},
                     const std::string_view second_text = {}) {
    return runtime.Call(operation, first, second, first_text, second_text);
}

bool SameCStringIgnoreCase(std::string_view left, std::string_view right) {
    left = left.substr(0, left.find('\0'));
    right = right.substr(0, right.find('\0'));
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0u; i < left.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(left[i])) !=
            std::tolower(static_cast<unsigned char>(right[i]))) return false;
    }
    return true;
}

std::string CopyCString(std::string_view text) {
    return std::string(text.substr(0, text.find('\0')));
}

}  // namespace

bool GameArenaSceneRuntimeLoader::Load(
    const std::uint32_t world, const std::string_view scene_name,
    IGameArenaSceneRuntime& runtime) {
    const std::string selected = scene_name.empty()
                                     ? CopyCString(runtime.ReadText(world + 12u))
                                     : CopyCString(scene_name);
    runtime.SetSceneName(world + 224u, selected);
    const auto navigation = runtime.NavClump();
    if (navigation != 0u)
        runtime.BuildCollisionTree(world, runtime.NavClump());
    else
        runtime.ShowMissingNavmesh("Not Exist navMesh", "navMesh error");

    runtime.UpdateProgress(0.5f);
    static_cast<void>(Invoke(runtime, GameArenaRuntimeOperation::ProcessScriptStage));
    static_cast<void>(Invoke(runtime, GameArenaRuntimeOperation::ResetPlayerData));
    runtime.UpdateProgress(0.55000001f);
    const std::string world_name = CopyCString(runtime.ReadText(world + 8u));
    static_cast<void>(Invoke(runtime, GameArenaRuntimeOperation::InitializeNpc,
                             0u, 0u, world_name, selected));
    runtime.UpdateProgress(0.60000002f);
    static_cast<void>(Invoke(runtime,
                             GameArenaRuntimeOperation::LoadGameObjects,
                             0u, 0u, world_name, selected));
    runtime.UpdateProgress(0.64999998f);
    static_cast<void>(Invoke(runtime, GameArenaRuntimeOperation::InitializeMonster,
                             0u, 0u, world_name, selected));
    runtime.UpdateProgress(0.69999999f);
    runtime.LoadEffectRecords(runtime.EffectRoot(), world_name, selected);
    runtime.UpdateProgress(0.75f);
    static_cast<void>(Invoke(runtime, GameArenaRuntimeOperation::LoadPathway,
                             0u, 0u, world_name, selected));
    runtime.UpdateProgress(0.80000001f);

    static_cast<void>(Invoke(runtime, GameArenaRuntimeOperation::CleanupScripts));
    static_cast<void>(Invoke(runtime, GameArenaRuntimeOperation::DispatchUiEvent,
                             kUiEvent, 0u));
    static_cast<void>(Invoke(runtime, GameArenaRuntimeOperation::UpdateArenaCache,
                             0u, 0u, world_name, selected));
    static_cast<void>(Invoke(runtime, GameArenaRuntimeOperation::ForEachPlayer,
                             world, kPlayerCallback));
    static_cast<void>(Invoke(runtime, GameArenaRuntimeOperation::UpdatePlayerData));
    runtime.UpdateProgress(0.81999999f);
    static_cast<void>(Invoke(runtime, GameArenaRuntimeOperation::ForEachNpc,
                             world, kNpcCallback));
    runtime.UpdateProgress(0.83999997f);
    static_cast<void>(Invoke(runtime, GameArenaRuntimeOperation::ForEachAnimation,
                             world, kAnimationCallback));
    runtime.UpdateProgress(0.86000001f);
    static_cast<void>(Invoke(runtime, GameArenaRuntimeOperation::ForEachMonster,
                             world, kMonsterCallback));
    runtime.UpdateProgress(0.88f);
    static_cast<void>(Invoke(runtime, GameArenaRuntimeOperation::ForEachEffect,
                             world, kEffectCallback));
    runtime.UpdateProgress(0.89999998f);

    const UppercaseStringPair key(world_name, selected);
    const auto records = runtime.SceneRecords();
    const GameArenaSceneRecord* first_match = nullptr;
    for (const auto& record : records) {
        if (record.world == key.First() && record.scene == key.Second()) {
            first_match = &record;
            break;
        }
    }
    const auto actor = first_match == nullptr ? 0u : first_match->address;
    runtime.WriteWord(world + 264u, actor);
    if (actor != 0u) {
        static_cast<void>(Invoke(runtime,
                                 GameArenaRuntimeOperation::SetActorWord28,
                                 first_match->actor_type != 1u ? 1u : 0u));
        if (first_match->actor_type != 2u &&
            Invoke(runtime, GameArenaRuntimeOperation::PlayerZeroHasData) != 0u)
            static_cast<void>(Invoke(runtime,
                                     GameArenaRuntimeOperation::UpdatePlayerStates,
                                     world, 0u));
    }
    if (runtime.ReadWord(world + 132u) != 0u) {
        const auto distance = std::bit_cast<float>(runtime.ReadWord(world + 160u));
        static_cast<void>(Invoke(runtime,
                                 GameArenaRuntimeOperation::SetCameraFarClip,
                                 std::bit_cast<std::uint32_t>(distance + distance)));
    }
    runtime.UpdateProgress(0.94999999f);
    const bool script_name_differs = !SameCStringIgnoreCase(
        runtime.ReadText(world + 240u), world_name);
    static_cast<void>(Invoke(runtime,
                             GameArenaRuntimeOperation::ExecuteSceneScript,
                             world, script_name_differs ? 1u : 0u));
    runtime.UpdateProgress(0.98000002f);
    runtime.UpdateProgress(1.0f);
    static_cast<void>(Invoke(runtime,
                             GameArenaRuntimeOperation::SetBackgroundMusic,
                             0u, 0u, world_name, selected));
    return true;
}

}  // namespace pal4
