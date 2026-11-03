#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <span>
#include <functional>

namespace pal4::portable {
using ScriptValue=std::variant<std::monostate,std::int32_t,bool,float,double,std::string>;
using ScriptGlobals=std::array<std::int32_t,48>;
struct ScriptCall { int system_id; std::int32_t story_before; std::vector<ScriptValue> arguments; };
struct ScriptResponse { ScriptValue value; bool suspend{}; };
class IScriptServices {
public:
    virtual ~IScriptServices()=default;
    virtual ScriptResponse Invoke(const ScriptCall& call)=0;
    virtual std::uint32_t ImmediateMode() const {return 0;}
};
enum class ScriptState { idle,prepared,suspended,finished,failed };
enum class ScriptPause { none,service,budget,step };
enum class ScriptRunMode {until_wait,original_steps};
class ScriptSession {
public:
    ScriptSession(ScriptGlobals& globals,IScriptServices& services);
    ~ScriptSession();
    ScriptSession(const ScriptSession&)=delete;
    ScriptSession& operator=(const ScriptSession&)=delete;
    void Compile(std::string_view module,std::string_view source);
    void LoadCsb(std::string_view module,std::span<const std::uint8_t> bytes,
        std::function<std::string(std::string_view)> decoder={});
    void UnloadModule(std::string_view module);
    // Shares compiled modules and globals, but owns an independent context.
    std::unique_ptr<ScriptSession> Fork(IScriptServices& services);
    bool HasFunction(std::string_view module,std::string_view declaration) const;
    void Start(std::string_view module,std::string_view declaration);
    ScriptState Execute(std::uint32_t statement_budget=10000,ScriptRunMode mode=ScriptRunMode::until_wait);
    void Cancel();
    void Fail(std::string message);
    ScriptState State() const;
    ScriptPause PauseReason() const;
    const std::string& Error() const;
private:
    ScriptSession(ScriptSession& shared,IScriptServices& services);
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
std::string EncodeScriptCall(const ScriptCall& call);
struct ScriptContract { std::int32_t story{}; std::uint32_t calls{},suspensions{}; std::string trace; };
struct ScriptQueryFixture { int system_id; ScriptValue value; };
// Isolated migration check. Every recorded call yields; no game service runs.
ScriptContract VerifyScriptSourceContract(std::string_view module,std::string_view source,
    std::string_view declaration,std::int32_t initial_story,std::string_view expected_trace,
    const std::vector<ScriptQueryFixture>& queries);
ScriptContract VerifyNewGameScriptContract(std::string_view source,std::string_view original_trace);
}  // namespace pal4::portable
