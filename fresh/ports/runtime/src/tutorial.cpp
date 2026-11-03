#include "pal4/portable/tutorial.h"
#include "pal4/portable/gbk.h"
#include "pal4/script_tutorial_wait.h"
#include "pal4/ui_frame_manager_singleton.h"
#include <bit>
#include <bitset>
#include <istream>
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool valid,const char* message){if(!valid)throw std::runtime_error(message);}
}
TutorialCatalog ReadTutorialCatalog(std::istream& input){return ReadTextCatalog(input,"P4TU");}
struct TutorialServices::Impl final:pal4::IScriptTutorialWaitRuntime,pal4::IUiFrameManagerTutorialRuntime {
    static constexpr unsigned manager=0x100,global=0x200,frame=0x300,executor=0x400;
    IScriptContextServices& next;const TutorialCatalog& catalog;std::uint64_t generation;bool enabled;SaveFlag save;
    std::bitset<128> flags;TutorialView view;bool frame_exists{},handler{},closed{};
    pal4::TutorialWaitState wait;std::optional<ScriptToken> token;
    Impl(IScriptContextServices& n,const TutorialCatalog& c,std::uint64_t g,bool e,SaveFlag s):next(n),catalog(c),generation(g),enabled(e),save(std::move(s)){Require(bool(save),"tutorial settings writer is missing");view.generation=g;}
    unsigned UiFrameManager() override{return manager;}
    void ShowTutorial(unsigned owner,unsigned key) override{Require(owner==manager,"invalid tutorial UI owner");pal4::UiFrameManagerSingleton::ShowTutorialDialog(manager,key,*this);}
    unsigned FindFrame(unsigned owner,std::string_view name) override{Require(owner==manager && name==pal4::ScriptTutorialWait::kFrameName,"invalid tutorial frame lookup");return frame_exists?frame:0;}
    unsigned FindFrame(unsigned owner,const char* name) override{return FindFrame(owner,std::string_view(name));}
    pal4::TutorialWaitState* AllocateWait(unsigned size) override{Require(size==sizeof(wait) && !handler,"invalid tutorial wait allocation");wait={};return &wait;}
    unsigned ScriptExecutor() override{return executor;}
    void ConstructWait(pal4::TutorialWaitState& state,std::uint8_t mode,unsigned queue) override{Require(&state==&wait && mode==1 && queue==executor+pal4::ScriptTutorialWait::kQueueOffset,"invalid tutorial wait owner");closed=false;}
    void FlushWait(pal4::TutorialWaitState& state) override{Require(&state==&wait && state.vtable_address==pal4::ScriptTutorialWait::kWaitVtable,"invalid tutorial wait metadata");}
    unsigned AddHandler(unsigned queue,pal4::TutorialWaitState* state) override{Require(queue==executor+pal4::ScriptTutorialWait::kQueueOffset && state==&wait,"invalid tutorial wait registration");handler=true;return 1;}
    bool IsVisible(unsigned owner) override{Require(owner==frame && frame_exists,"invalid tutorial visibility owner");return view.visible;}
    void CloseWait(pal4::TutorialWaitState& state) override{Require(&state==&wait,"invalid tutorial close owner");closed=true;}
    unsigned InitializeGlobalData() override{return global;}
    unsigned ReadWord(unsigned address) override{Require(address==global+8,"invalid tutorial option address");return enabled;}
    bool GetDialogFlag(unsigned owner,unsigned key) override{Require(owner==global,"invalid tutorial flags owner");return key<flags.size() && flags.test(key);}
    unsigned ManagerSingleton() override{return manager;}
    void RegisterFrame(unsigned owner,const char* name) override{Require(owner==manager && std::string_view(name)==pal4::ScriptTutorialWait::kFrameName,"invalid tutorial registration");frame_exists=true;}
    void SetDialogFlagAndSave(unsigned owner,unsigned key) override{Require(owner==global,"invalid tutorial flag save owner");if(key<flags.size()){save(key);flags.set(key);}}
    void UpdateVisibility(unsigned owner,unsigned visible,unsigned key,unsigned parent) override{
        Require(owner==frame && visible==1 && parent==0,"invalid tutorial show request");
        const auto found=catalog.find(key);Require(found!=catalog.end(),"original tutorial record is missing");
        view.source=found->second;view.key=key;view.visible=true;++view.id;++view.revision;
    }
};
TutorialServices::TutorialServices(IScriptContextServices& next,const TutorialCatalog& catalog,std::uint64_t generation,bool enabled,SaveFlag save):impl_(std::make_unique<Impl>(next,catalog,generation,enabled,std::move(save))){}
TutorialServices::~TutorialServices()=default;
void TutorialServices::OnCall(const ScriptRequest& request){impl_->next.OnCall(request);}
ScriptResponse TutorialServices::Invoke(const ScriptRequest& request){
    auto& s=*impl_;if(request.call.system_id!=309)return s.next.Invoke(request);
    Require(request.call.arguments.size()==1 && std::holds_alternative<std::int32_t>(request.call.arguments[0]),"invalid tutorial arguments");
    Require(!s.token && !s.handler,"tutorial request already waiting");
    const auto key=std::bit_cast<unsigned>(std::get<std::int32_t>(request.call.arguments[0]));
    pal4::ScriptTutorialWait original(s);(void)original.Show(key);s.token=request.token;s.view.waiting=true;return {{},true};
}
TutorialView TutorialServices::View() const{return impl_->view;}
bool TutorialServices::ShowFromUi(std::uint32_t key){auto& s=*impl_;const auto revision=s.view.revision;pal4::UiFrameManagerSingleton::ShowTutorialDialog(Impl::manager,key,s);return s.view.revision!=revision;}
bool TutorialServices::Close(std::uint64_t generation,std::uint64_t id){auto& s=*impl_;if(!s.view.visible || s.generation!=generation || s.view.id!=id)return false;s.view.visible=false;++s.view.revision;return true;}
std::optional<ScriptToken> TutorialServices::Advance(){auto& s=*impl_;if(!s.token)return {};pal4::ScriptTutorialWait(s).Update(s.wait);if(!s.closed)return {};const auto token=s.token;s.token.reset();s.handler=false;s.view.waiting=false;return token;}
void TutorialServices::Cancel(){auto& s=*impl_;s.token.reset();s.handler=false;s.closed=false;s.view.waiting=s.view.visible=false;++s.view.revision;}
}
