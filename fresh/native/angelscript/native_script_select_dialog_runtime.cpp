#include "pal4/native_script_select_dialog_runtime.h"
#include "pal4/game_arena_load_handler.h"
#include <bit>
#include <stdexcept>
#include <utility>
namespace pal4 {
NativeScriptSelectDialogRuntime::NativeScriptSelectDialogRuntime(asIScriptContext& c,
    IGameArenaLoadRuntime& x,NativeScriptSelectDialogServices s):context_(c),xml_(x),services_(std::move(s)){
    if(!services_.manager||!services_.dialog||!services_.add_item||!services_.show||
       !services_.visible||!services_.selection||!services_.set_default)
        throw std::invalid_argument("SelectDialog requires live cached UI/XML services");
}
void NativeScriptSelectDialogRuntime::Executing()const{
    if(asGetActiveContext()!=&context_)throw std::logic_error("SelectDialog SDK context owner");
}
unsigned NativeScriptSelectDialogRuntime::UiFrameManager(){return services_.manager;}
unsigned NativeScriptSelectDialogRuntime::SelectDialog(unsigned m){Executing();return CreateSelectDialog(m);}
unsigned NativeScriptSelectDialogRuntime::CreateSelectDialog(unsigned m){
    if(m!=services_.manager)throw std::invalid_argument("SelectDialog manager owner");
    const auto f=services_.dialog();
    if(!f||(active_&&f!=frame_))throw std::logic_error("SelectDialog cached owner replaced during wait");
    frame_=f;return f;
}
void NativeScriptSelectDialogRuntime::AddItem(std::string_view text){
    Executing();if(active_||borrowed_text_)throw std::logic_error("SelectDialog option reentry");
    borrowed_text_=&text;
    try{(void)ScriptSelectDialog::AddItem(reinterpret_cast<unsigned>(&text),*this);borrowed_text_=nullptr;}
    catch(...){borrowed_text_=nullptr;throw;}
}
int NativeScriptSelectDialogRuntime::AddButton(unsigned f,unsigned text){
    Executing();if(f!=frame_||!borrowed_text_||text!=reinterpret_cast<unsigned>(borrowed_text_))
        throw std::invalid_argument("SelectDialog borrowed SDK text owner");
    services_.add_item(f,*borrowed_text_);return 0;
}
int NativeScriptSelectDialogRuntime::LastSelection(){Executing();return ScriptSelectDialog::LastSelection(*this);}
void NativeScriptSelectDialogRuntime::SetDefaultSelection(int n){Executing();if(active_)throw std::logic_error("SelectDialog default during wait");(void)ScriptSelectDialog::SetDefaultSelection(n,*this);}
unsigned NativeScriptSelectDialogRuntime::ReadWord(unsigned a){if(!frame_||a!=frame_+0x9cu)throw std::invalid_argument("SelectDialog result owner");return std::bit_cast<unsigned>(services_.selection(frame_));}
void NativeScriptSelectDialogRuntime::WriteWord(unsigned a,unsigned v){if(!frame_||a!=frame_+0xa0u)throw std::invalid_argument("SelectDialog default owner");services_.set_default(frame_,std::bit_cast<int>(v));}
void NativeScriptSelectDialogRuntime::SetSelectQuestion(std::string_view t,unsigned option){Executing();if(active_||!frame_||option)throw std::logic_error("SelectDialog show owner/option");services_.show(frame_,t);}
ScriptDialogWaitState* NativeScriptSelectDialogRuntime::AllocateWait(unsigned n){if(n!=sizeof(state_)||active_)throw std::invalid_argument("SelectDialog wait allocation");state_={};return &state_;}
unsigned NativeScriptSelectDialogRuntime::ScriptExecutor(){return reinterpret_cast<unsigned>(this);}
void NativeScriptSelectDialogRuntime::ConstructWait(ScriptDialogWaitState& s,std::uint8_t mode,unsigned q){if(&s!=&state_||mode!=1u||q!=ScriptExecutor()+ScriptDialogs::kQueueOffset)throw std::invalid_argument("SelectDialog wait queue");}
void NativeScriptSelectDialogRuntime::FlushWait(ScriptDialogWaitState& s){if(&s!=&state_||s.vtable_address!=ScriptDialogs::kSelectVtable)throw std::invalid_argument("SelectDialog wait identity");}
unsigned NativeScriptSelectDialogRuntime::AddHandler(unsigned q,ScriptDialogWaitState* s){
    Executing();if(q!=ScriptExecutor()+ScriptDialogs::kQueueOffset||s!=&state_||active_||context_.Suspend()<0)
        throw std::runtime_error("SelectDialog cannot suspend invoking SDK context");
    active_=true;return 1u;
}
bool NativeScriptSelectDialogRuntime::IsVisible(unsigned f){if(!frame_||f!=frame_)throw std::invalid_argument("SelectDialog visibility owner");return services_.visible(f);}
void NativeScriptSelectDialogRuntime::CloseWait(ScriptDialogWaitState& s){if(&s!=&state_)throw std::invalid_argument("SelectDialog close wait owner");active_=false;}
bool NativeScriptSelectDialogRuntime::Tick(float){const bool was=active_;if(active_)dialogs_.UpdateSelect(state_);return was&&!active_;}
void NativeScriptSelectDialogRuntime::ClearHandlers(){if(asGetActiveContext()==&context_||document_)throw std::logic_error("Cannot retire executing SelectDialog/XML");active_=false;state_={};frame_=0u;}
unsigned NativeScriptSelectDialogRuntime::CreateDocument(){if(document_)throw std::logic_error("SelectDialog XML reentry");document_=xml_.InitializeXmlDocument();if(!document_)throw std::runtime_error("SelectDialog XML allocation");return document_;}
void NativeScriptSelectDialogRuntime::UpsertPointerAttribute(unsigned d,std::string_view key,unsigned f){
    if(d!=document_||!d||key!="frame"||f!=frame_)throw std::invalid_argument("SelectDialog XML frame owner");
    if(const auto a=xml_.FindAttribute(d,key);a&&*a)xml_.SetInt(*a-0x1cu,std::bit_cast<int>(f));
    else{const auto created=xml_.InitializeIntAttribute(key,std::bit_cast<int>(f));try{xml_.AddAttribute(d,created);}catch(...){xml_.DestroyAttribute(created);throw;}xml_.DestroyAttribute(created);}
}
void NativeScriptSelectDialogRuntime::EnterState(unsigned state,unsigned d){Executing();if(state!=10u||!document_||d!=document_||!active_)throw std::invalid_argument("SelectDialog ModalFrame entry");xml_.InitializeStateAndUi(xml_.GameInstance(),state,d);}
unsigned NativeScriptSelectDialogRuntime::DestroyDocument(unsigned d){if(!document_||d!=document_)throw std::invalid_argument("SelectDialog XML retirement");const auto v=static_cast<unsigned>(xml_.DestroyXmlDocument(d));document_=0u;return v;}
unsigned NativeScriptSelectDialogRuntime::CreateCommonDialog(unsigned){throw std::logic_error("SelectDialog adapter does not own CommonDialog");}
void NativeScriptSelectDialogRuntime::ShowCommon(std::string_view,int,int,unsigned,unsigned){throw std::logic_error("SelectDialog adapter does not own CommonDialog");}
}
