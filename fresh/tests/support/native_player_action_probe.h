#pragma once
#include "pal4/native_script_player_action_runtime.h"
#include "pal4/native_player_animation_set.h"
#include "pal4/native_player_actor_assets.h"
#include "pal4/native_player_manager_session.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/game_object_embedded_animation.h"
#include <rphanim.h>
#include <array>
#include <stdexcept>
#include <string>

namespace pal4::test {
inline void VerifyNativeConcurrentPlayerActions(NativePlayerManagerSession& players,
    NativePlayerActorAssets& assets, NativePlayerAnimationSet& animations,
    NativePlayerEquipmentSession* equipment) {
    const auto check = [](bool ok, const char* message) {
        if (!ok) throw std::runtime_error(message);
    };
    const auto address = [&](unsigned i) { return reinterpret_cast<std::uint32_t>(&players.State().slots[i].base); };
    std::array<bool, 2> bound{};
    std::array<std::string, 2> previous;
    std::array<float, 2> times{};
    std::array<std::uint32_t, 2> restart{}, pause{};
    for (unsigned i = 0; i < 2; ++i) {
        bound[i] = animations.At(i).Bound();
        const auto& state = *reinterpret_cast<const GameObjectEmbeddedAnimationState*>(address(i) + 0x170u);
        restart[i] = state.words[17]; pause[i] = state.words[19];
        times[i] = assets.At(i).Hierarchy()->currentAnim->currentTime;
    }
    NativeAngelScriptEngine engine;
    NativeScriptFrameRunner runner(engine);
    NativeScriptPlayerActionRuntime actions(players, assets.At(0), runner.Context());
    actions.BindPlayers(assets, animations);
    actions.BindEquipment(equipment);
    for (unsigned i = 0; i < 2; ++i)
        if (bound[i]) previous[i] = actions.CurrentRecordName(address(i));
    constexpr char source[] = R"(
        void Lowercase() { giPlayerDoAction(0, "z03", 1, false); }
        void Start() { giPlayerDoAction(0, "C01", 0, false); giPlayerDoAction(1, "J03", -1, false); }
        void Replace() { giPlayerDoAction(1, "J03", -1, false); }
        void End() { giPlayerUnHoldAct(1); giPlayerEndAction(1); }
        void Finish() { giPlayerDoAction(0, "J13", 1, true); }
    )";
    check(engine.Raw()->AddScriptSection("parallel_actions", "parallel_actions.cs", source, sizeof(source)-1) >= 0 &&
          engine.Raw()->Build("parallel_actions") >= 0, "compile parallel action probe");
    NativeGameScriptServices services{}; services.player_action = &actions; services.action_hold = &actions;
    NativeGameScriptCallbacks callbacks(services); engine.SetCallbacks(&callbacks);
    const auto run = [&](const char* name) { runner.Queue("parallel_actions", name); return runner.Tick(0.0f, &actions); };
    check(run("Lowercase") == NativeScriptFrameStatus::finished &&
          actions.ActiveActionCount() == 1 &&
          actions.CurrentRecordName(address(0)) == "Z03",
          "actual lowercase script callback reaches the original uppercase action record");
    actions.ClearHandlers();
    check(run("Start") == NativeScriptFrameStatus::finished && actions.ActiveActionCount() == 2 &&
          !actions.BlocksScript(), "different player actions coexist without suspending script");
    check(run("Replace") == NativeScriptFrameStatus::finished && actions.ActiveActionCount() == 2,
          "same phase/name replaces one handler while preserving other player");
    for (unsigned frame = 0; frame < 600 && actions.ActiveActionCount() == 2; ++frame)
        (void)actions.Tick(0.05f);
    check(animations.At(1).HoldEventSeen() && actions.ActiveActionCount() == 1 &&
          actions.ActionName(address(1)) == "hold", "role1 J03 reaches actual AMF HOLD with role0 still pending");
    check(run("End") == NativeScriptFrameStatus::suspended && actions.BlocksScript(),
          "role1 end action suspends its actual script context");
    for (unsigned frame = 0; frame < 600 && runner.Status() == NativeScriptFrameStatus::suspended; ++frame)
        (void)runner.Tick(0.05f, &actions);
    check(runner.Status() == NativeScriptFrameStatus::finished && actions.ActiveActionCount() == 1 &&
          !actions.BlocksScript() && actions.CurrentRecordName(address(1)) == "C01",
          "role1 end resumes context and restores idle without retiring role0");
    check(run("Finish") == NativeScriptFrameStatus::suspended && actions.ActiveActionCount() == 1,
          "blocking replacement matches same name independently of wait mode");
    for (unsigned frame = 0; frame < 600 && runner.Status() == NativeScriptFrameStatus::suspended; ++frame)
        (void)runner.Tick(0.05f, &actions);
    check(runner.Status() == NativeScriptFrameStatus::finished && !actions.Waiting(),
          "replacement completes from actual clip and drains action queue");
    actions.ClearHandlers();
    for (unsigned i = 0; i < 2; ++i) {
        if (!bound[i]) animations.At(i).Release();
        else check(animations.At(i).Select(previous[i]) && animations.At(i).SetTime(times[i]),
                   "restore borrowed actor clip after parallel action probe");
        auto& state = *reinterpret_cast<GameObjectEmbeddedAnimationState*>(address(i) + 0x170u);
        state.words[17] = restart[i]; state.words[19] = pause[i];
    }
}
} // namespace pal4::test
