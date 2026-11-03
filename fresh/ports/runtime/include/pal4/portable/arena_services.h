#pragma once
#include "pal4/portable/script_stack.h"

namespace pal4::portable {
struct ArenaLoadCommand {ScriptToken token;std::uint64_t generation{};std::string world,scene,resource;};
class ArenaServices final:public IScriptContextServices {
public:
    explicit ArenaServices(IScriptContextServices& next):next_(next){}
    void OnCall(const ScriptRequest& request) override {next_.OnCall(request);}
    ScriptResponse Invoke(const ScriptRequest& request) override;
    std::optional<ArenaLoadCommand> TakeCommand();
    bool Finish(ScriptToken token);
    void Cancel() noexcept {pending_.reset();dispatched_=false;}
    bool Waiting() const noexcept {return pending_.has_value();}
private:
    IScriptContextServices& next_;
    std::optional<ArenaLoadCommand> pending_;
    bool dispatched_{};
    std::uint64_t next_generation_{};
};
} // namespace pal4::portable
