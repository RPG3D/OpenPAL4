#include "pal4/world_state.h"

namespace pal4 {

std::uint32_t WorldState::Construct(
    std::uint32_t object, IWorldStateConstructionRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    return object;
}

std::uint32_t WorldState::EnterWrapper(
    std::uint32_t, std::uint32_t, std::uint32_t transition,
    IWorldStateLifecycleRuntime& runtime) {
    return Enter(transition, runtime);
}

void WorldState::Render(std::uint32_t frame,
                        IWorldStateFrameRuntime& runtime) {
    if (runtime.ReadWord(runtime.ArenaManager() + 196u) != 1u ||
        runtime.ReadWord(frame) == 0u) return;
    runtime.ProcessWorldFrame(runtime.ArenaManager(), runtime.ReadWord(frame));
    runtime.DrawMinimap(runtime.Minimap(), frame);
    runtime.DrawScreenEffect(runtime.ScreenEffect(), frame);
    runtime.RenderBink(runtime.BinkPlayer(), frame);
    runtime.RenderUI(runtime.UIFrameManager(), frame);
    runtime.RenderScene(frame);
    runtime.RenderConsole(runtime.Console(), frame);
}

void WorldState::Update(float delta, std::uint32_t mode,
                        std::uint32_t flag, IWorldStateFrameRuntime& runtime) {
    runtime.ProcessLog(runtime.ScriptExecutor() + 84u,
                       static_cast<double>(delta));
    runtime.ProcessTimedScripts(runtime.ScriptExecutor(), delta);
    runtime.UpdateCamera(runtime.ActiveCamera(runtime.CameraManager()), delta);
    if (flag == 0u)
        runtime.AdvanceScreenEffect(runtime.ScreenEffect(), delta);
    if (runtime.ReadWord(runtime.ArenaManager() + 196u) == 1u) {
        runtime.UpdateWorld(runtime.ArenaManager(), delta, mode, flag);
        runtime.UpdateUI(runtime.UIFrameManager(), static_cast<double>(delta));
    }
}

std::uint32_t WorldState::LeaveWrapper(
    std::uint32_t, std::uint32_t, IWorldStateLifecycleRuntime& runtime) {
    return Leave(runtime);
}

std::uint32_t WorldState::RemoveMonsterByName(
    const char* name, IWorldStateLifecycleRuntime& runtime) {
    const auto monster = runtime.FindMonster(runtime.MonsterManager(), name);
    if (monster == 0u) return 0u;
    return runtime.LeaveCharacter(runtime.ArenaManager(), monster);
}

void WorldState::ResetTimestamps(IWorldStateLifecycleRuntime& runtime) {
    const auto time = runtime.Milliseconds();
    runtime.WriteWord(runtime.GameState() + 1388u, time);
    runtime.WriteWord(runtime.GameState() + 1396u, time);
    const auto copy = runtime.ReadWord(runtime.GameState() + 1396u);
    runtime.WriteWord(runtime.GameState() + 1396u, copy);
    runtime.WriteWord(runtime.GameState() + 1392u, copy);
}

std::uint32_t WorldState::Enter(
    std::uint32_t transition, IWorldStateLifecycleRuntime& runtime) {
    runtime.SetUIFlag(runtime.UIFrameManager(), 0u);
    if (transition == 3u) {
        const auto result = runtime.ReadWord(runtime.CombatSystem() + 504u);
        if (result == 0u || result == 2u) {
            runtime.SetMonsterFlags(runtime.MonsterManager());
            runtime.ClearMonsterFlags(runtime.MonsterManager());
            if (result == 2u &&
                runtime.CombatProperty519(runtime.CombatSystem()) == 0u)
                runtime.EnableInvincibility(runtime.ActorControl(), 5.0f);
        } else if (result == 1u) {
            runtime.ForEachStateMonster(runtime.GameInstance());
        }
        const auto scene_name = runtime.ReadWord(runtime.ArenaManager() + 224u);
        const auto player = runtime.ReadWord(runtime.ArenaManager() + 8u);
        runtime.SetBackgroundMusic(runtime.BackgroundMusicPlayer(), player,
                                   scene_name);
    }
    runtime.ClearStateCollection(runtime.GameInstance());
    runtime.ActivateCamera(runtime.CameraManager(), runtime.DefaultCameraName());
    if (runtime.ReadWord(runtime.ActorControl()) != 0u)
        runtime.SendUIEvent(runtime.UIFrameManager(), kUiActorEvent, 1u, 0u);
    runtime.ClearActorMessages(runtime.ActorMessageManager());
    runtime.UpdateActorComponents(runtime.ActorControl());
    runtime.ProcessActorInput(runtime.ActorControl());
    runtime.ClearActorInputVector(runtime.ActorControl());
    runtime.InitializeInput(runtime.InputManager());
    ResetTimestamps(runtime);
    runtime.SetNpcGridUpdating(runtime.NpcManager() + 56u, false);
    runtime.ProcessMonsterCollisions(runtime.MonsterManager(), 0.1f);
    runtime.SetGameObjectsActive(runtime.GameObjectManager(), true);
    if (runtime.ReadByte(runtime.TemperatureManager() + 40u) != 0u)
        runtime.SendUIEvent(runtime.UIFrameManager(), kUiTemperatureEvent,
                            1u, 0u);
    return 1u;
}

std::uint32_t WorldState::Leave(IWorldStateLifecycleRuntime& runtime) {
    runtime.SendUIEvent(runtime.UIFrameManager(), kUiActorEvent, 0u, 0u);
    runtime.ClearActorMessages(runtime.ActorMessageManager());
    runtime.SetNpcGridUpdating(runtime.NpcManager() + 56u, true);
    runtime.ProcessActorInput(runtime.ActorControl());
    runtime.ClearActorInputVector(runtime.ActorControl());
    runtime.InitializeInput(runtime.InputManager());
    runtime.SetGameObjectsActive(runtime.GameObjectManager(), false);
    ResetTimestamps(runtime);
    runtime.SendUIEvent(runtime.UIFrameManager(), kUiTemperatureEvent, 0u, 0u);
    return 1u;
}

}  // namespace pal4
