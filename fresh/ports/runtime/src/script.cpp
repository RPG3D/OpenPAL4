#include "pal4/portable/script.h"
#include "pal4/portable/csb_execution.h"
#include <unordered_map>
#include "pal4/script_global_variable_registry.h"
#include <angelscript.h>
#include <scriptstdstring.h>
#include <cmath>
#include <bit>
#include <stdexcept>

namespace pal4::portable {
struct ScriptSession::Impl {
    struct Binding { int id; };
    struct Sdk {
        struct Legacy {std::shared_ptr<const CsbModule> module;CsbTextDecoder decoder;};
        std::unordered_map<std::string,Legacy> legacy;
        asIScriptEngine* engine{};
        std::vector<std::unique_ptr<Binding>> bindings;
        std::string diagnostics;
        unsigned active{};
        bool executing{};
        ~Sdk() {if(engine)engine->ShutDownAndRelease();}
    };
    std::shared_ptr<Sdk> sdk;
    ScriptGlobals& globals;
    IScriptServices& services;
    asIScriptEngine* engine{};
    asIScriptContext* context{};
    ScriptState state{ScriptState::idle};
    ScriptPause pause{};
    std::uint32_t budget{};
    std::string error;
    std::unique_ptr<CsbExecution> legacy;
    Impl(ScriptGlobals& g,IScriptServices& s):globals(g),services(s) {}
    bool Active() const {return state==ScriptState::prepared || state==ScriptState::suspended;}
    ~Impl() {if(Active())--sdk->active;if(context)context->Release();}
    void CreateContext() {
        context=engine->CreateContext();
        if(!context)throw std::runtime_error("cannot create portable script context");
        context->SetUserData(this);context->SetLineCallback(asFUNCTION(Impl::Line),this,asCALL_CDECL);
    }
    static void Message(const asSMessageInfo* message,void* user) {
        auto& self=*static_cast<Sdk*>(user);
        self.diagnostics+=std::string(message->section)+":"+std::to_string(message->row)+": "+message->message+"\n";
    }
    static void Line(asIScriptContext* ctx,void* user) {
        auto& self=*static_cast<Impl*>(user);
        if (!self.budget) { self.pause=ScriptPause::budget; ctx->Suspend(); }
        else --self.budget;
    }
    static void Math(asIScriptGeneric* args) {
        const std::string_view name=args->GetFunction()->GetName();
        if (name=="abs") args->SetReturnDWord(static_cast<asDWORD>(std::abs(static_cast<std::int32_t>(args->GetArgDWord(0)))));
        else if (name=="fabs") args->SetReturnDouble(std::fabs(args->GetArgDouble(0)));
        else {
            const float x=args->GetArgFloat(0);
            const float result=name=="sqrtf"?std::sqrt(x):name=="sinf"?std::sin(x):name=="cosf"?std::cos(x):
                name=="acosf"?std::acos(x):name=="asinf"?std::asin(x):name=="tanf"?std::tan(x):std::atan(x);
            args->SetReturnFloat(result);
        }
    }
    static void DoubleFromBits(asIScriptGeneric* args) {
        args->SetReturnDouble(std::bit_cast<double>(static_cast<std::uint64_t>(args->GetArgQWord(0))));
    }
    static void Invoke(asIScriptGeneric* args) {
        auto* ctx=asGetActiveContext();
        try {
            auto* owner=ctx?static_cast<Impl*>(ctx->GetUserData()):nullptr;
            if(!owner)throw std::runtime_error("game services cannot run during module initialization");
            auto& self=*owner;
            const auto* binding=static_cast<Binding*>(args->GetFunction()->GetUserData());
            ScriptCall call{binding->id,self.globals[0],{}};
            const int string_type=self.engine->GetTypeIdByDecl("string");
            for (int i=0;i<args->GetArgCount();++i) {
                int type{}; asDWORD flags{}; args->GetFunction()->GetParam(i,&type,&flags);
                if (flags & asTM_OUTREF) throw std::runtime_error("writable script reference requires a typed write-back service");
                if ((flags & asTM_INREF) && type!=string_type) throw std::runtime_error("unsupported primitive script reference");
                if (type==string_type) call.arguments.emplace_back(*static_cast<std::string*>(args->GetArgObject(i)));
                else if (type==asTYPEID_INT32) call.arguments.emplace_back(static_cast<std::int32_t>(args->GetArgDWord(i)));
                else if (type==asTYPEID_BOOL) call.arguments.emplace_back(args->GetArgByte(i)!=0);
                else if (type==asTYPEID_FLOAT) call.arguments.emplace_back(args->GetArgFloat(i));
                else if (type==asTYPEID_DOUBLE) call.arguments.emplace_back(args->GetArgDouble(i));
                else throw std::runtime_error("unsupported game script argument type");
            }
            const auto response=self.services.Invoke(call);
            const int result=args->GetReturnTypeId();
            if (result==asTYPEID_VOID) { if (!std::holds_alternative<std::monostate>(response.value)) throw std::runtime_error("non-void service result for void game function"); }
            else if (result==asTYPEID_INT32) args->SetReturnDWord(static_cast<asDWORD>(std::get<std::int32_t>(response.value)));
            else if (result==asTYPEID_BOOL) args->SetReturnByte(std::get<bool>(response.value)?1:0);
            else if (result==asTYPEID_FLOAT) args->SetReturnFloat(std::get<float>(response.value));
            else if (result==string_type) args->SetReturnObject(const_cast<std::string*>(&std::get<std::string>(response.value)));
            else throw std::runtime_error("unsupported game script result type");
            if (response.suspend) { self.pause=ScriptPause::service; ctx->Suspend(); }
        } catch (const std::exception& e) { if(ctx)ctx->SetException(e.what()); }
    }
    void Register(int id,const char* declaration) {
        const int function=engine->RegisterGlobalFunction(declaration,asFUNCTION(Invoke),asCALL_GENERIC);
        if (function<0) throw std::runtime_error(std::string("script API registration: ")+declaration+"\n"+sdk->diagnostics);
        auto binding=std::make_unique<Binding>(); binding->id=id;
        engine->GetFunctionById(function)->SetUserData(binding.get()); sdk->bindings.push_back(std::move(binding));
    }
};
ScriptSession::ScriptSession(ScriptGlobals& globals,IScriptServices& services):impl_(std::make_unique<Impl>(globals,services)) {
    auto& self=*impl_;self.sdk=std::make_shared<Impl::Sdk>();self.engine=asCreateScriptEngine();self.sdk->engine=self.engine;
    if (!self.engine) throw std::runtime_error("cannot create portable script engine");
    self.engine->SetMessageCallback(asFUNCTION(Impl::Message),self.sdk.get(),asCALL_CDECL);
    RegisterStdString(self.engine);
    if(self.engine->RegisterGlobalFunction("double pal4DoubleFromBits(uint64)",asFUNCTION(Impl::DoubleFromBits),asCALL_GENERIC)<0)
        throw std::runtime_error("script exact literal registration failed");
    for (const auto* declaration:{"int abs(int)","double fabs(double)","float sqrtf(float)","float sinf(float)",
         "float cosf(float)","float acosf(float)","float asinf(float)","float tanf(float)","float atanf(float)"})
        if (self.engine->RegisterGlobalFunction(declaration,asFUNCTION(Impl::Math),asCALL_GENERIC)<0) throw std::runtime_error("script math registration failed");
#define PAL4_GAME_FUNCTION(id,declaration) self.Register(id,declaration);
#include "game_function_catalog.inc"
#undef PAL4_GAME_FUNCTION
    const auto bind=[&](const std::string& name,std::size_t index) {
        if (self.engine->RegisterGlobalProperty(name.c_str(),&globals[index])<0) throw std::runtime_error("script global registration failed");
    };
    bind("int global_mvar",0);
    for (unsigned i=1;i<=10;++i) bind("int maze_var"+std::to_string(i),i);
    for (unsigned i=0;i<37;++i) bind(std::string(pal4::ScriptGlobalVariableRegistry::NamedSignature(static_cast<pal4::ScriptNamedVariable>(i))),11+i);
    self.CreateContext();
}
ScriptSession::ScriptSession(ScriptSession& shared,IScriptServices& services):impl_(std::make_unique<Impl>(shared.impl_->globals,services)) {
    impl_->sdk=shared.impl_->sdk;impl_->engine=impl_->sdk->engine;impl_->CreateContext();
}
ScriptSession::~ScriptSession()=default;
std::unique_ptr<ScriptSession> ScriptSession::Fork(IScriptServices& services) {
    if(impl_->sdk->executing)throw std::logic_error("cannot create script context during execution");
    return std::unique_ptr<ScriptSession>(new ScriptSession(*this,services));
}
bool ScriptSession::HasFunction(std::string_view module,std::string_view declaration) const {
    const auto found=impl_->sdk->legacy.find(std::string(module));
    if(found!=impl_->sdk->legacy.end()) {
        if(!declaration.starts_with("void ") || !declaration.ends_with("()"))return false;
        const auto* function=found->second.module->Find(declaration.substr(5,declaration.size()-7));
        return function && function->parameters.empty() && function->result.token==69;
    }
    auto* loaded=impl_->engine->GetModule(std::string(module).c_str(),asGM_ONLY_IF_EXISTS);
    return loaded && loaded->GetFunctionByDecl(std::string(declaration).c_str());
}
void ScriptSession::Compile(std::string_view name,std::string_view source) {
    auto& self=*impl_;
    if(self.sdk->executing || (self.sdk->active && (self.sdk->legacy.contains(std::string(name)) || self.engine->GetModule(std::string(name).c_str(),asGM_ONLY_IF_EXISTS))))
        throw std::logic_error("cannot replace a module while script contexts are active");
    self.sdk->diagnostics.clear(); auto* module=self.engine->GetModule(std::string(name).c_str(),asGM_ALWAYS_CREATE);
    if (!module || module->AddScriptSection("restored.cs",source.data(),source.size())<0 || module->Build()<0)
        throw std::runtime_error("script compilation failed:\n"+self.sdk->diagnostics);
    self.sdk->legacy.erase(std::string(name));
}
void ScriptSession::LoadCsb(std::string_view name,std::span<const std::uint8_t> bytes,CsbTextDecoder decoder) {
    auto& self=*impl_;
    if(self.sdk->executing || (self.sdk->active && (self.sdk->legacy.contains(std::string(name)) || self.engine->GetModule(std::string(name).c_str(),asGM_ONLY_IF_EXISTS))))
        throw std::logic_error("cannot replace a module while script contexts are active");
    auto module=std::make_shared<const CsbModule>(ReadCsb(bytes));
    if(self.engine->GetModule(std::string(name).c_str(),asGM_ONLY_IF_EXISTS))self.engine->DiscardModule(std::string(name).c_str());
    self.sdk->legacy.insert_or_assign(std::string(name),Impl::Sdk::Legacy{std::move(module),std::move(decoder)});
}
void ScriptSession::UnloadModule(std::string_view name) {
    auto& self=*impl_;
    if(self.sdk->executing || self.Active())throw std::logic_error("cannot unload a script module during execution");
    const std::string module(name);
    self.sdk->legacy.erase(module);
    if(self.engine->GetModule(module.c_str(),asGM_ONLY_IF_EXISTS))self.engine->DiscardModule(module.c_str());
}
void ScriptSession::Start(std::string_view module,std::string_view declaration) {
    auto& self=*impl_;
    if(self.sdk->executing || self.Active())throw std::logic_error("script context is already active or executing");
    const auto found=self.sdk->legacy.find(std::string(module));
    if(found!=self.sdk->legacy.end()) {
        if(!HasFunction(module,declaration))throw std::runtime_error("legacy script entry is missing");
        self.legacy=std::make_unique<CsbExecution>(found->second.module,declaration.substr(5,declaration.size()-7),self.globals,self.services,found->second.decoder);
        self.state=ScriptState::prepared;++self.sdk->active;self.pause=ScriptPause::none;self.error.clear();return;
    }
    self.legacy.reset();
    auto* m=self.engine->GetModule(std::string(module).c_str());
    auto* function=m?m->GetFunctionByDecl(std::string(declaration).c_str()):nullptr;
    if (!function || self.context->Prepare(function)<0) throw std::runtime_error("script entry is missing");
    self.state=ScriptState::prepared;++self.sdk->active; self.pause=ScriptPause::none; self.error.clear();
}
ScriptState ScriptSession::Execute(std::uint32_t budget,ScriptRunMode mode) {
    auto& self=*impl_;
    if(!budget || !self.Active() || self.sdk->executing)throw std::logic_error("script execution requires an inactive prepared/suspended context and budget");
    if(mode==ScriptRunMode::original_steps && !self.legacy)throw std::logic_error("original script steps require raw CSB");
    self.budget=budget; self.pause=ScriptPause::none;
    struct Guard {bool& value;explicit Guard(bool& flag):value(flag){value=true;}~Guard(){value=false;}} guard(self.sdk->executing);
    if(self.legacy) {
        self.state=self.legacy->Execute(budget,mode);self.pause=self.legacy->PauseReason();self.error=self.legacy->Error();
        if(!self.Active())--self.sdk->active;
        return self.state;
    }
    const int state=self.context->Execute();
    if (state==asEXECUTION_FINISHED) self.state=ScriptState::finished;
    else if (state==asEXECUTION_SUSPENDED) self.state=ScriptState::suspended;
    else { self.state=ScriptState::failed; self.error=self.context->GetExceptionString()?self.context->GetExceptionString():"script execution failed"; }
    if(!self.Active())--self.sdk->active;
    return self.state;
}
void ScriptSession::Cancel() {
    auto& self=*impl_;
    if(self.sdk->executing)throw std::logic_error("cannot cancel an executing script engine");
    if(self.Active()){if(!self.legacy)self.context->Abort();--self.sdk->active;}
    self.legacy.reset();
    self.context->Unprepare();self.state=ScriptState::idle;self.pause=ScriptPause::none;self.error.clear();
}
void ScriptSession::Fail(std::string message) {
    if(!impl_->Active() || message.empty())throw std::logic_error("script failure requires an active context and message");
    Cancel();impl_->state=ScriptState::failed;impl_->error=std::move(message);
}
ScriptState ScriptSession::State() const { return impl_->state; }
ScriptPause ScriptSession::PauseReason() const { return impl_->pause; }
const std::string& ScriptSession::Error() const { return impl_->error; }
ScriptContract VerifyScriptSourceContract(std::string_view module,std::string_view source,
    std::string_view declaration,std::int32_t initial_story,std::string_view expected_trace,
    const std::vector<ScriptQueryFixture>& queries) {
    struct Recording final:IScriptServices {
        const std::vector<ScriptQueryFixture>& queries;
        ScriptContract result;
        explicit Recording(const std::vector<ScriptQueryFixture>& supplied):queries(supplied) {}
        ScriptResponse Invoke(const ScriptCall& call) override {
            result.trace+=EncodeScriptCall(call);++result.calls;
            for(const auto& query:queries)if(query.system_id==call.system_id)return {query.value,true};
            return {{},true};
        }
    } recording(queries);
    ScriptGlobals globals{};globals[0]=initial_story;ScriptSession session(globals,recording);
    session.Compile(module,source);session.Start(module,declaration);
    for(unsigned tick=0;tick<10001;++tick) {
        const auto state=session.Execute();
        if(state==ScriptState::failed)throw std::runtime_error(session.Error());
        if(state==ScriptState::finished)break;
        if(session.PauseReason()!=ScriptPause::service)throw std::runtime_error("source contract exceeded statement budget");
        ++recording.result.suspensions;
    }
    recording.result.story=globals[0];recording.result.trace+="final "+std::to_string(globals[0])+"\n";
    if(session.State()!=ScriptState::finished)throw std::runtime_error("source contract exceeded service call limit");
    if(recording.result.trace!=expected_trace) {
        std::size_t offset=0;
        while(offset<recording.result.trace.size() && offset<expected_trace.size() && recording.result.trace[offset]==expected_trace[offset])++offset;
        const auto last=offset?recording.result.trace.rfind('\n',offset-1):std::string::npos;
        const auto start=last==std::string::npos?0:last+1;
        throw std::runtime_error("portable source trace differs from legacy source oracle at byte "+std::to_string(offset)+
            "\nportable: "+recording.result.trace.substr(start,recording.result.trace.find('\n',start)-start)+
            "\nlegacy: "+std::string(expected_trace.substr(start,expected_trace.find('\n',start)-start)));
    }
    return recording.result;
}
ScriptContract VerifyNewGameScriptContract(std::string_view source,std::string_view original_trace) {
    struct Recording final:IScriptServices {
        ScriptContract result;
        ScriptResponse Invoke(const ScriptCall& call) override {
            result.trace+=EncodeScriptCall(call); ++result.calls;
            const bool wait=call.system_id==291 || call.system_id==297 || call.system_id==65;
            return {{},wait};
        }
    } recording;
    ScriptGlobals globals{}; ScriptSession session(globals,recording);
    session.Compile("sdk_contract","void Numeric(){global_mvar = parseFloat(\"1.25\")==1.25 ? 1 : 0;}");
    session.Start("sdk_contract","void Numeric()");
    if (session.Execute()!=ScriptState::finished || globals[0]!=1) throw std::runtime_error("script numeric locale contract failed");
    globals.fill(0);
    session.Compile("opening",source); session.Start("opening","void NewGame()");
    for (unsigned tick=0;tick<16;++tick) {
        const auto state=session.Execute();
        if (state==ScriptState::failed) throw std::runtime_error(session.Error());
        if (state==ScriptState::finished) break;
        if (session.PauseReason()!=ScriptPause::service) throw std::runtime_error("unexpected NewGame execution budget pause");
        ++recording.result.suspensions;
    }
    recording.result.story=globals[0]; recording.result.trace+="final "+std::to_string(globals[0])+"\n";
    if (session.State()!=ScriptState::finished || recording.result.trace!=original_trace || recording.result.suspensions!=4)
        throw std::runtime_error("64-bit NewGame trace differs from original SDK oracle");
    return recording.result;
}
}  // namespace pal4::portable
