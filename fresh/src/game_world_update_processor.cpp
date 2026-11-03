#include "pal4/game_world_update_processor.h"

namespace pal4 {
namespace {

using Op = GameWorldUpdateOperation;

std::uint32_t Invoke(IGameWorldUpdateRuntime& runtime, const Op operation,
                     const std::uint32_t object = 0u,
                     const float delta = 0.0f,
                     const std::uint32_t argument = 0u) {
    return runtime.Call(operation, object, delta, argument);
}

}  // namespace

std::uint32_t GameWorldUpdateProcessor::Update(
    const std::uint32_t world, const float delta, const std::uint32_t mode,
    const std::uint32_t skip_world_objects,
    IGameWorldUpdateRuntime& runtime) {
    static_cast<void>(Invoke(runtime, Op::UpdateRenderParameters, 0u, delta));
    static_cast<void>(Invoke(runtime, Op::UpdateClumpMatrices, world + 320u,
                             delta));
    auto manager = Invoke(runtime, Op::GetTemperatureManager, 0u, delta);
    static_cast<void>(Invoke(runtime, Op::UpdateTemperature, manager));

    if (skip_world_objects == 0u) {
        auto context = runtime.ReadWord(world + 16u);
        manager = Invoke(runtime, Op::GetDialogManager, 0u, delta, context);
        static_cast<void>(Invoke(runtime, Op::UpdateDialogs, manager, delta,
                                 context));

        context = runtime.ReadWord(world + 16u);
        manager = Invoke(runtime, Op::GetPlayerManager, 0u, delta, context);
        static_cast<void>(Invoke(runtime, Op::ProcessPlayerElements, manager,
                                 delta, context));

        context = runtime.ReadWord(world + 16u);
        manager = Invoke(runtime, Op::GetNpcManager, 0u, delta, context);
        static_cast<void>(Invoke(runtime, Op::ProcessNpcObjects, manager,
                                 delta, context));

        context = runtime.ReadWord(world + 16u);
        manager = Invoke(runtime, Op::GetMonsterManager, 0u, delta, context);
        static_cast<void>(Invoke(runtime, Op::ProcessMonsterObjects, manager,
                                 delta, context));

        context = runtime.ReadWord(world + 16u);
        manager = Invoke(runtime, Op::GetPetManager);
        static_cast<void>(Invoke(runtime, Op::ProcessPetObjects, manager,
                                 delta, context));

        context = runtime.ReadWord(world + 16u);
        manager = Invoke(runtime, Op::GetObjectManager, 0u, delta, context);
        static_cast<void>(Invoke(runtime, Op::ProcessObjects, manager, delta,
                                 context));

        manager = Invoke(runtime, Op::GetArenaManager, 0u, delta);
        static_cast<void>(Invoke(runtime, Op::UpdateArenaDialogs, manager));
        manager = Invoke(runtime, Op::GetEffectManager, 0u, delta);
        static_cast<void>(Invoke(runtime, Op::UpdateEffects, manager));
        static_cast<void>(Invoke(runtime, Op::SyncWeatherCamera, world, delta,
                                 mode));
        static_cast<void>(Invoke(runtime, Op::UpdateArenaCache, world + 448u,
                                 delta));
    }

    static_cast<void>(Invoke(runtime, Op::AdvanceFades, world, delta));
    static_cast<void>(Invoke(runtime, Op::AdvanceFadeIn, world, delta));
    static_cast<void>(Invoke(runtime, Op::AdvanceSecondaryFade, world, delta));
    static_cast<void>(Invoke(runtime, Op::NoOpWorld, world, delta));
    static_cast<void>(Invoke(runtime, Op::NoOpWorld, world, delta));
    const auto music = Invoke(runtime, Op::GetBackgroundMusicPlayer);
    static_cast<void>(Invoke(runtime, Op::NoOpBackgroundMusic, music));
    return music;
}

}  // namespace pal4
