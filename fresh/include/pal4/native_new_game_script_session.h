#pragma once

#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_frame_runner.h"
#include "pal4/native_timed_script_stack.h"
#include "pal4/native_scene_script_stack.h"
#include "pal4/script_combat_start.h"
#include "pal4/native_script_random_talk_pool.h"

#include <angelscript.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <unordered_set>

namespace pal4 {

class BackgroundMusicPlayer;
class IBackgroundMusicRuntime;
class IScriptNightTimeQueryRuntime;

// Owns original bytecode with SDK contexts for NewGame, events and scene stack entries.
// Services must outlive active frame ticks; release them before their owners.
class NativeNewGameScriptSession final : public IScriptCommandRuntime {
public:
    NativeNewGameScriptSession(const std::filesystem::path& game_root,
                               NativeGameScriptServices services,
                               std::int32_t* story_variable = nullptr,
                               NativeAngelScriptEngine::GlobalResolver globals = {});
    NativeNewGameScriptSession(const NativeNewGameScriptSession&) = delete;
    NativeNewGameScriptSession& operator=(const NativeNewGameScriptSession&) = delete;
    ~NativeNewGameScriptSession();

    NativeScriptRandomTalkPool& RandomTalkPool() noexcept { return random_talk_; }
    void Start();
    [[nodiscard]] ScriptCommandQueue& ScriptExecutorQueue() override { return combat_executor_.commands; }
    [[nodiscard]] ScriptCombatExecutor& CombatExecutor() noexcept { return combat_executor_; }
    // IDA60034E: clear random dialogue and combat additions, retain capacity and SDK stacks.
    void ResetSceneQueues();
    // Exactly once per game frame, before the primary/event runners. Selected
    // external/timed modules use original normal/IMM source-step scheduling.
    bool ProcessTimedScripts(float elapsed, bool actor_active,
        NativeGameScriptServices services,
        const std::function<void()>& clear_handlers,
        const std::function<bool()>& boundary);
    bool ProcessTimedScriptWaits(float elapsed, NativeGameScriptServices services,
        const std::function<bool()>& boundary, bool original_step = false);
    NativeScriptTimer& Timer() { return timed_scripts_.Timer(); }
    void DispatchUiScript(NativeGameScriptServices services,
        const std::function<void()>& clear_handlers,
        const std::function<void(NativeTimedScriptStack&)>& dispatch);
    // External scene callbacks share the original selected script stack.
    // Loading does not execute; pushing prepares an owned SDK context and
    // retires displaced handlers through the existing scoped dispatch.
    [[nodiscard]] bool HasExternalStackModule(std::string_view full_path) const;
    [[nodiscard]] bool HasExternalStackModulePath(std::string_view path) const {
        return timed_scripts_.HasModulePath(path);
    }
    void LoadExternalStackModule(std::string_view source, std::string_view full_path);
    void PushExternalStackModule(std::string_view full_path, std::string_view function,
        NativeGameScriptServices services, const std::function<void()>& clear_handlers);
    // IDA484560/600370: worldMap.csb init runs on a temporary SDK context,
    // synchronously, and removes that context/module before returning to UI.
    void RunWorldMapInitializer(NativeGameScriptServices services);
    // Staged original interpreter protocol for the shared WorldMap adapters.
    // Handles identify owned interpreter records; Add restores real CSB into
    // the SDK, and queued modules transfer to the selected stack on Push.
    [[nodiscard]] std::uint32_t CreateWorldMapInterpreter(std::string_view object,
        std::string_view entry);
    [[nodiscard]] bool AddWorldMapScriptModule(std::uint32_t interpreter);
    [[nodiscard]] std::string_view WorldMapInterpreterFunctionName(std::uint32_t interpreter);
    [[nodiscard]] std::string_view WorldMapInterpreterFullPath(std::uint32_t interpreter);
    // Native game wrapper identity, not the SDK engine's object layout. Shared
    // adapters forward owner+54 opaquely and must never dereference that token.
    [[nodiscard]] std::uint32_t WorldMapScriptOwnerAddress() const noexcept {
        return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(this));
    }
    void ExecuteWorldMapInterpreter(std::uint32_t interpreter, NativeGameScriptServices services);
    [[nodiscard]] bool PushWorldMapInterpreter(std::uint32_t interpreter,
        NativeGameScriptServices services, const std::function<void()>& clear_handlers);
    void ReleaseWorldMapInterpreter(std::uint32_t interpreter);
    // IDA484C10/7DEE60: arrival only prepares/pushes the destination function.
    // The selected stack executes it during the state's ordinary script phase.
    // An absent destination preserves existing handlers and returns false.
    [[nodiscard]] bool QueueWorldMapDestination(std::string_view function,
        NativeGameScriptServices services, const std::function<void()>& clear_handlers);
    std::uint64_t TimedScriptsStarted() const { return timed_scripts_.StartedCount(); }
    std::uint64_t TimedScriptsFinished() const { return timed_scripts_.FinishedCount(); }
    [[nodiscard]] bool TimedScriptsActive() const { return timed_scripts_.Active(); }
    // Across typed context owners, select the latest original7DEE60 push.
    // Includes a finished nested scene top until its normal pop restores parent.
    [[nodiscard]] bool SceneInitHasPriority() const noexcept;
    // External script operations must append to the selected context, including
    // already-closed position/rotation handlers (IDA5E0B10/7DF4B0).
    [[nodiscard]] IScriptCompletedHandlerQueue* SelectedCompletedHandlers() const noexcept;
    void SetTimedContextFactory(NativeTimedScriptStack::ContextFactory factory) {
        timed_scripts_.SetContextFactory(std::move(factory));
    }
    void SetCacheHandlerCleanup(std::function<void()> cleanup) {
        stack_handler_cleanup_ = cleanup;
        cache_context_.SetHandlerCleanup(std::move(cleanup));
    }
    void LoadSceneInit(std::string_view world, std::string_view scene);
    void StartSceneInit();
    bool QueueConcurrentSceneInit(std::string_view world,
                                  std::string_view scene);
    void BindConcurrentSceneServices(NativeGameScriptServices services);
    // Bind persistent scene owners after loading, before the queued function runs.
    void BindQueuedSceneServices(NativeGameScriptServices services);
    [[nodiscard]] asIScriptContext& ConcurrentSceneContext() const noexcept {
        return scene_stack_.Runner().Context();
    }
    [[nodiscard]] NativeScriptFrameStatus TickConcurrentSceneInit(
        float elapsed, INativeScriptFrameWaitRuntime* waits = nullptr);
    [[nodiscard]] NativeScriptFrameStatus TickConcurrentSceneOriginal(
        float elapsed, INativeScriptFrameWaitRuntime* waits,
        const std::function<bool()>& boundary);
    [[nodiscard]] NativeScriptFrameStatus TickConcurrentSceneUntilBoundary(
        float elapsed, INativeScriptFrameWaitRuntime* waits,
        const std::function<bool()>& boundary);
    void CancelConcurrentSceneInit();
    std::size_t ConcurrentSceneDepth() const noexcept { return scene_stack_.Depth(); }
    bool PopFinishedSceneInit() { return scene_stack_.PopFinished(); }
    void DetachConcurrentSceneServices();
    void RebindSuspendedSceneServices(NativeGameScriptServices services);
    // False means the full function was already active; no handler cleanup.
    bool QueueEventFunction(std::string_view world,
                            std::string_view function);
    void BindEventServices(NativeGameScriptServices services);
    [[nodiscard]] NativeScriptFrameStatus TickEvent(
        float elapsed, INativeScriptFrameWaitRuntime* waits = nullptr);
    [[nodiscard]] NativeScriptFrameStatus TickEventOriginal(
        float elapsed, INativeScriptFrameWaitRuntime* waits,
        const std::function<bool()>& boundary);
    [[nodiscard]] NativeScriptFrameStatus TickEventUntilBoundary(
        float elapsed, INativeScriptFrameWaitRuntime* waits,
        const std::function<bool()>& boundary);
    void DetachEventServices();
    void RebindSuspendedEventServices(NativeGameScriptServices services);
    void CancelQueuedEvent();
    [[nodiscard]] NativeScriptFrameStatus EventStatus() const noexcept {
        return event_runner_.Status();
    }
    [[nodiscard]] const std::string& EventError() const noexcept {
        return event_runner_.Error();
    }
    void RunMusicInitializer(std::string_view scene,
                             BackgroundMusicPlayer& player,
                             IBackgroundMusicRuntime& runtime,
                             IScriptNightTimeQueryRuntime& night_time,
                             const IScriptBgmAreaName& area);
    [[nodiscard]] NativeScriptFrameStatus ConcurrentSceneStatus() const noexcept {
        return scene_stack_.Runner().Status();
    }
    [[nodiscard]] const std::string& ConcurrentSceneError() const noexcept {
        return scene_stack_.Runner().Error();
    }
    [[nodiscard]] bool SceneInitLoaded() const noexcept {
        return !scene_module_.empty();
    }
    [[nodiscard]] std::int32_t GlobalStoryVariable() const noexcept {
        return engine_.GlobalStoryVariable();
    }
    void BindServices(NativeGameScriptServices services);
    // Preserve a suspended story while scene-owned services are replaced.
    void DetachSceneServices();
    void RebindSuspendedServices(NativeGameScriptServices services);
    void BindObjectVisibility(IScriptObjectVisibilityRuntime* runtime) noexcept;
    // Cancels queued/suspended execution before dropping callback references.
    void ReleaseServices();
    // IDA7DF330: stop execution, then reset/delete wait handlers. The caller
    // owns those native handlers and must provide their actual cleanup.
    // Loaded modules, callback services and externally bound global_mvar remain
    // intact. Owner teardown must explicitly detach/release borrowed services.
    void ClearExecutionStack(const std::function<void()>& clear_handlers);
    // IDA7DF3E0, distinct from7DF330: close waits while preserving execution.
    void ClearSceneWaitHandlers(const std::function<void()>& clear_handlers);
    [[nodiscard]] NativeScriptFrameStatus Tick(
        float elapsed, INativeScriptFrameWaitRuntime* waits = nullptr);
    [[nodiscard]] NativeScriptFrameStatus TickOriginal(
        float elapsed, INativeScriptFrameWaitRuntime* waits = nullptr,
        const std::function<bool()>& boundary = {});
    [[nodiscard]] NativeScriptFrameStatus TickUntilBoundary(
        float elapsed, INativeScriptFrameWaitRuntime* waits,
        const std::function<bool()>& boundary);
    [[nodiscard]] NativeScriptFrameStatus Status() const noexcept {
        return runner_.Status();
    }
    [[nodiscard]] const std::string& Error() const noexcept {
        return runner_.Error();
    }
    [[nodiscard]] std::uint32_t ImmediateMode() const noexcept {
        return callbacks_.ImmediateMode();
    }
    [[nodiscard]] asIScriptContext& Context() const noexcept {
        return runner_.Context();
    }
    [[nodiscard]] asIScriptContext& EventContext() const noexcept {
        return event_runner_.Context();
    }

private:
    static constexpr const char* kModule =
        "gamedata\\editData\\script\\script.csb";

