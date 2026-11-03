#include "pal4/combat_scene_coordinator.h"
namespace pal4 {
namespace {
class Legacy final:public ICombatSceneServices {
    std::uint32_t object;ICombatSceneRuntime& r;std::uint32_t state{};bool state_read{};
    std::uint32_t State(){if(!state_read){state=r.GameState();state_read=true;}return state;}
public:
    Legacy(std::uint32_t o,ICombatSceneRuntime& runtime):object(o),r(runtime){}
    void EnableUi()override{r.SetUIFlag(r.UIFrameManager(),1u);}
    void HideConsole()override{r.WriteWord(r.InitializeConsole(),0u);}
    void HideScenarioDialog()override{r.HideScenarioDialog(r.CreateScenarioDialog(r.UIFrameManager()));}
    void CloseHint()override{r.CloseHintWindow(r.CreateHintWindow(r.UIFrameManager()));}
    std::uint32_t GameFlag(std::uint32_t o)override{return r.ReadWord(State()+o);}
    void SetGameFlag(std::uint32_t o,std::uint32_t v)override{r.WriteWord(State()+o,v);}
    std::uint32_t SavedFlag(std::uint32_t i)override{return r.ReadWord(object+4u+4u*i);}
    void SetSavedFlag(std::uint32_t i,std::uint32_t v)override{r.WriteWord(object+4u+4u*i,v);}
    void AssignRequest(std::uint32_t d)override{r.AssignXmlNode(object+12u,d);}
    void SetupCamera()override{r.SetupCombatCamera(r.CombatSystem());}
    bool ForceNormal()override{return r.ReadByte(r.CombatSystem()+501u)!=0u;}
    std::uint32_t ChoiceMode()override{return r.ReadWord(r.GlobalDataSubsystem());}
    void SetWaitingState()override{r.WriteWord(r.CombatSystem()+508u,0u);}
    void ShowChoice()override{
        auto callback=r.Allocate(12u);
        if(callback){r.WriteWord(callback+4u,0x58b270u);r.WriteWord(callback+8u,object);r.WriteWord(callback,CombatSceneCoordinator::kCallbackVtable);}
        try{
            const auto record=r.FindGameString(r.GameDatabase()+52u,110u);auto message=r.ReadWord(record+4u);
            if(!message)message=CombatSceneCoordinator::kEmptyMessage;
            const auto dialog=r.CreateCommonDialog(r.UIFrameManager());
            r.ShowCommonDialog(dialog,message,0.4f,0.4f,3u,callback);
        }catch(...){if(callback)r.DestroyCallback(callback,1u);throw;}
        if(callback)r.DestroyCallback(callback,1u);
    }
    void ActivateCamera(bool quick)override{
        const auto manager=r.CameraManager();const auto name=r.ReadWord(quick?0x8c1cf0u:0x8c1cf8u);r.ActivateCamera(manager,name);
    }
    std::uint8_t BeginCombat(std::uint32_t mode)override{return r.BeginCombat(r.CombatSystem(),object+12u,mode);}
    void CloseCombatUi()override{r.CloseCombatUI(r.CombatSystem());}
};
}
std::uint32_t CombatSceneCoordinator::EnterWithServices(std::uint32_t document,ICombatSceneServices& r){
    r.EnableUi();r.HideConsole();r.HideScenarioDialog();r.CloseHint();
    r.SetSavedFlag(0u,r.GameFlag(kGameStateFlagOffset));r.SetGameFlag(kGameStateFlagOffset,0u);
    r.HideConsole();r.SetSavedFlag(1u,r.GameFlag(kGameStateModeOffset));r.SetGameFlag(kGameStateModeOffset,1u);
    r.AssignRequest(document);r.SetupCamera();
    if(r.ForceNormal()){(void)StartWithServices(2u,r);return 1u;}
    if(r.ChoiceMode()!=2u){const auto selection=r.ChoiceMode()!=0u?1u:2u;(void)StartWithServices(selection,r);return 1u;}
    r.SetWaitingState();r.ShowChoice();return 1u;
}
std::uint8_t CombatSceneCoordinator::StartWithServices(std::uint32_t selection,ICombatSceneServices& r){
    const bool quick=selection==1u;r.ActivateCamera(quick);return r.BeginCombat(quick?1u:0u);
}
std::uint32_t CombatSceneCoordinator::LeaveWithServices(ICombatSceneServices& r){
    r.SetGameFlag(kGameStateFlagOffset,r.SavedFlag(0u));r.SetGameFlag(kGameStateModeOffset,r.SavedFlag(1u));
    r.CloseCombatUi();r.CloseHint();return 1u;
}
std::uint32_t CombatSceneCoordinator::Enter(std::uint32_t object,std::uint32_t,std::uint32_t document,ICombatSceneRuntime& runtime){
    Legacy services(object,runtime);return EnterWithServices(document,services);
}
std::uint8_t CombatSceneCoordinator::StartCombat(std::uint32_t object,std::uint32_t mode,ICombatSceneRuntime& runtime){
    Legacy services(object,runtime);return StartWithServices(mode,services);
}
std::uint32_t CombatSceneCoordinator::Leave(std::uint32_t object,std::uint32_t,ICombatSceneRuntime& runtime){return FinishLeave(object,runtime);}
std::uint32_t CombatSceneCoordinator::FinishLeave(std::uint32_t object,ICombatSceneRuntime& runtime){
    Legacy services(object,runtime);return LeaveWithServices(services);
}
}
