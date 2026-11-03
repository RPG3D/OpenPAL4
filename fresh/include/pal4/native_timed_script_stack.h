#pragma once
#include <filesystem>
#include <functional>
#include <memory>

#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_timer.h"
#include "pal4/native_script_frame_wait_runtime.h"
#include "pal4/native_script_cache_context.h"
#include "pal4/native_script_push_order.h"
namespace pal4 {
class NativeAngelScriptEngine;
class NativeUiCountdownScriptRuntime;
class NativePropertyWindowScriptRuntime;
// Own context-dependent waits/services while the borrowed SDK context is alive.
// Services may override the entry's giWait; Waits must then process that wait.
class INativeTimedScriptContext {
public:
    virtual ~INativeTimedScriptContext() = default;
    virtual NativeGameScriptServices BindServices(NativeGameScriptServices shared) = 0;
    virtual INativeScriptFrameWaitRuntime* Waits() noexcept = 0;
    // Video state already advances the shared screen/movie before script waits.
    virtual INativeScriptFrameWaitRuntime* HandlerWaits() noexcept { return Waits(); }
    virtual void ClearHandlers() = 0;
    virtual IScriptCompletedHandlerQueue* CompletedHandlers() noexcept { return nullptr; }
};
class NativeTimedScriptStack final {
   public:
    using ModuleLoader =
        std::function<void(std::string_view source, std::string_view module)>;
    using ContextFactory = std::function<std::unique_ptr<INativeTimedScriptContext>(asIScriptContext&)>;
    NativeTimedScriptStack(NativeAngelScriptEngine&, const std::filesystem::path&,
                           ModuleLoader loader = {}, NativeScriptPushOrder* push_order = nullptr);
    // Evaluated after periodic/UI pushes, before advancing the selected SDK
    // context. Returning false lets a newer context owned elsewhere run.
    void SetSelectionPolicy(std::function<bool(std::uint64_t)> policy);
    std::uint64_t SelectedPushOrder() const noexcept;
    void SetBeforePush(std::function<void()>);
    ~NativeTimedScriptStack();
    NativeScriptTimer& Timer();
    void BindServices(NativeGameScriptServices);
    // Configure while no timed context is active. The factory's captured owners
    // must outlive this stack. Detached bindings rebuild on the next BindServices.
    void SetContextFactory(ContextFactory);
    void DetachContextServices();
    // IDA7DEE60: a UI countdown may push while an SDK callback is active.
    // Prepare first; request Suspend, then retire displaced handlers after the
    // native callback unwinds and before child execution. Captured handler owners
    // must outlive the next Process/Clear/ClearHandlers that drains this work.
    // Dispatch is synchronous; queued script execution stays deferred.
    void DispatchUiScript(NativeGameScriptServices,
        const std::function<void()>& clear_handlers,
        const std::function<void(NativeTimedScriptStack&)>& dispatch);
    // Returns whether a timed context consumed this frame. Call once per game
    // frame, before advancing the underlying story/event context.
    bool Process(float elapsed, bool actor_active, bool script_busy,
                 std::string_view current_world, bool step = false,
                 const std::function<bool()>& boundary = {});
    // State12 processes only the selected context/waits; it does not advance
    // the ordinary TimeScript scheduling clock or start new periodic requests.
    bool ProcessSelected(float elapsed, bool step = false,
                         const std::function<bool()>& boundary = {});
    bool Active() const;
    // Borrow only for an immediate main-thread operation; a push/pop or service
    // detach can retire this queue's context owner.
    [[nodiscard]] IScriptCompletedHandlerQueue* SelectedCompletedHandlers() const noexcept;
    // Active selected-stack entries associated with an original module path.
    bool HasModulePath(std::string_view path) const;
    std::size_t Depth() const;
    // Lifetime counters for runtime diagnostics; Clear does not erase evidence.
    std::uint64_t StartedCount() const;
    std::uint64_t FinishedCount() const;
    void Clear();
    // Scene handler cleanup preserves all suspended SDK contexts/modules.
    void ClearHandlers();
    void AppendCacheEntries(std::vector<NativeScriptCacheEntry>& entries);
    void RetireClearedEntries();

   private:
    friend class NativeUiCountdownScriptRuntime;
    friend class NativePropertyWindowScriptRuntime;
    friend class NativeShopTaskScriptRuntime;
    friend class NativeNewGameScriptSession;
    [[nodiscard]] bool HasActiveUiModule(std::string_view module) const;
    void LoadUiModule(std::string_view source, std::string_view module);
    void PushUiModule(std::string_view module, std::string_view function);
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}  // namespace pal4
