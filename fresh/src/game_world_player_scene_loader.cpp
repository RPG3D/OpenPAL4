#include "pal4/game_world_player_scene_loader.h"

#include "pal4/uppercase_string_pair.h"

#include <string>

namespace pal4 {

bool GameWorldPlayerSceneLoader::Load(
    const std::uint32_t world, IGameWorldPlayerSceneRuntime& runtime) {
    runtime.ResetPlayerData();
    const auto player = runtime.CurrentPlayer();
    if (player == 0u) return false;

    runtime.SetPlayerStateFlag(player, 1u);
    runtime.SelectPlayerAnimation(player + 368u,
                                  kInitialAnimationStringSlot);
    runtime.AddPlayerToScene(world, player);
    runtime.SetActor(runtime.ActorControl(), player);
    runtime.WriteWord(runtime.ActorControl() + 272u, 0u);

    const std::string scene_name(runtime.ReadText(world + 12u));
    const std::string world_name(runtime.ReadText(world + 8u));
    const UppercaseStringPair key(world_name, scene_name);
    const auto records = runtime.SceneRecords();
    for (const auto& record : records) {
        if (record.world == key.First() && record.scene == key.Second()) {
            runtime.SetActorWord28(runtime.ActorControl(),
                                   record.actor_type != 1u);
            break;
        }
    }
    return true;
}

}  // namespace pal4
