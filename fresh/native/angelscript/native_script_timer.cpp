#include "pal4/native_script_timer.h"

#include <bit>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "pal4/pal_script_timed_processor.h"
namespace pal4 {
namespace {
std::uint32_t Address(const void* p) { return reinterpret_cast<std::uint32_t>(p); }
const char* Text(std::uint32_t p) { return reinterpret_cast<const char*>(p); }
}  // namespace
class NativeScriptTimer::Impl final : public IPalScriptTimerStateRuntime {
   public:
    PalScriptExecutorState state{};
    std::string callback;
    std::uint32_t InitStringBuffer(PalScriptExecutorState& s, std::uint32_t off,
                                   const char* text, std::uint32_t n) override {
        if (&s != &state || off != 72u)
            throw std::invalid_argument("native timer callback field");
        callback.assign(text, n);
        s.words[18] = Address(callback.c_str());
        s.words[19] = static_cast<std::uint32_t>(callback.size());
        s.words[20] = static_cast<std::uint32_t>(callback.capacity());
        return Address(&s) + 72u;
    }
    class Frame final : public IPalScriptTimedRuntime {
        Impl& timer;
        INativeScriptTimerHost& host;
        std::unordered_map<StringBufferState*, std::string> strings;
        std::string source, full_path, function;
        bool prepared{};
        void Sync(StringBufferState& s) {
            auto& v = strings.at(&s);
            s = {Address(v.c_str()), static_cast<std::uint32_t>(v.size()),
                 static_cast<std::uint32_t>(v.capacity())};
        }
        void RequireContext(std::uint32_t c) {
            if (c != Address(&timer.state) + 84u)
                throw std::invalid_argument("native timer execution context");
        }
        void RequireInterpreter(std::uint32_t p) {
            if (p != Address(this) || !prepared)
                throw std::invalid_argument("native timer interpreter");
        }

       public:
        Frame(Impl& t, INativeScriptTimerHost& h) : timer(t), host(h) {}
        bool ActorControlActive() override { return host.ActorControlActive(); }
        std::uint32_t ExecutorAddress() override { return Address(&timer.state); }
        std::uint32_t ExecutorWord(std::uint32_t a) override {
            if (a != Address(&timer.state) + 96u)
                throw std::invalid_argument("native timer busy slot");
            return host.ScriptBusy() ? 1u : 0u;
        }
        void InitializeScriptSystem() override { source = host.ScriptSource(); }
        void InitializePath(StringBufferState& s, const char* p) override {
            strings[&s] = p;
            Sync(s);
        }
        void CopyString(StringBufferState& dst, const StringBufferState& src) override {
            strings[&dst] = std::string(Text(src.data), src.length);
            Sync(dst);
        }
        void AppendLiteral(StringBufferState& s, const char* p,
                           std::uint32_t n) override {
            strings.at(&s).append(p, n);
            Sync(s);
        }
        void AppendText(StringBufferState& s, std::uint32_t p, std::uint32_t n) override {
            AppendLiteral(s, Text(p), n);
        }
        bool HasStackModule(std::uint32_t c, std::uint32_t p) override {
            RequireContext(c);
            return host.HasStackModule(Text(p));
        }
        std::uint32_t ArenaContext() override { return Address(source.c_str()); }
        std::uint32_t ScriptMode() override {
            return 1u;
        }  // Original CSB resources, loaded through SDK.
        std::uint32_t CreateInterpreter(const TimedScriptInterpreterRequest& r) override {
            if (r.mode != 1u || r.arena_context != Address(source.c_str()) ||
                r.read_callback != 0x600d50u ||
                std::string_view(Text(r.script_path)) != "TimeScript")
                throw std::invalid_argument("native timer interpreter request");
            full_path = Text(r.full_path);
            function = Text(r.callback);
            prepared = true;
            return Address(this);
        }
        bool AddScriptModule(std::uint32_t c, std::uint32_t p) override {
            RequireContext(c);
            RequireInterpreter(p);
            host.LoadTimerModule(source, full_path);
            return true;
        }
        const char* FunctionName(std::uint32_t p) override {
            RequireInterpreter(p);
            return function.c_str();
        }
        const char* FullPath(std::uint32_t p) override {
            RequireInterpreter(p);
            return full_path.c_str();
        }
        void PushStackModule(std::uint32_t c, const char* path,
                             const char* name) override {
            RequireContext(c);
            host.PushTimerModule(path, name);
        }
        void ReleaseInterpreter(std::uint32_t p) override {
            RequireInterpreter(p);
            prepared = false;
        }
        void ShowMissingScriptMessage() override {
            throw std::runtime_error("PAL4 timed script is unavailable");
        }
        // Host LoadTimerModule uses a scoped archive/read buffer; no global
        // PK script handles survive it. Remaining local text is RAII-owned.
        void CleanupAllScripts() override { source.clear(); }
        std::uint32_t CleanupString(StringBufferState& s) override {
            strings.erase(&s);
            s = {};
            return 0;
        }
    };
};
NativeScriptTimer::NativeScriptTimer() : impl_(std::make_unique<Impl>()) {}
NativeScriptTimer::~NativeScriptTimer() = default;
void NativeScriptTimer::Process(float dt, INativeScriptTimerHost& host) {
    if (!std::isfinite(dt) || dt < 0)
        throw std::invalid_argument("PAL4 timer frame delta");
    Impl::Frame frame(*impl_, host);
    (void)PalScriptTimedProcessor::Process(impl_->state, dt, frame);
}
void NativeScriptTimer::Reset() { (void)ScriptTimedState::Terminate(*this); }
void NativeScriptTimer::DisableForSceneRelease() { impl_->state.words[15u]=0u; }
bool NativeScriptTimer::Enabled() const { return impl_->state.words[15] != 0; }
float NativeScriptTimer::Elapsed() const {
    return std::bit_cast<float>(impl_->state.words[16]);
}
float NativeScriptTimer::Delay() const {
    return std::bit_cast<float>(impl_->state.words[17]);
}
std::string_view NativeScriptTimer::Callback() const { return impl_->callback; }
std::uint32_t NativeScriptTimer::ReadWord(std::uint32_t p) {
    return *reinterpret_cast<const std::uint32_t*>(p);
}
std::uint32_t NativeScriptTimer::EmptyString() { return Address(""); }
std::uint32_t NativeScriptTimer::ScriptExecutor() { return Address(&impl_->state); }
std::uint32_t NativeScriptTimer::InitializeState(std::uint32_t owner, std::uint32_t delay,
                                                 std::uint32_t callback) {
    if (owner != ScriptExecutor()) throw std::invalid_argument("native timer owner");
    return PalScriptExecutor::InitializeTimerState(impl_->state, delay, Text(callback),
                                                   *impl_);
}
void NativeScriptTimer::WriteWord(std::uint32_t p, std::uint32_t v) {
    if (p != ScriptExecutor() + 60u && p != ScriptExecutor() + 64u &&
        p != ScriptExecutor() + 68u)
        throw std::invalid_argument("native timer field");
    *reinterpret_cast<std::uint32_t*>(p) = v;
}
std::uint32_t NativeScriptTimer::TextLength(std::uint32_t p) {
    return static_cast<std::uint32_t>(std::strlen(Text(p)));
}
std::uint32_t NativeScriptTimer::InitializeText(std::uint32_t p, std::uint32_t text,
                                                std::uint32_t n) {
    if (p != ScriptExecutor() + 72u)
        throw std::invalid_argument("native timer callback address");
    return impl_->InitStringBuffer(impl_->state, 72u, Text(text), n);
}
}  // namespace pal4
