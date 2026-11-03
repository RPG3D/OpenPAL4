#include "pal4/game_world_weather_effects.h"

namespace pal4 {

std::uint32_t GameWorldWeatherEffects::Select(
    const std::uint32_t world, const std::int32_t weather_type,
    IGameWorldWeatherRuntime& runtime) {
    runtime.GetActorControl();
    if (weather_type < 0 ||
        static_cast<std::size_t>(weather_type) >= kEffectNames.size())
        return static_cast<std::uint32_t>(weather_type);

    for (const auto address : {world + 408u, world + 416u}) {
        if (runtime.NeedsEffectUpdate(address))
            static_cast<void>(runtime.UpdateEffectContent(address));
    }

    GameWorldEffectHandle temporary{};
    const auto created = runtime.CreateEffect(
        runtime.GetEffectManager(),
        kEffectNames[static_cast<std::size_t>(weather_type)],
        kPositionAddress, 1u, 0u, temporary);
    runtime.WriteWord(world + 416u, created.first);
    runtime.WriteWord(world + 420u, created.second);
    return runtime.ResetTemporary(temporary);
}

std::uint8_t GameWorldWeatherEffects::UpdateExisting(
    const std::uint32_t world, IGameWorldWeatherRuntime& runtime) {
    if (runtime.NeedsEffectUpdate(world + 408u))
        static_cast<void>(runtime.UpdateEffectContent(world + 408u));
    if (runtime.NeedsEffectUpdate(world + 416u))
        return runtime.UpdateEffectContent(world + 416u);
    return 0u;
}

std::uint8_t GameWorldWeatherEffects::SyncCameraPosition(
    const std::uint32_t world, const std::uint32_t,
    const std::uint32_t, IGameWorldWeatherRuntime& runtime) {
    const auto camera = runtime.ActiveCamera(runtime.GetCameraManager());
    if (camera == 0u) return 0u;

    const auto sync = [&](const std::uint32_t handle) {
        const std::array<std::uint32_t, 3> position = {
            runtime.ReadWord(camera + 116u),
            runtime.ReadWord(camera + 120u),
            runtime.ReadWord(camera + 124u),
        };
        return runtime.UpdateEffectProperties(handle, position, 0u);
    };
    if (runtime.NeedsEffectUpdate(world + 408u))
        static_cast<void>(sync(world + 408u));
    if (runtime.NeedsEffectUpdate(world + 416u))
        return sync(world + 416u);
    return 0u;
}

}  // namespace pal4
