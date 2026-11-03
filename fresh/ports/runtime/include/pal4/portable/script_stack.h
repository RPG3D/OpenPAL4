#pragma once
#include "pal4/portable/script.h"
#include <optional>

namespace pal4::portable {
struct ScriptToken {
    std::uint64_t context{},sequence{};
    bool operator==(const ScriptToken&) const = default;
};
struct ScriptRequest {ScriptToken token;const ScriptCall& call;};
class IScriptContextServices {
public:
    virtual ~IScriptContextServices()=default;
    virtual void OnCall(const ScriptRequest&) {}
    // Arguments are borrowed for this call. Async services must copy them.
    virtual ScriptResponse Invoke(const ScriptRequest& request)=0;
};
enum class ScriptWaitKind {none,delay,external};

// One game thread owns the stack. Complete external operations on that thread,
// outside Tick; request tokens remain valid only for their matching wait.
class ScriptStack {
public:
    explicit ScriptStack(ScriptGlobals& globals);
    ~ScriptStack();
    ScriptStack(const ScriptStack&)=delete;
    ScriptStack& operator=(const ScriptStack&)=delete;
    void Compile(std::string_view module,std::string_view source);
    void LoadCsb(std::string_view module,std::span<const std::uint8_t> bytes,
        std::function<std::string(std::string_view)> decoder={});
    std::optional<std::uint64_t> Push(std::string_view module,std::string_view declaration,IScriptContextServices& services);
    ScriptState Tick(double elapsed,std::uint32_t statement_budget=10000,ScriptRunMode mode=ScriptRunMode::until_wait);
    bool Complete(ScriptToken token,std::string_view error={});
    bool ContainsActive(std::string_view module,std::string_view declaration) const;
    bool HasActiveModule(std::string_view module) const;
    void UnloadModule(std::string_view module);
    bool PopFinished();
    void Clear();
    std::size_t Depth() const;
    std::uint64_t TopId() const;
    ScriptState State() const;
    ScriptWaitKind WaitKind() const;
    std::optional<ScriptToken> Pending() const;
    const std::string& Error() const;
    std::uint32_t ImmediateMode() const;
    std::uint32_t SavedImmediateMode() const;
    std::uint64_t StepYields() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
struct ChapterStackContract {
    std::uint32_t calls{},scene_initializers{};
    std::int32_t story{};
    double simulation_seconds{};
};
// Runs verified source with real giWait countdowns and nested scene contexts;
// remaining services are recording fixtures, not gameplay implementations.
ChapterStackContract VerifyChapterStackContract(std::string_view source,
    std::string_view opening_trace,std::string_view destination_trace);
} // namespace pal4::portable
