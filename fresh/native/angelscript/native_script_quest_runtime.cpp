#include "pal4/native_script_quest_runtime.h"
#include "pal4/game_arena_load_handler.h"
#include <bit>
#include <stdexcept>
#include <utility>
namespace pal4 {
NativeScriptQuestRuntime::NativeScriptQuestRuntime(asIScriptContext& c,IGameArenaLoadRuntime& xml,NativeScriptQuestServices s):context_(c),xml_(xml),services_(std::move(s)){
    if(!services_.manager || !services_.dialog || !services_.show || !services_.result || !services_.current_state_id)throw std::invalid_argument("Quest requires live UI/state/XML services");
}
unsigned NativeScriptQuestRuntime::FrameManager(){return services_.manager;}
unsigned NativeScriptQuestRuntime::CreateQuestDialog(unsigned manager){if(manager!=services_.manager || asGetActiveContext()!=&context_)throw std::logic_error("Quest requires its executing SDK context");frame_=services_.dialog();if(!frame_)throw std::runtime_error("Quest dialog factory returned null");return frame_;}
void NativeScriptQuestRuntime::ShowText(unsigned frame,const char* text){if(active_ || frame!=frame_ || asGetActiveContext()!=&context_)throw std::logic_error("Quest show context/owner");services_.show(frame,text?text:"");}
std::int32_t NativeScriptQuestRuntime::ReadSignedWord(unsigned a){if(!frame_ || a!=frame_+0x4cu)throw std::invalid_argument("Quest result owner");return services_.result(frame_);}
unsigned NativeScriptQuestRuntime::InitializeDocument(){if(document_)throw std::logic_error("Quest document reentry");document_=xml_.InitializeXmlDocument();return document_;}
unsigned NativeScriptQuestRuntime::DestroyDocument(unsigned d){if(!document_ || d!=document_)throw std::invalid_argument("Quest document retirement");const auto result=static_cast<unsigned>(xml_.DestroyXmlDocument(d));document_=0u;return result;}
void NativeScriptQuestRuntime::BeginFrameName(std::string_view name){if(name!="frame" || name_active_)throw std::invalid_argument("Quest frame name");name_active_=true;}
void NativeScriptQuestRuntime::EndFrameName(){if(!name_active_)throw std::logic_error("Quest frame name cleanup");name_active_=false;}
ScriptFrameAttributeLookup NativeScriptQuestRuntime::FindAttribute(unsigned d,std::string_view name){if(d!=document_ || name!="frame" || !name_active_)throw std::invalid_argument("Quest attribute lookup");return {xml_.FindAttribute(d,name).value_or(0u),0u};}
unsigned NativeScriptQuestRuntime::CreateFrameAttribute(std::string_view name,unsigned frame){if(name!="frame" || frame!=frame_)throw std::invalid_argument("Quest frame attribute");return xml_.InitializeIntAttribute(name,std::bit_cast<int>(frame));}
void NativeScriptQuestRuntime::AddAttribute(unsigned d,unsigned attr){if(d!=document_)throw std::invalid_argument("Quest document owner");xml_.AddAttribute(d,attr);}
void NativeScriptQuestRuntime::DestroyAttribute(unsigned a){xml_.DestroyAttribute(a);}
void NativeScriptQuestRuntime::SetExistingAttribute(unsigned a,unsigned frame){if(frame!=frame_)throw std::invalid_argument("Quest existing frame attribute");xml_.SetInt(a-0x1cu,std::bit_cast<int>(frame));}
unsigned NativeScriptQuestRuntime::Game(){return xml_.GameInstance();}
void NativeScriptQuestRuntime::InitializeStateAndUi(unsigned game,unsigned state,unsigned d){
    if(game!=Game() || state!=10u || d!=document_ || active_ || asGetActiveContext()!=&context_)throw std::invalid_argument("Quest state entry");
    xml_.InitializeStateAndUi(game,state,d);
    if(context_.Suspend()<0)throw std::runtime_error("Quest SDK context suspend failed");active_=true;
}
bool NativeScriptQuestRuntime::Tick(float){if(!active_ || services_.current_state_id()!=1u)return false;active_=false;return true;}
void NativeScriptQuestRuntime::ClearHandlers(){if(asGetActiveContext()==&context_)throw std::logic_error("Cannot retire executing Quest");active_=false;}
}
