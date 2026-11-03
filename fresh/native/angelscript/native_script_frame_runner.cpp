#include "pal4/native_script_frame_runner.h"

#include <cmath>
#include <stdexcept>

namespace pal4 {

NativeScriptFrameRunner::NativeScriptFrameRunner(NativeAngelScriptEngine& engine)
    : engine_(engine) {
    if (engine_.Raw()->CreateContext(&context_) < 0 || context_ == nullptr)
        throw std::runtime_error("PAL4 script context creation failed");
}

NativeScriptFrameRunner::~NativeScriptFrameRunner() {
    (void)context_->Release();
}

void NativeScriptFrameRunner::Queue(const char* module, const char* function) {
    if (status_ == NativeScriptFrameStatus::queued ||
        status_ == NativeScriptFrameStatus::suspended)
        throw std::logic_error("PAL4 script module is already active");
    if (module == nullptr || function == nullptr)
        throw std::invalid_argument("PAL4 script module and function are required");
    const int id = engine_.FunctionId(module, function);
    const std::string suffix = std::string("[") + function + "]";
    const std::string module_name(module);
    const std::string cache_name = module_name.ends_with(suffix) ? module_name : module_name + suffix;
    if (id < 0 || context_->Prepare(id) < 0)
        throw std::runtime_error("PAL4 script function is unavailable");
    error_.clear();
    cache_name_ = cache_name;
    status_ = NativeScriptFrameStatus::queued;
}

void NativeScriptFrameRunner::Cancel() {
    if (status_ != NativeScriptFrameStatus::queued &&
        status_ != NativeScriptFrameStatus::suspended)
        return;
    if (context_->Abort() < 0)
        throw std::runtime_error("PAL4 script context abort failed");
    status_ = NativeScriptFrameStatus::failed;
    error_ = "PAL4 script execution cancelled";
}

void NativeScriptFrameRunner::ClearExecution() {
    if (asGetActiveContext() == context_)
        throw std::logic_error("PAL4 cannot clear an executing script context");
    if (context_->Abort() < 0)
        throw std::runtime_error("PAL4 script context cleanup failed");
    error_.clear();
    status_ = NativeScriptFrameStatus::idle;
}

NativeScriptFrameStatus NativeScriptFrameRunner::Tick(
    const float elapsed, INativeScriptFrameWaitRuntime* waits) {
    if (!std::isfinite(elapsed) || elapsed < 0.0f)
        throw std::invalid_argument("PAL4 script frame time is invalid");
    const bool wait_closed = waits != nullptr && waits->Tick(elapsed);
    if (status_ == NativeScriptFrameStatus::suspended) {
        if (!wait_closed) return status_;
    } else if (status_ != NativeScriptFrameStatus::queued) {
        return status_;
    }
    return Execute();
}

NativeScriptFrameStatus NativeScriptFrameRunner::Execute() {
    for (;;) {
        const auto result = context_->Execute();
        const bool cleaned = FlushCallbackCleanup();
        if (cleaned && result == asEXECUTION_SUSPENDED) continue;
        return RecordResult(result);
    }
}
void NativeScriptFrameRunner::CleanupAfterCallback(std::function<void()> cleanup) {
    if (!cleanup || callback_cleanup_ || asGetActiveContext() != context_)
        throw std::logic_error("script callback cleanup requires its selected executing context");
    callback_cleanup_ = std::move(cleanup);
    if (context_->Suspend() < 0) {
        callback_cleanup_ = {};
        throw std::runtime_error("script callback cleanup could not suspend the SDK");
    }
}
bool NativeScriptFrameRunner::FlushCallbackCleanup() {
    if (!callback_cleanup_) return false;
    if (asGetActiveContext() == context_)
        throw std::logic_error("script callback cleanup must follow SDK return");
    auto cleanup = std::exchange(callback_cleanup_, {});
    cleanup();
    return true;
}

NativeScriptFrameStatus NativeScriptFrameRunner::TickOriginal(
    const float elapsed, INativeScriptFrameWaitRuntime* waits,
    const std::function<bool()>& boundary) {
    if(boundary && boundary())return status_;
    if (!ReadyToStep(elapsed, waits)) return status_;
    return ExecuteStep(waits, boundary);
}

NativeScriptFrameStatus NativeScriptFrameRunner::TickUntilBoundary(
    const float elapsed, INativeScriptFrameWaitRuntime* waits,
    const std::function<bool()>& boundary) {
    if (!boundary)
        throw std::invalid_argument("PAL4 script boundary is required");
    if (!ReadyToStep(elapsed, waits)) return status_;
    constexpr int kMaximumFrameSteps = 100000;
    for (int index = 0; index < kMaximumFrameSteps; ++index) {
        if (boundary()) return status_;
        const auto status = ExecuteStep(waits, boundary);
        if (status != NativeScriptFrameStatus::suspended || boundary() ||
            (waits && waits->BlocksScript()))
            return status;
    }
    throw std::runtime_error("PAL4 script frame step limit exceeded");
}

bool NativeScriptFrameRunner::ReadyToStep(
    const float elapsed, INativeScriptFrameWaitRuntime* waits) {
    if (!std::isfinite(elapsed) || elapsed < 0.0f)
        throw std::invalid_argument("PAL4 script frame time is invalid");
    const bool blocked = waits && waits->BlocksScript();
    const bool wait_closed = waits && waits->Tick(elapsed);
    if (status_ == NativeScriptFrameStatus::suspended) {
        if (blocked && !wait_closed) return false;
    } else if (status_ != NativeScriptFrameStatus::queued) {
        return false;
    }
    return true;
}

NativeScriptFrameStatus NativeScriptFrameRunner::ExecuteStep(
    INativeScriptFrameWaitRuntime*,
    const std::function<bool()>& boundary) {
    // IDA 7DF6B1/7DF6EA: choose IMM versus one normal source step at entry.
    // A normal step that executes giIMMBegin must not enter the IMM loop until
    // the next invocation of ExecuteCurrentModule.
    const bool run_immediate = immediate_mode_ && immediate_mode_();
    constexpr int kMaximumImmediateSteps = 100000;
    for (int index = 0; index < kMaximumImmediateSteps; ++index) {
        const auto result = context_->ExecuteStep(asEXEC_STEP_INTO);
        (void)FlushCallbackCleanup();
        const auto status = RecordResult(result);
        // 7DF540 checks the execution flag before entering IMM, then runs
        // to IMMEnd without rechecking flags raised by those callbacks.
        if (status != NativeScriptFrameStatus::suspended ||
            (boundary && boundary()) ||
            !run_immediate ||
            !immediate_mode_ || !immediate_mode_())
            return status;
    }
    throw std::runtime_error("PAL4 immediate script step limit exceeded");
}

NativeScriptFrameStatus NativeScriptFrameRunner::RecordResult(
    const int result) {
    if (result == asEXECUTION_FINISHED) {
        status_ = NativeScriptFrameStatus::finished;
    } else if (result == asEXECUTION_SUSPENDED) {
        status_ = NativeScriptFrameStatus::suspended;
    } else {
        status_ = NativeScriptFrameStatus::failed;
        const char* message = context_->GetExceptionString();
        error_ = message != nullptr && *message != '\0'
            ? message : "PAL4 script execution failed";
    }
    return status_;
}

}  // namespace pal4
