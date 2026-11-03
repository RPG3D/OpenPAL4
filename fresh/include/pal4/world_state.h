#pragma once

#include <cstdint>

namespace pal4 {

class IWorldStateFrameRuntime {
public:
    virtual ~IWorldStateFrameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    virtual void ProcessWorldFrame(std::uint32_t arena, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Minimap() = 0;
    virtual void DrawMinimap(std::uint32_t manager, std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t ScreenEffect() = 0;
    virtual void DrawScreenEffect(std::uint32_t effect, std::uint32_t frame) = 0;
    virtual void AdvanceScreenEffect(std::uint32_t effect, float delta) = 0;
    [[nodiscard]] virtual std::uint32_t BinkPlayer() = 0;
    virtual void RenderBink(std::uint32_t player, std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void RenderUI(std::uint32_t manager, std::uint32_t frame) = 0;
    virtual void UpdateUI(std::uint32_t manager, double delta) = 0;
    virtual void RenderScene(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    virtual void RenderConsole(std::uint32_t console, std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ProcessLog(std::uint32_t logger, double delta) = 0;
    virtual void ProcessTimedScripts(std::uint32_t executor, float delta) = 0;
    [[nodiscard]] virtual std::uint32_t CameraManager() = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCamera(std::uint32_t manager) = 0;
    virtual void UpdateCamera(std::uint32_t camera, float delta) = 0;
    virtual void UpdateWorld(std::uint32_t arena, float delta,
                             std::uint32_t mode, std::uint32_t flag) = 0;
};

class IWorldStateConstructionRuntime {
public:
    virtual ~IWorldStateConstructionRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class IWorldStateLifecycleRuntime : public IWorldStateConstructionRuntime {
public:
    ~IWorldStateLifecycleRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void SetUIFlag(std::uint32_t manager, std::uint32_t flag) = 0;
    virtual void SendUIEvent(std::uint32_t manager, std::uint32_t event,
                             std::uint32_t first, std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual std::uint32_t MonsterManager() = 0;
    virtual void SetMonsterFlags(std::uint32_t manager) = 0;
    virtual void ClearMonsterFlags(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint8_t CombatProperty519(std::uint32_t system) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    virtual void EnableInvincibility(std::uint32_t control, float seconds) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    virtual void ForEachStateMonster(std::uint32_t game) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t BackgroundMusicPlayer() = 0;
    virtual void SetBackgroundMusic(std::uint32_t player,
                                     std::uint32_t arena_player,
                                     std::uint32_t scene_name) = 0;
    virtual void ClearStateCollection(std::uint32_t game) = 0;
    [[nodiscard]] virtual std::uint32_t CameraManager() = 0;
    virtual void ActivateCamera(std::uint32_t manager,
                                std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t DefaultCameraName() = 0;
    [[nodiscard]] virtual std::uint32_t ActorMessageManager() = 0;
    virtual void ClearActorMessages(std::uint32_t manager) = 0;
    virtual void UpdateActorComponents(std::uint32_t control) = 0;
    virtual void ProcessActorInput(std::uint32_t control) = 0;
    virtual void ClearActorInputVector(std::uint32_t control) = 0;
    [[nodiscard]] virtual std::uint32_t InputManager() = 0;
    virtual void InitializeInput(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t Milliseconds() = 0;
    [[nodiscard]] virtual std::uint32_t GameState() = 0;
    [[nodiscard]] virtual std::uint32_t NpcManager() = 0;
    virtual void SetNpcGridUpdating(std::uint32_t grid, bool enabled) = 0;
    virtual void ProcessMonsterCollisions(std::uint32_t manager,
                                           float delta) = 0;
    [[nodiscard]] virtual std::uint32_t GameObjectManager() = 0;
    virtual void SetGameObjectsActive(std::uint32_t manager,
                                      bool active) = 0;
    [[nodiscard]] virtual std::uint32_t TemperatureManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindMonster(
        std::uint32_t manager, const char* name) = 0;
    [[nodiscard]] virtual std::uint32_t LeaveCharacter(
        std::uint32_t arena, std::uint32_t monster) = 0;
};

class WorldState final {
public:
    static constexpr std::uint32_t kVtable = 0x848604u;
    static constexpr std::uint32_t kUiActorEvent = 0x8a80a4u;
    static constexpr std::uint32_t kUiTemperatureEvent = 0x8b135cu;

    // IDA 0x58ef20/0x58ef30/0x58f0b0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IWorldStateConstructionRuntime& runtime);
    [[nodiscard]] static std::uint32_t EnterWrapper(
        std::uint32_t object, std::uint32_t unused,
        std::uint32_t transition, IWorldStateLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Enter(
        std::uint32_t transition, IWorldStateLifecycleRuntime& runtime);

    // IDA 0x58ef50/0x58efd0.
    static void Render(std::uint32_t frame, IWorldStateFrameRuntime& runtime);
    static void Update(float delta, std::uint32_t mode, std::uint32_t flag,
                       IWorldStateFrameRuntime& runtime);

    // IDA 0x58f060/0x58f080/0x58f2b0.
    [[nodiscard]] static std::uint32_t LeaveWrapper(
        std::uint32_t object, std::uint32_t unused,
        IWorldStateLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t RemoveMonsterByName(
        const char* name, IWorldStateLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Leave(
        IWorldStateLifecycleRuntime& runtime);

private:
    static void ResetTimestamps(IWorldStateLifecycleRuntime& runtime);
};

}  // namespace pal4
