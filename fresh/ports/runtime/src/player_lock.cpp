#include "pal4/portable/player_lock.h"
#include "pal4/script_entity_callbacks.h"
#include <stdexcept>

namespace pal4::portable {
namespace {
class Runtime final:public pal4::IScriptPlayerLockRuntime {
public:
    PlayerRoster& players;ActorController& controller;SceneUi& ui;IControllerResetServices& services;
    const std::function<void(std::uint32_t,std::string_view)>& select;
    std::function<void(Point3,Point3)> update_grid;
    Runtime(PlayerRoster& p,ActorController& c,SceneUi& u,IControllerResetServices& s,
        const std::function<void(std::uint32_t,std::string_view)>& a):players(p),controller(c),ui(u),services(s),select(a){}
    std::uint32_t PlayerManager() override{return 0x10000;}
    std::uint32_t ReadWord(std::uint32_t address) override {
        if(address==0x12c50) {return players.Leader();}
        if(address==0x20004)return controller.State().actor?0x10000+*controller.State().actor*0xb14:0;
        throw std::logic_error("invalid player lock read");
    }
    void SetActionCode(std::uint32_t component,std::string_view code) override {
        if(component!=0x10000+players.Leader()*0xb14+0x170 || code!="C01")throw std::logic_error("invalid player lock action");
        players.SelectAction(players.Leader(),code);select(players.Leader(),code);
    }
    std::uint32_t ActorControl() override{return 0x20000;}
    void ResetAndSync(std::uint32_t control,std::uint32_t flag) override {
        if(control!=0x20000 || flag>1)throw std::logic_error("invalid player lock reset");
        controller.ResetAndSync(flag!=0,services);
    }
    std::uint32_t UiFrameManager() override{return 0x30000;}
    std::uint8_t ProcessUiEvents(std::uint32_t manager,std::uint8_t flag) override {
        if(manager!=0x30000 || flag>1)throw std::logic_error("invalid player lock UI event");
        ui.ShowScene(flag!=0);return 1;
    }
    std::uint32_t MinimapManager() override{return 0x40000;}
    void UpdateGrid(std::uint32_t manager,std::uint32_t position,std::uint32_t rotation) override {
        if(!controller.State().actor || manager!=0x40000 || !update_grid)throw std::logic_error("player unlock minimap owner is unavailable");
        const auto index=*controller.State().actor;
        const auto object=0x10000+index*0xb14;
        if(position!=object+0xac || rotation!=object+0xa0)throw std::logic_error("invalid player unlock grid handles");
        const auto& player=players.Slots()[index];update_grid(player.position,player.rotation);
    }
};
}
void LockPlayer(PlayerRoster& players,ActorController& controller,SceneUi& ui,IControllerResetServices& services,
    const std::function<void(std::uint32_t,std::string_view)>& select_animation) {
    if(!players.Prepared() || !controller.Initialized() || !select_animation)throw std::logic_error("player lock owners are unavailable");
    Runtime runtime(players,controller,ui,services,select_animation);(void)pal4::ScriptPlayerLockHandler::Lock(runtime);
}
void UnlockPlayer(PlayerRoster& players,ActorController& controller,SceneUi& ui,IControllerResetServices& services,
    const std::function<void(Point3,Point3)>& update_grid) {
    if(!players.Prepared() || !controller.Initialized() || !update_grid)throw std::logic_error("player unlock owners are unavailable");
    const std::function<void(std::uint32_t,std::string_view)> unused;
    Runtime runtime(players,controller,ui,services,unused);runtime.update_grid=update_grid;
    pal4::ScriptPlayerLockHandler::Unlock(runtime);
}
}
