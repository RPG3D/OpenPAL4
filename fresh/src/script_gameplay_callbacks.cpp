#include "pal4/script_gameplay_callbacks.h"
#include "pal4/comparison_camera_mode_trace.h"

namespace pal4 {

std::uint8_t ScriptPostEventHandler::Handle(
    std::uint32_t mode, IScriptPostEventRuntime& runtime) {
    if (mode != 0u && mode != 1u)
        return static_cast<std::uint8_t>(mode - 1u);

    const auto game = runtime.GameInstance();
    const auto current_state = runtime.ReadWord(game + 0x18cu);
    if (current_state == 0u) return static_cast<std::uint8_t>(game);

    const auto game_again = runtime.GameInstance();
    const auto state_again = runtime.ReadWord(game_again + 0x18cu);
    const auto result = runtime.QueryCurrentState(state_again);
    if (result != 1u) return static_cast<std::uint8_t>(result);

    if (mode == 1u) {
        const auto state_game = runtime.GameInstance();
        return static_cast<std::uint8_t>(
            runtime.InitializeStateAndUi(state_game, 5u, 0u));
    }
    const auto trade = runtime.TradeSystem();
    return static_cast<std::uint8_t>(
        runtime.SetTradeItemInfo(trade, "0", 0u));
}

std::uint32_t MonsterPursuitController::StopOne(
    std::uint32_t monster, IMonsterPursuitCommandRuntime& runtime) {
    runtime.SetStopPursuitFlag(monster);
    return monster;
}

std::uint32_t MonsterPursuitController::StopAll(
    IMonsterPursuitCommandRuntime& runtime) {
    return runtime.Iterate(runtime.MonsterManager(),
                           &MonsterPursuitController::StopOne, 0u);
}

std::int32_t ScriptCameraModeHandler::SetMode(
    std::uint32_t mode, std::uint32_t sync_player_state,
    IScriptCameraModeRuntime& runtime) {
    auto& mode_trace=GetComparisonCameraModeTrace();
    mode_trace.Record(mode,false);
    const auto first_manager = runtime.CameraSubsystem();
    runtime.SetModeByIndex(first_manager, mode);
    const auto second_manager = runtime.CameraSubsystem();
    const auto current_mode = runtime.ReadWord(second_manager + 0xc0u);
    const auto sync = static_cast<std::uint8_t>(sync_player_state);
    if (current_mode == 0u || current_mode == 5u) {
        const auto manager = runtime.CameraSubsystem();
        runtime.SetLookAt(runtime.ActiveCameraObject(manager), 1u);
        runtime.FlushMainCameraTailYaw(runtime.ActorControl());
    } else {
        const auto manager = runtime.CameraSubsystem();
        const auto internal = runtime.ActiveCameraInternal(manager);
        runtime.SetPosition(internal, 0u, 0u, 0u, 1u);
        if (sync != 0u) {
            const auto next_manager = runtime.CameraSubsystem();
            runtime.SetLookAt(runtime.ActiveCameraObject(next_manager), 1u);
        }
    }
    const auto result=runtime.SetPlayerState(runtime.PlayerManager(), sync);
    mode_trace.Record(mode,true);
    return result;
}

}  // namespace pal4
