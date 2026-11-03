#pragma once

#include <cstdint>

namespace pal4 {

enum class GameWorldFrameOperation : std::uint8_t {
    GetPlayerManager,
    ProcessPlayerUpdates,
    GetNpcManager,
    ProcessNpcAlternative,
    BeginFrame,
    RenderWorld,
    EndFrame,
    // IDA519CA9: PalGameObjectManager (scene GOB), not DialogManager.
    GetGameObjectManager,
    RenderGameObjects,
    UpdateAndRenderPlayers,
    ProcessNpcObjects,
    GetMonsterManager,
    ProcessMonsterObjects,
    GetPetManager,
    ProcessPetObjects,
    GetPalObjectManager,
    RenderPalObjects,
    // IDA519CF7: ArenaSkillManager, distinct from the world/arena owner.
    GetArenaSkillManager,
    RenderArenaSkills,
    ProcessScenePass,
    GetEffectManager,
    ProcessEffects,
    ProcessPlayerRelatedData,
};

class IGameWorldFrameRuntime {
public:
    virtual ~IGameWorldFrameRuntime() = default;
    // Native implementations may wrap the shared processor with lifetime and
    // exception cleanup. The default preserves existing runtime adapters.
    virtual void ProcessFrame(std::uint32_t world, std::uint32_t camera);
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        GameWorldFrameOperation operation, std::uint32_t first = 0u,
        std::uint32_t second = 0u) = 0;
};

class GameWorldFrameRuntimeStub final : public IGameWorldFrameRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Call(GameWorldFrameOperation,
                                     std::uint32_t, std::uint32_t) override {
        return 0u;
    }
};

class GameWorldFrameProcessor final {
public:
    // IDA 0x519c50.
    static void Process(std::uint32_t world, std::uint32_t frame,
                        IGameWorldFrameRuntime& runtime);
};

}  // namespace pal4
