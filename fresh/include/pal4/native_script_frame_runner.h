#pragma once

#include "pal4/native_angelscript_engine.h"
#include "pal4/native_script_frame_wait_runtime.h"

#include <angelscript.h>

#include <functional>
#include <string>
#include <utility>

namespace pal4 {

enum class NativeScriptFrameStatus {
    idle,
    queued,
    suspended,
    finished,
    failed,
};

// One game script module is executed on frame ticks, including wait resumes.
class NativeScriptFrameRunner final {
public:
    explicit NativeScriptFrameRunner(NativeAngelScriptEngine& engine);
    ~NativeScriptFrameRunner();
    NativeScriptFrameRunner(const NativeScriptFrameRunner&) = delete;
    NativeScriptFrameRunner& operator=(const NativeScriptFrameRunner&) = delete;

    void Queue(const char* module, const char* function);
    void Cancel();
    // Public SDK Abort releases suspended stack values; retain the context
    // itself because native wait services borrow its stable address.
    void ClearExecution();
    // Suspend at the current native callback, then retire wait owners as soon
    // as the SDK returns, before executing the next script instruction.
    void CleanupAfterCallback(std::function<void()> cleanup);
    [[nodiscard]] NativeScriptFrameStatus Tick(
        float elapsed, INativeScriptFrameWaitRuntime* waits = nullptr);
    [[nodiscard]] NativeScriptFrameStatus TickOriginal(
        float elapsed, INativeScriptFrameWaitRuntime* waits = nullptr,
        const std::function<bool()>& boundary = {});
    [[nodiscard]] NativeScriptFrameStatus TickUntilBoundary(
        float elapsed, INativeScriptFrameWaitRuntime* waits,
        const std::function<bool()>& boundary);
    void SetImmediateModeProvider(std::function<bool()> provider) {
        immediate_mode_ = std::move(provider);
    }
    [[nodiscard]] NativeScriptFrameStatus Status() const noexcept {
        return status_;
    }
    [[nodiscard]] const std::string& Error() const noexcept { return error_; }
    [[nodiscard]] asIScriptContext& Context() const noexcept { return *context_; }
    [[nodiscard]] const std::string& CacheName() const noexcept { return cache_name_; }

private:
    [[nodiscard]] NativeScriptFrameStatus Execute();
    [[nodiscard]] NativeScriptFrameStatus ExecuteStep(
        INativeScriptFrameWaitRuntime* waits,
        const std::function<bool()>& boundary);
    [[nodiscard]] bool ReadyToStep(
        float elapsed, INativeScriptFrameWaitRuntime* waits);
    [[nodiscard]] NativeScriptFrameStatus RecordResult(int result);
    bool FlushCallbackCleanup();

    NativeAngelScriptEngine& engine_;
    asIScriptContext* context_{};
    NativeScriptFrameStatus status_{NativeScriptFrameStatus::idle};
    std::string error_;
    std::string cache_name_;
    std::function<bool()> immediate_mode_;
    std::function<void()> callback_cleanup_;
};

}  // namespace pal4
