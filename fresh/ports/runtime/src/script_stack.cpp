#include "pal4/portable/script_stack.h"
#include "pal4/portable/script_immediate.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace pal4::portable {
namespace {
bool Equal(std::string_view a,std::string_view b) {
    const auto lower=[](unsigned char c){return c>='A' && c<='Z'?c+('a'-'A'):c;};
    return a.size()==b.size() && std::equal(a.begin(),a.end(),b.begin(),[&](auto x,auto y){return lower(x)==lower(y);});
}
bool Active(ScriptState state) {return state==ScriptState::prepared || state==ScriptState::suspended;}
}
struct ScriptStack::Impl {
    struct Unbound:IScriptServices {
        ScriptResponse Invoke(const ScriptCall&) override {throw std::logic_error("stack compiler context cannot execute");}
    } unbound;
    ScriptSession compiler;
    struct Frame:IScriptServices {
        std::uint64_t id,sequence{};
        IScriptContextServices& services;
        std::string module,declaration;
        ScriptWaitKind wait{};
        std::optional<ScriptToken> pending;
        double remaining{};
        std::unique_ptr<ScriptSession> session;
        ScriptImmediateState immediate;std::uint64_t step_yields{};
        std::uint32_t ImmediateMode() const override{return immediate.Current();}
        Frame(std::uint64_t value,IScriptContextServices& host):id(value),services(host) {}
        ScriptResponse Invoke(const ScriptCall& call) override {
            if(wait!=ScriptWaitKind::none)throw std::logic_error("script invoked a service while waiting");
            if(sequence==std::numeric_limits<std::uint64_t>::max())throw std::overflow_error("script request sequence exhausted");
            const ScriptToken token{id,++sequence};
            services.OnCall({token,call});
            if(call.system_id==55 || call.system_id==56){
                if(!call.arguments.empty())throw std::runtime_error("invalid immediate mode arguments");
                if(call.system_id==55)immediate.Begin();else immediate.End();return {};
            }
            if(call.system_id==239) {
                if(call.arguments.size()!=1)throw std::runtime_error("invalid giWait arguments");
                remaining=std::get<double>(call.arguments[0]);
                if(!std::isfinite(remaining))throw std::runtime_error("nonfinite script delay");
                wait=ScriptWaitKind::delay;pending=token;return {{},true};
            }
            auto response=services.Invoke({token,call});
            if(response.suspend){wait=ScriptWaitKind::external;pending=token;}
            return response;
        }
    };
    std::vector<std::unique_ptr<Frame>> frames;
    std::uint64_t next_id{};
    bool executing{};
    std::string empty_error;
    explicit Impl(ScriptGlobals& globals):compiler(globals,unbound) {}
    void Inactive() const {if(executing)throw std::logic_error("cannot mutate an executing script stack");}
};
ScriptStack::ScriptStack(ScriptGlobals& globals):impl_(std::make_unique<Impl>(globals)) {}
ScriptStack::~ScriptStack()=default;
void ScriptStack::Compile(std::string_view module,std::string_view source) {
    impl_->Inactive();impl_->compiler.Compile(module,source);
}
void ScriptStack::LoadCsb(std::string_view module,std::span<const std::uint8_t> bytes,std::function<std::string(std::string_view)> decoder) {
    impl_->Inactive();impl_->compiler.LoadCsb(module,bytes,std::move(decoder));
}
std::optional<std::uint64_t> ScriptStack::Push(std::string_view module,std::string_view declaration,IScriptContextServices& services) {
    auto& self=*impl_;self.Inactive();
    for(const auto& frame:self.frames)
        if(Active(frame->session->State()) && Equal(module,frame->module) && Equal(declaration,frame->declaration))return {};
    if(!self.compiler.HasFunction(module,declaration))return {};
    if(!self.frames.empty() && !Active(self.frames.back()->session->State()))
        throw std::logic_error("completed or failed script top must be retired before push");
    if(self.next_id==std::numeric_limits<std::uint64_t>::max())throw std::overflow_error("script context ID exhausted");
    auto frame=std::make_unique<Impl::Frame>(++self.next_id,services);
    frame->module=module;frame->declaration=declaration;frame->session=self.compiler.Fork(*frame);
    frame->session->Start(module,declaration);
    const auto id=frame->id;self.frames.push_back(std::move(frame));return id;
}
ScriptState ScriptStack::Tick(double elapsed,std::uint32_t budget,ScriptRunMode mode) {
    auto& self=*impl_;self.Inactive();
    if(!std::isfinite(elapsed) || elapsed<0 || !budget)throw std::invalid_argument("invalid script frame time or budget");
    if(self.frames.empty())return ScriptState::idle;
    auto& frame=*self.frames.back();
    if(!Active(frame.session->State()))return frame.session->State();
    if(frame.wait==ScriptWaitKind::external)return ScriptState::suspended;
    if(frame.wait==ScriptWaitKind::delay) {
        // Same remaining-time rule as ScriptWaitHandler::Update (IDA 5dfad0).
        frame.remaining-=elapsed;
        if(frame.remaining>0)return ScriptState::suspended;
        frame.remaining=0;frame.wait=ScriptWaitKind::none;frame.pending.reset();
    }
    struct Guard {bool& value;explicit Guard(bool& flag):value(flag){value=true;}~Guard(){value=false;}} guard(self.executing);
    const auto state=frame.session->Execute(budget,mode);
    if(frame.session->PauseReason()==ScriptPause::step)++frame.step_yields;
    return state;
}
bool ScriptStack::Complete(ScriptToken token,std::string_view error) {
    auto& self=*impl_;self.Inactive();
    for(auto& frame:self.frames)
        if(frame->wait==ScriptWaitKind::external && frame->pending==token && Active(frame->session->State())) {
            if(!error.empty())frame->session->Fail(std::string(error));
            frame->wait=ScriptWaitKind::none;frame->pending.reset();return true;
        }
    return false;
}
bool ScriptStack::ContainsActive(std::string_view module,std::string_view declaration) const {
    for(const auto& frame:impl_->frames)if(Active(frame->session->State()) && Equal(module,frame->module) && Equal(declaration,frame->declaration))return true;
    return false;
}
bool ScriptStack::HasActiveModule(std::string_view module) const {
    for(const auto& frame:impl_->frames)if(Active(frame->session->State()) && Equal(module,frame->module))return true;
    return false;
}
void ScriptStack::UnloadModule(std::string_view module) {
    impl_->Inactive();
    if(HasActiveModule(module))throw std::logic_error("cannot unload a module with an active script context");
    impl_->compiler.UnloadModule(module);
}
bool ScriptStack::PopFinished() {
    auto& self=*impl_;self.Inactive();
    if(self.frames.empty())return false;
    if(self.frames.back()->session->State()!=ScriptState::finished)throw std::logic_error("cannot retire an unfinished script context");
    self.frames.pop_back();return true;
}
void ScriptStack::Clear() {
    auto& self=*impl_;self.Inactive();
    for(auto& frame:self.frames)frame->session->Cancel();
    self.frames.clear();
}
std::size_t ScriptStack::Depth() const {return impl_->frames.size();}
std::uint64_t ScriptStack::TopId() const {return impl_->frames.empty()?0:impl_->frames.back()->id;}
ScriptState ScriptStack::State() const {return impl_->frames.empty()?ScriptState::idle:impl_->frames.back()->session->State();}
ScriptWaitKind ScriptStack::WaitKind() const {return impl_->frames.empty()?ScriptWaitKind::none:impl_->frames.back()->wait;}
std::optional<ScriptToken> ScriptStack::Pending() const {return impl_->frames.empty()?std::nullopt:impl_->frames.back()->pending;}
const std::string& ScriptStack::Error() const {return impl_->frames.empty()?impl_->empty_error:impl_->frames.back()->session->Error();}
std::uint32_t ScriptStack::ImmediateMode() const{return impl_->frames.empty()?0:impl_->frames.back()->immediate.Current();}
std::uint32_t ScriptStack::SavedImmediateMode() const{return impl_->frames.empty()?0:impl_->frames.back()->immediate.Saved();}
std::uint64_t ScriptStack::StepYields() const{return impl_->frames.empty()?0:impl_->frames.back()->step_yields;}
} // namespace pal4::portable