    NativeScriptRandomTalkPool random_talk_;
    ScriptCombatExecutor combat_executor_;
    NativeAngelScriptEngine engine_;
    NativeScriptPushOrder push_order_;
    NativeScriptFrameRunner runner_;
    NativeSceneScriptStack scene_stack_;
    NativeScriptFrameRunner music_runner_;
    NativeScriptFrameRunner event_runner_;
    NativeGameScriptCallbacks callbacks_;
    NativeGameScriptCallbacks music_callbacks_;
    NativeGameScriptCallbacks event_callbacks_;
    std::filesystem::path game_root_;
    std::string loaded_scene_world_;
    std::unordered_set<std::string> loaded_world_modules_;
    std::string scene_module_;
    std::string scene_function_;
    bool music_module_loaded_ = false;
    NativeTimedScriptStack timed_scripts_;
    NativeScriptCacheContext cache_context_;
    std::function<void()> stack_handler_cleanup_;
    struct WorldMapInterpreter;
    std::vector<std::unique_ptr<WorldMapInterpreter>> world_map_interpreters_;
    WorldMapInterpreter& RequireWorldMapInterpreter(std::uint32_t interpreter);
    std::vector<NativeScriptCacheEntry> CollectCacheEntries();
    NativeGameScriptServices WithTimer(NativeGameScriptServices);
    [[nodiscard]] std::string LoadWorldModule(std::string_view world);
    [[nodiscard]] std::pair<std::string, std::string> LoadSceneModule(
        std::string_view world, std::string_view scene);
};

}  // namespace pal4
