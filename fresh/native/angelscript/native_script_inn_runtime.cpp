#include "pal4/native_script_inn_runtime.h"
#include "pal4/game_arena_load_handler.h"
#include <bit>
#include <stdexcept>
#include <utility>
namespace pal4 {
NativeScriptInnRuntime::NativeScriptInnRuntime(asIScriptContext& c,IGameArenaLoadRuntime& xml,NativeScriptInnServices s):context_(c),xml_(xml),services_(std::move(s)){
    if(!services_.manager || !services_.show || !services_.frame || !services_.result || !services_.current_state_id)throw std::invalid_argument("Inn requires live UI/state/XML services");
}
unsigned NativeScriptInnRuntime::FrameManager(){return services_.manager;}
void NativeScriptInnRuntime::ProcessEvent(unsigned dispatcher,unsigned key,unsigned kind,unsigned value){
    if(dispatcher!=services_.manager+0x104u || key!=ScriptInnDialog::kEventKeyCell || kind!=1u || active_ || asGetActiveContext()!=&context_)throw std::invalid_argument("Inn show event/context");
    services_.show(value&0xffu);frame_=services_.frame();if(!frame_)throw std::runtime_error("Inn show did not create the actual frame");
}
unsigned NativeScriptInnRuntime::FindFrame(unsigned manager,unsigned key){
    if(manager!=services_.manager || key!=ScriptInnDialog::kFrameKeyCell)throw std::invalid_argument("Inn frame lookup");
    const auto frame=services_.frame();if(!frame || (frame_ && frame!=frame_))throw std::logic_error("Inn frame owner unavailable or replaced");frame_=frame;return frame;
}
std::int32_t NativeScriptInnRuntime::ReadSignedWord(unsigned a){if(!frame_ || a!=frame_+0x70u)throw std::invalid_argument("Inn result owner");return services_.result(frame_);}
unsigned NativeScriptInnRuntime::InitializeDocument(){if(document_)throw std::logic_error("Inn document reentry");document_=xml_.InitializeXmlDocument();return document_;}
unsigned NativeScriptInnRuntime::DestroyDocument(unsigned d){if(!document_ || d!=document_)throw std::invalid_argument("Inn document retirement");const auto result=static_cast<unsigned>(xml_.DestroyXmlDocument(d));document_=0u;return result;}
void NativeScriptInnRuntime::BeginFrameName(std::string_view name){if(name!="frame" || name_active_)throw std::invalid_argument("Inn frame name");name_active_=true;}
void NativeScriptInnRuntime::EndFrameName(){if(!name_active_)throw std::logic_error("Inn frame name cleanup");name_active_=false;}
ScriptFrameAttributeLookup NativeScriptInnRuntime::FindAttribute(unsigned d,std::string_view name){if(d!=document_ || name!="frame" || !name_active_)throw std::invalid_argument("Inn attribute lookup");return {xml_.FindAttribute(d,name).value_or(0u),0u};}
unsigned NativeScriptInnRuntime::CreateFrameAttribute(std::string_view name,unsigned frame){if(name!="frame" || frame!=frame_)throw std::invalid_argument("Inn frame attribute");return xml_.InitializeIntAttribute(name,std::bit_cast<int>(frame));}
void NativeScriptInnRuntime::AddAttribute(unsigned d,unsigned attr){if(d!=document_)throw std::invalid_argument("Inn document owner");xml_.AddAttribute(d,attr);}
void NativeScriptInnRuntime::DestroyAttribute(unsigned a){xml_.DestroyAttribute(a);}
void NativeScriptInnRuntime::SetExistingAttribute(unsigned a,unsigned frame){if(frame!=frame_)throw std::invalid_argument("Inn existing frame attribute");xml_.SetInt(a-0x1cu,std::bit_cast<int>(frame));}
unsigned NativeScriptInnRuntime::Game(){return xml_.GameInstance();}
void NativeScriptInnRuntime::InitializeStateAndUi(unsigned game,unsigned state,unsigned d){
    if(game!=Game() || state!=10u || d!=document_ || active_ || asGetActiveContext()!=&context_)throw std::invalid_argument("Inn state entry");
    xml_.InitializeStateAndUi(game,state,d);if(context_.Suspend()<0)throw std::runtime_error("Inn SDK context suspend failed");active_=true;
}
bool NativeScriptInnRuntime::Tick(float){if(!active_ || services_.current_state_id()!=1u)return false;active_=false;return true;}
void NativeScriptInnRuntime::ClearHandlers(){if(asGetActiveContext()==&context_)throw std::logic_error("Cannot retire executing Inn wait");active_=false;}
}
