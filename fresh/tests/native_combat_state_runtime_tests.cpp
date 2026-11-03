#include "pal4/native_combat_state_runtime.h"
#include "pal4/native_combat_scene_update.h"
#include "pal4/native_combat_scene_lifecycle.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_combat_state_owner.h"
#include "pal4/native_pal_game_iv_state_list.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/combat_scene_coordinator.h"
#include "pal4/combat_scene_frame.h"
#include "pal4/combat_scene_update.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void Check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
std::uint32_t Ptr(const void* p){return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(p));}
struct Services final : pal4::ICombatSceneRuntime, pal4::ICombatSceneFrameRuntime,
    pal4::ICombatSystemRenderRuntime, pal4::ICombatSceneUpdateRuntime {
    pal4::NativeBaseObjectMemory memory;
    pal4::NativeEventDocumentRuntime docs{memory};
    pal4::NativeCombatStateOwner owner{memory,docs};
    std::array<std::uint32_t,400> game_state{};
    pal4::NativeCombatSystemImage combat;
    std::array<std::uint32_t,128> game{};
    std::uint32_t console{},global{},current_camera{},frame_value{42u};
    std::uint32_t scene_document{},next{},validations{},begin_mode{};
    bool ready{true},inactive{},throw_update{},throw_tree{};
    std::vector<int> calls;
    pal4::INativeCombatStateDispatch* dispatch{};
    Services(){
        (void)owner.Construct();
        game_state[1412u/4u]=9;game_state[1364u/4u]=8;
        scene_document=docs.Allocate(60u);(void)docs.InitializeEmptyDocument(scene_document);
    }
    ~Services(){if(scene_document){docs.DestroyDocument(scene_document);(void)docs.Free(scene_document);}}
    std::uint32_t ReadWord(std::uint32_t a) override {
        if(a==0x8c1cf0u)return 101u;
        if(a==0x8c1cf8u)return 102u;
        if(a==pal4::CombatSceneUpdate::kMessageGlobal)return Ptr("CombatResult");
        return memory.ReadDword(a);
    }
    std::uint8_t ReadByte(std::uint32_t a) override{return memory.ReadByte(a);}
    void WriteWord(std::uint32_t a,std::uint32_t v) override{memory.WriteDword(a,v);}
    std::uint32_t Allocate(std::uint32_t n) override{return memory.Allocate(n);}
    std::uint32_t UIFrameManager() override{return 1u;}
    void SetUIFlag(std::uint32_t,std::uint32_t flag) override{Check(flag==1,"entry UI flag");calls.push_back(1);}
    std::uint32_t InitializeConsole() override{return Ptr(&console);}
    std::uint32_t CreateScenarioDialog(std::uint32_t) override{return 2u;}
    void HideScenarioDialog(std::uint32_t) override{calls.push_back(2);}
    std::uint32_t CreateHintWindow(std::uint32_t) override{return 3u;}
    void CloseHintWindow(std::uint32_t) override{calls.push_back(3);}
    std::uint32_t GameState() override{return Ptr(game_state.data());}
    void AssignXmlNode(std::uint32_t d,std::uint32_t s) override{docs.AssignDocument(d,s);calls.push_back(4);}
    std::uint32_t CombatSystem() override{return Ptr(&combat);}
    void SetupCombatCamera(std::uint32_t) override{calls.push_back(5);}
    std::uint32_t GlobalDataSubsystem() override{return Ptr(&global);}
    std::uint32_t GameDatabase() override{throw std::logic_error("unused dialog fixture");}
    std::uint32_t FindGameString(std::uint32_t,std::uint32_t) override{throw std::logic_error("unused dialog fixture");}
    std::uint32_t CreateCommonDialog(std::uint32_t) override{throw std::logic_error("unused dialog fixture");}
    void ShowCommonDialog(std::uint32_t,std::uint32_t,float,float,std::uint32_t,std::uint32_t&) override{throw std::logic_error("unused dialog fixture");}
    void DestroyCallback(std::uint32_t a,std::uint32_t) override{(void)memory.Free(a);}
    std::uint32_t CameraManager() override{return 4u;}
    void ActivateCamera(std::uint32_t,std::uint32_t n) override{current_camera=n;calls.push_back(6);}
    std::uint8_t BeginCombat(std::uint32_t c,std::uint32_t doc,std::uint32_t mode) override{
        Check(c==CombatSystem() && doc==owner.Document() && docs.ChildCount(doc)==1,"owned request reaches start");
        begin_mode=mode;combat.State()=1;calls.push_back(7);return 1;
    }
    void CloseCombatUI(std::uint32_t) override{calls.push_back(8);}
    bool IsCombatSystemInactive(std::uint32_t) override{return inactive;}
    std::uint32_t ArenaManager() override{return 5;}
    void ProcessArenaFrame(std::uint32_t,std::uint32_t v) override{Check(v==42,"arena frame word");calls.push_back(10);}
    void PlaySound(std::uint32_t,std::uint32_t,std::uint32_t) override{throw std::logic_error("unused sound fixture");}
    void RenderWorld(std::uint32_t) override{calls.push_back(11);}
    void RenderGameObject(std::uint32_t,std::uint32_t) override{calls.push_back(12);}
    void RenderActors(std::uint32_t v) override{Check(v==42,"actor frame word");calls.push_back(13);}
    void RenderUi(std::uint32_t) override{calls.push_back(14);}
    void RenderConsole(std::uint32_t v) override{Check(v==42,"console frame word");calls.push_back(15);}
    std::uint32_t GetGameState() override{return GameState();}
    std::uint32_t CountEffects(std::uint32_t) override{throw std::logic_error("unused debug fixture");}
    std::string GetBgmName() override{throw std::logic_error("unused debug fixture");}
    std::string ReadCString(std::uint32_t) override{throw std::logic_error("unused debug fixture");}
    void DrawText(std::string_view,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) override{throw std::logic_error("unused debug fixture");}
    void AdvanceCombat(std::uint32_t,float delta) override{
        Check(delta==0.25f,"original frame delta");calls.push_back(20);
        if(throw_update)throw std::runtime_error("frame failure");
    }
    void UpdateUI(std::uint32_t,double delta) override{Check(delta==0.25,"UI delta");calls.push_back(21);}
    std::uint32_t ConstructXmlDocument() override{
        auto d=docs.Allocate(60);(void)docs.InitializeEmptyDocument(d);
        docs.WriteIntAttribute(d,"CombatResult",-1);return d;
    }
    std::uint32_t DestroyXmlDocument(std::uint32_t d) override{docs.DestroyDocument(d);(void)docs.Free(d);return 99;}
    std::uint32_t GetConfigStringValue(std::uint32_t d,std::uint32_t key) override{return docs.FindNumericAttribute(d,reinterpret_cast<const char*>(key));}
    std::uint32_t FindTreeAttribute(std::uint32_t document,std::uint32_t key) override{
        Check(document!=scene_document && document!=owner.Document(),
              "alternate result searches its temporary document, not retained trees");
        if(throw_tree)throw std::runtime_error("tree lookup failure");
        const auto attribute=docs.FindNumericAttribute(document,reinterpret_cast<const char*>(key));
        return attribute ? attribute-28u : 0u;
    }
    void SetXmlAttributeInt(std::uint32_t a,std::uint32_t v) override{docs.SetAttributeIntValue(a,static_cast<std::int32_t>(v));}
    std::uint32_t ConstructXmlAttribute(std::uint32_t,std::uint32_t) override{throw std::logic_error("unused absent attribute fixture");}
    void AddXmlAttribute(std::uint32_t,std::uint32_t) override{throw std::logic_error("unused absent attribute fixture");}
    void DestroyXmlAttribute(std::uint32_t) override{throw std::logic_error("unused absent attribute fixture");}
    std::uint32_t ScriptExecutor() override{return 0x5000;}
    void CleanupScriptCache(std::uint32_t a) override{Check(a==0x5054,"failure cleanup address");calls.push_back(22);}
    std::uint32_t GameInstance() override{return Ptr(game.data());}
    void InitializeStateAndUI(std::uint32_t g,std::uint32_t n,std::uint32_t d) override{
        Check(g==GameInstance(),"transition game identity");next=n;
        dispatch->Leave(n); // Real transition reenters Leave synchronously during Update.
        docs.AssignDocument(scene_document,d);calls.push_back(23);
    }
};
template<class F> void Reject(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}Check(rejected,"expected rejection");}
}
int main(){try{
    Services s;
    Reject([&]{pal4::NativeCombatStateRuntime missing(s.owner,s,s,s,s,{});});
    pal4::NativeCombatStateRuntime runtime(s.owner,s,s,s,s,[&]{++s.validations;if(!s.ready)throw std::logic_error("unbound dependency");});
    s.dispatch=&runtime;
    pal4::NativePalGameIVStateList states(s.GameInstance());
    Check(states.Register(runtime.Address(),3,[&]{s.owner.Destroy();}) && states.Find(3)==runtime.Address(),"registered owner identity");
    const auto source=s.docs.Allocate(60);(void)s.docs.InitializeEmptyDocument(source);
    s.docs.WriteStringAttribute(source,"CombatArea","M01");(void)s.docs.CreateChild(source,"CbMonster");
    s.ready=false;auto count=s.docs.DocumentCount();
    Reject([&]{s.dispatch->Enter(1,source);});
    Check(s.calls.empty() && s.game_state[1412/4]==9 && s.docs.DocumentCount()==count,"preflight failure before entry mutations");
    Reject([&]{s.dispatch->Render(Ptr(&s.frame_value));});
    Reject([&]{(void)s.dispatch->Update(0.25f);});
    Reject([&]{s.dispatch->Leave(1);});
    Check(s.calls.empty(),"all runtime boundaries validate dependencies before mutation");
    s.ready=true;
    Check(s.dispatch->Enter(1,source),"shared native entry");
    s.docs.DestroyDocument(source);(void)s.docs.Free(source);
    Check(s.current_camera==102 && s.begin_mode==0 && s.memory.ReadDword(runtime.Address()+4)==9 &&
          s.memory.ReadDword(runtime.Address()+8)==8 && s.game_state[1364/4]==1 &&
          s.docs.AttributeToString(s.docs.FindNumericAttribute(s.owner.Document(),"CombatArea"))=="M01" &&
          s.docs.ChildCount(s.owner.Document())==1,"native request survives temporary root and flags saved");
    s.calls.clear();s.dispatch->Render(Ptr(&s.frame_value));
    Check(s.calls==std::vector<int>{11,12,13,14,15},"shared active rendering order");
    class FrameRenderer final : public pal4::ICombatSystemFrameRenderer {
        Services& services;
    public:
        explicit FrameRenderer(Services& s):services(s){}
        void Render(std::uint32_t f)override{pal4::CombatSystemRender::Render(services.CombatSystem(),f,services);}
    } frame_renderer(s);
    pal4::NativeCombatStateRuntime typed(s.owner,s,s,frame_renderer,s,[&]{++s.validations;if(!s.ready)throw std::logic_error("unbound dependency");});
    s.calls.clear();typed.Render(Ptr(&s.frame_value));
    Check(s.calls==std::vector<int>{11,12,13,14,15},"typed renderer runs through registered native state composition");
    s.ready=false;s.calls.clear();Reject([&]{typed.Render(Ptr(&s.frame_value));});
    Check(s.calls.empty(),"typed renderer validates before scene or drawing");s.ready=true;
    s.calls.clear();s.combat.State()=0;s.dispatch->Render(Ptr(&s.frame_value));
    Check(s.calls==std::vector<int>{10,14,15},"zero state renders underlying arena before UI");
    s.combat.State()=1;s.throw_update=true;s.calls.clear();
    Reject([&]{(void)s.dispatch->Update(0.25f);});
    Check(s.calls==std::vector<int>{20},"failed combat frame propagates before UI");
    s.throw_update=false;s.calls.clear();Check(s.dispatch->Update(0.25f)==1 && s.calls==std::vector<int>{20,21},"ordinary update no outcome transition");
    s.combat.State()=7;s.combat.session_storage[27]=1;s.memory.WriteByte(s.CombatSystem()+500,0);
    Check(s.dispatch->Update(0.25f)==99 && s.next==1 && s.game_state[1412/4]==9 && s.game_state[1364/4]==8 &&
        s.docs.AttributeToInt(s.docs.FindNumericAttribute(s.scene_document,"CombatResult"),-1)==1,"outcome XML cloned before destruction and leave restores flags");
    s.memory.WriteByte(s.CombatSystem()+500,1);
    s.docs.WriteIntAttribute(s.owner.Document(),"CombatResult",-77);
    s.docs.WriteIntAttribute(s.scene_document,"CombatResult",-55);
    const auto retained_count=s.docs.DocumentCount();
    s.throw_tree=true;s.calls.clear();
    Reject([&]{(void)s.dispatch->Update(0.25f);});
    Check(s.docs.DocumentCount()==retained_count && s.calls==std::vector<int>{20,21} &&
          s.docs.AttributeToInt(s.docs.FindNumericAttribute(s.owner.Document(),"CombatResult"),0)==-77 &&
          s.docs.AttributeToInt(s.docs.FindNumericAttribute(s.scene_document,"CombatResult"),0)==-55,
          "failed outcome lookup releases temporary XML without touching retained documents or transitioning");
    s.throw_tree=false;
    Check(s.dispatch->Update(0.25f)==99 && s.next==1 && s.docs.DocumentCount()==retained_count &&
          s.docs.AttributeToInt(s.docs.FindNumericAttribute(s.owner.Document(),"CombatResult"),0)==-77 &&
          s.docs.AttributeToInt(s.docs.FindNumericAttribute(s.scene_document,"CombatResult"),0)==1,
          "alternate success writes the new XML and clones it before destruction, preserving request tree");
    s.combat.session_storage[27]=0;
    Check(s.dispatch->Update(0.25f)==99 && s.next==7,"alternate loss routes failure state");
    pal4::NativeCombatSceneUpdate outcome(s.combat,s.docs,{
        [&s](float dt){s.AdvanceCombat(s.CombatSystem(),dt);},
        [&s](double dt){s.UpdateUI(s.UIFrameManager(),dt);},
        [&s]{s.CleanupScriptCache(s.ScriptExecutor()+0x54u);},
        [&s](auto next,auto xml){s.InitializeStateAndUI(s.GameInstance(),next,xml);}});
    pal4::NativeCombatStateRuntime native(s.owner,s,s,frame_renderer,outcome,[&]{++s.validations;if(!s.ready)throw std::logic_error("unbound dependency");});
    s.dispatch=&native;s.memory.WriteByte(s.CombatSystem()+500u,0u);s.combat.session_storage[27]=2u;
    s.calls.clear();const auto native_count=s.docs.DocumentCount();
    Check(native.Update(0.25f)==0u && s.next==1u && s.docs.DocumentCount()==native_count &&
          s.docs.AttributeToInt(s.docs.FindNumericAttribute(s.scene_document,"CombatResult"),-1)==1,
          "registered state composes actual native outcome XML and synchronous Leave before temporary release");
    s.ready=false;s.calls.clear();Reject([&]{(void)native.Update(0.25f);});
    Check(s.calls.empty(),"native outcome dispatcher validates services before any frame mutation");s.ready=true;
    // Exercise the same lifecycle interface implemented by NativeCombatSceneEntry.
    // Domain/UI services remain fixtures; the dispatcher must not perform a
    // second raw Entry/Leave or block outcome's synchronous nested Leave.
    class Lifecycle final : public pal4::INativeCombatSceneLifecycle {
        Services& services;
    public:
        unsigned entries{},leaves{};bool accept{true},fail{},active{},wrong_owner{};
        std::function<void()> leaving;
        explicit Lifecycle(Services& s):services(s){}
        std::uint32_t Address()const override{return services.owner.Address()+(wrong_owner?4u:0u);}
        bool Enter(std::uint32_t document)override{
            ++entries;
            if(fail)throw std::runtime_error("typed entry failure");
            if(!accept)return false;
            Check(!active,"typed lifecycle entered once");
            active=pal4::CombatSceneCoordinator::Enter(services.owner.Address(),1u,document,services)!=0u;
            return active;
        }
        void Leave()override{
            Check(active,"typed lifecycle has an active entry");
            if(leaving)leaving();
            (void)pal4::CombatSceneCoordinator::Leave(services.owner.Address(),1u,services);
            active=false;++leaves;
        }
    } lifecycle(s);
    pal4::NativeCombatStateRuntime lifecycle_dispatch(s.owner,lifecycle,s,frame_renderer,outcome,
        [&]{++s.validations;if(!s.ready)throw std::logic_error("unbound dependency");});
    lifecycle.wrong_owner=true;s.calls.clear();
    Reject([&]{(void)lifecycle_dispatch.Enter(1u,s.owner.Document());});
    Reject([&]{lifecycle_dispatch.Render(Ptr(&s.frame_value));});
    Reject([&]{(void)lifecycle_dispatch.Update(0.25f);});
    Reject([&]{lifecycle_dispatch.Leave(1u);});
    Check(!lifecycle.entries && !lifecycle.leaves && s.calls.empty(),"foreign lifecycle owner rejected before any domain mutation");
    lifecycle.wrong_owner=false;
    s.dispatch=&lifecycle_dispatch;
    Check(lifecycle_dispatch.Address()==states.Find(3u),"typed lifecycle preserves registered owner identity");
    const auto request=s.docs.Allocate(60u);(void)s.docs.InitializeEmptyDocument(request);
    s.docs.WriteStringAttribute(request,"CombatArea","M01");(void)s.docs.CreateChild(request,"CbMonster");
    s.ready=false;s.calls.clear();
    Reject([&]{(void)lifecycle_dispatch.Enter(1u,request);});
    Reject([&]{lifecycle_dispatch.Leave(1u);});
    Check(!lifecycle.entries && !lifecycle.leaves && s.calls.empty(),"typed lifecycle preflight prevents any Entry or Leave call");
    s.ready=true;
    Reject([&]{(void)lifecycle_dispatch.Enter(1u,0u);});
    Check(!lifecycle.entries,"typed null request rejected before lifecycle");
    lifecycle.accept=false;
    Check(!lifecycle_dispatch.Enter(1u,request) && lifecycle.entries==1u && !lifecycle.active,
          "typed entry rejection is propagated without synthetic success");
    lifecycle.accept=true;lifecycle.fail=true;
    Reject([&]{(void)lifecycle_dispatch.Enter(1u,request);});
    Check(lifecycle.entries==2u && !lifecycle.active,"typed entry exception propagates");lifecycle.fail=false;
    const auto saved_flag=s.game_state[1412u/4u],saved_mode=s.game_state[1364u/4u];
    Check(lifecycle_dispatch.Enter(1u,request) && lifecycle.entries==3u && lifecycle.active,"typed entry reaches shared actual coordinator once");
    s.docs.DestroyDocument(request);(void)s.docs.Free(request);
    Check(s.docs.ChildCount(s.owner.Document())==1u,"typed Entry owns XML after caller releases request");
    s.calls.clear();lifecycle_dispatch.Render(Ptr(&s.frame_value));
    Check(s.calls==std::vector<int>{11,12,13,14,15},"typed lifecycle uses existing full rendering algorithm");
    s.throw_update=true;Reject([&]{(void)lifecycle_dispatch.Update(0.25f);});s.throw_update=false;
    Check(lifecycle.active && !lifecycle.leaves,"failed update does not synthesize Leave");
    s.combat.State()=7u;s.combat.session_storage[27]=1u;
    unsigned nested_updates{};
    lifecycle.leaving=[&]{Reject([&]{(void)lifecycle_dispatch.Update(0.25f);});++nested_updates;};
    const auto before_return=s.docs.DocumentCount();
    Check(lifecycle_dispatch.Update(0.25f)==0u && lifecycle.leaves==1u && !lifecycle.active && nested_updates==1u &&
          s.next==1u && s.docs.DocumentCount()==before_return && s.game_state[1412u/4u]==saved_flag &&
          s.game_state[1364u/4u]==saved_mode && s.docs.AttributeToInt(s.docs.FindNumericAttribute(s.scene_document,"CombatResult"),-1)==1,
          "typed synchronous outcome Leave restores flags and clones XML while recursive Update is rejected");
    states.Clear();
    Reject([&]{(void)s.dispatch->Update(0.25f);});
    Check(!runtime.Address() && s.docs.DocumentCount()==1 && states.OutstandingAllocations()==0,"retired owner invalidates borrowed dispatcher");
    s.docs.DestroyDocument(s.scene_document);(void)s.docs.Free(s.scene_document);s.scene_document=0;
    Check(s.docs.DocumentCount()==0 && s.memory.OutstandingAllocations()==0,"native state and temporary XML allocations retire completely");
    std::cout<<"Native Combat state: real registry/XML, preflight, Entry/Render/Update/Leave composition and synchronous outcome transition pass; domain services are protocol fixtures\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
