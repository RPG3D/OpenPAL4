#include "pal4/game_world_frame_processor.h"

namespace pal4 {
namespace {

using Op = GameWorldFrameOperation;

std::uint32_t Invoke(IGameWorldFrameRuntime& runtime, const Op operation,
                     const std::uint32_t first = 0u,
                     const std::uint32_t second = 0u) {
    return runtime.Call(operation, first, second);
}

void ProcessPlayerAndNpc(IGameWorldFrameRuntime& runtime,
                         const std::uint32_t frame) {
    const auto player_manager = Invoke(runtime, Op::GetPlayerManager, frame);
    static_cast<void>(Invoke(runtime, Op::ProcessPlayerUpdates,
                             player_manager, frame));
    const auto npc_manager = Invoke(runtime, Op::GetNpcManager, frame);
    static_cast<void>(Invoke(runtime, Op::ProcessNpcAlternative,
                             npc_manager, frame));
}

}  // namespace

void IGameWorldFrameRuntime::ProcessFrame(std::uint32_t world,std::uint32_t camera) {
    GameWorldFrameProcessor::Process(world,camera,*this);
}

void GameWorldFrameProcessor::Process(
    const std::uint32_t world, const std::uint32_t frame,
    IGameWorldFrameRuntime& runtime) {
    const auto scene_state = runtime.ReadWord(world + 504u);
    if (scene_state != 0u && runtime.ReadByte(scene_state + 1u) != 0u &&
        runtime.ReadWord(scene_state + 8u) == 3u)
        ProcessPlayerAndNpc(runtime, frame);

    static_cast<void>(Invoke(runtime, Op::BeginFrame, world));
    if (Invoke(runtime, Op::RenderWorld, world, frame) == 0u) {
        static_cast<void>(Invoke(runtime, Op::EndFrame, world));
        return;
    }

    auto manager = Invoke(runtime, Op::GetGameObjectManager, frame);
    static_cast<void>(Invoke(runtime, Op::RenderGameObjects, manager, frame));
    manager = Invoke(runtime, Op::GetPlayerManager, frame);
    static_cast<void>(Invoke(runtime, Op::UpdateAndRenderPlayers, manager,
                             frame));
    manager = Invoke(runtime, Op::GetNpcManager, frame);
    static_cast<void>(Invoke(runtime, Op::ProcessNpcObjects, manager, frame));
    manager = Invoke(runtime, Op::GetMonsterManager, frame);
    static_cast<void>(Invoke(runtime, Op::ProcessMonsterObjects, manager,
                             frame));
    manager = Invoke(runtime, Op::GetPetManager);
    static_cast<void>(Invoke(runtime, Op::ProcessPetObjects, manager, frame));
    manager = Invoke(runtime, Op::GetPalObjectManager, frame);
    static_cast<void>(Invoke(runtime, Op::RenderPalObjects, manager, frame));
    manager = Invoke(runtime, Op::GetArenaSkillManager, frame);
    static_cast<void>(Invoke(runtime, Op::RenderArenaSkills, manager, frame));
    static_cast<void>(Invoke(runtime, Op::ProcessScenePass, world, frame));
    static_cast<void>(Invoke(runtime, Op::EndFrame, world));

    ProcessPlayerAndNpc(runtime, frame);
    manager = Invoke(runtime, Op::GetEffectManager, frame);
    static_cast<void>(Invoke(runtime, Op::ProcessEffects, manager, frame));
    manager = Invoke(runtime, Op::GetPlayerManager, frame);
    static_cast<void>(Invoke(runtime, Op::ProcessPlayerRelatedData, manager,
                             frame));
}

}  // namespace pal4
