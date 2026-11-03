#pragma once

#include <cstdint>

namespace pal4 {

enum class GameWorldUpdateOperation : std::uint8_t {
    UpdateRenderParameters,
    UpdateClumpMatrices,
    GetTemperatureManager,
    UpdateTemperature,
    GetDialogManager,
    UpdateDialogs,
    GetPlayerManager,
    ProcessPlayerElements,
    GetNpcManager,
    ProcessNpcObjects,
    GetMonsterManager,
    ProcessMonsterObjects,
    GetPetManager,
    ProcessPetObjects,
    GetObjectManager,
    ProcessObjects,
    GetArenaManager,
    UpdateArenaDialogs,
    GetEffectManager,
    UpdateEffects,
    SyncWeatherCamera,
    UpdateArenaCache,
    AdvanceFades,
    AdvanceFadeIn,
    AdvanceSecondaryFade,
    NoOpWorld,
    GetBackgroundMusicPlayer,
    NoOpBackgroundMusic,
};

class IGameWorldUpdateRuntime {
public:
    virtual ~IGameWorldUpdateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    // Calls that take double in the original ABI receive the source float;
    // the native adapter owns the float-to-double conversion.
    [[nodiscard]] virtual std::uint32_t Call(
        GameWorldUpdateOperation operation, std::uint32_t object = 0u,
        float delta = 0.0f, std::uint32_t argument = 0u) = 0;
};

class GameWorldUpdateRuntimeStub final : public IGameWorldUpdateRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Call(GameWorldUpdateOperation,
                                     std::uint32_t, float,
                                     std::uint32_t) override { return 0u; }
};

class GameWorldUpdateProcessor final {
public:
    // IDA 0x519d50. The last no-op preserves the BGM getter's EAX result.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t world, float delta, std::uint32_t mode,
        std::uint32_t skip_world_objects,
        IGameWorldUpdateRuntime& runtime);
};

}  // namespace pal4
