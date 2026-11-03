#include "pal4/portable/controller.h"
#include "pal4/portable/movement_input_gate.h"
#include "pal4/portable/scene_ui.h"
#include <bit>
#include <cmath>
#include <limits>
#include <source_location>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
using namespace pal4::portable;
void Check(bool value,const std::source_location where=std::source_location::current()) {if(!value)throw std::runtime_error("controller contract failed at line "+std::to_string(where.line()));}
struct Services final:IControllerResetServices {
    SceneUi ui;
    bool stopped{};unsigned inputs{};
    const ControllerState* observed{};bool enabled_at_cursor{};
    std::string order;
    void SetDefaultCursor() override{ui.cursor="default.cur";order+='C';if(observed)enabled_at_cursor=observed->enabled;}
    void ClearPlatformInput() override{++inputs;order+='I';}
    void HideInteractionUi() override{ui.HideInteraction();order+='H';}
    void SetMonsterPursuitStopped(bool value) override{stopped=value;order+='M';}
};
Actor Dummy() {
    const Matrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    Actor actor;actor.frames.push_back({-1,identity});actor.palettes.push_back({0,identity});
    actor.mesh.vertices.resize(3);
    actor.influences.resize(3,{{0,0,0,0},{1,0,0,0}});return actor;
}
}
int main(int argc,char** argv) {
    try {
        {MovementInputGate gate;float x=0,y=0;gate.Arm(0,0);gate.Apply(1,0,x,y);Check(x==1 && !gate.Waiting());
            gate.Arm(x,y);gate.Apply(1,1,x,y);Check(gate.Waiting() && x==0 && y==0);
            gate.Apply(0,0,x,y);Check(!gate.Waiting() && x==0 && y==0);gate.Apply(0,-1,x,y);Check(y==-1);
            gate.Arm(x,y);gate.Reset();gate.Apply(1,0,x,y);Check(x==1);}
        ActorController controller;
        if(argc==3) {
            controller.Initialize(1280,720,[&]{std::ifstream input(argv[1],std::ios::binary);return ReadActor(input);});
            std::ofstream output(argv[2]);WriteControllerReport(output,controller);return 0;
        }
        Check(!controller.Initialized() && !controller.FloorCallbacksRegistered());
        bool failed=false;try{controller.ClassifyFloor(0,"WF011");}catch(const std::logic_error&){failed=true;}Check(failed);
        unsigned loads{};controller.Initialize(800,600,[&]{++loads;return Dummy();});
        Check(controller.Initialized() && loads==1 && controller.FloorCallbacksRegistered());
        const auto& s=controller.State();
        Check(!s.enabled && !s.actor && !s.database.actor && !s.motion.active && !s.motion.pending && s.motion.reached && s.motion.mode==0);
        Check(s.input.result==0 && s.input.event_mode==1 && s.input.mode==1 && s.input.multiplier==1 && s.motion.speed==40);
        Check(std::bit_cast<std::uint32_t>(s.input.speed)==0x4329e666u && std::bit_cast<std::uint32_t>(s.input.secondary_speed)==0x42fdcccdu);
        Check(s.input.turn_scale==static_cast<float>(270.0/800) && s.vertical_scale==static_cast<float>(40.0/600) && s.horizontal_scale==.15f);
        Check(s.database.flags==std::array<std::uint32_t,7>{0,0,0,1,1,0,1});
        for(unsigned foot=0;foot<2;++foot) {
            for(const auto* code:{"WF011","WF013","WF015","WF017","WF023"})Check(controller.ClassifyFloor(foot,code)==1);
            for(const auto* code:{"WF041","WF037"})Check(controller.ClassifyFloor(foot,code)==2);
            for(const auto* code:{"WF021","WF039"})Check(controller.ClassifyFloor(foot,code)==3);
            Check(controller.ClassifyFloor(foot,"wf011")==0 && controller.ClassifyFloor(foot,"WF039_WF011")==1);
            Check(controller.ClassifyFloor(foot,std::string_view("foo\0WF011",9))==0);
        }
        failed=false;try{controller.ClassifyFloor(2,"WF011");}catch(const std::logic_error&){failed=true;}Check(failed);
        failed=false;try{controller.Initialize(1280,720,[]{throw std::runtime_error("load error");return Dummy();});}
        catch(const std::runtime_error&){failed=true;}Check(failed && !controller.Initialized() && !controller.FloorCallbacksRegistered());
        failed=false;try{controller.Initialize(0,720,[]{return Dummy();});}catch(const std::invalid_argument&){failed=true;}Check(failed);
        controller.Initialize(1280,720,[]{return Dummy();});
        Check(controller.State().input.turn_scale==static_cast<float>(270.0/1280));
        Check(controller.SetMovementMode(0)==0 && controller.State().input.mode==0);
        Check(std::bit_cast<unsigned>(controller.State().input.multiplier)==0x3ecccccdU);
        Check(std::bit_cast<unsigned>(controller.State().input.speed)==0x4287eb85U);
        Check(controller.SetMovementMode(1)==0 && std::bit_cast<unsigned>(controller.State().input.speed)==0x4329e666U);
        Check(controller.SetMovementMode(2)==0 && std::bit_cast<unsigned>(controller.State().input.speed)==0x437ed999U);
        Check(controller.SetMovementMode(0xffffffffU)==0xfffffffdU && controller.State().input.mode==0xffffffffU);
        Check(controller.State().input.multiplier==1.5f && std::bit_cast<unsigned>(controller.State().input.speed)==0x437ed999U);
        controller.SetMovementMode(1);
        controller.SelectActor(0,1);Check(controller.State().actor==0 && controller.State().database.actor==0 && controller.State().input.event_mode==0);
        controller.SelectActor(1,0);Check(controller.State().actor==1 && controller.State().input.event_mode==1);
        {
            ActorController free;free.Initialize(1280,720,Dummy);free.SelectActor(0,1);
            Services reset;ControlCameraBasis basis{{1,0,0},{0,.3f,1}};
            const auto near=[](std::array<float,3> a,std::array<float,3> b){for(unsigned i=0;i<3;++i)if(std::abs(a[i]-b[i])>1e-5f)return false;return true;};
            FreeControlInput keys;keys.forward=true;unsigned dispatched{};std::array<float,3> last{};
            const auto dispatch=[&](std::array<float,3> direction,float elapsed){Check(elapsed==.125f);last=direction;++dispatched;};
            Check(!free.UpdateFreeInput(keys,basis,{0,0,0},.125f,dispatch) && dispatched==0);
            free.ResetAndSync(true,reset);
            Check(free.UpdateFreeInput(keys,basis,{0,0,0},.125f,dispatch) && dispatched==1 && near(last,{0,0,1}));
            keys.backward=true;
            Check(free.UpdateFreeInput(keys,basis,{0,0,0},.125f,dispatch) && near(last,{0,0,-1}));
            keys={};Check(!free.UpdateFreeInput(keys,basis,{0,0,0},.125f,dispatch) && dispatched==2);
            Check(free.State().input.pending[0]==0 && free.State().input.pending[1]==0 && free.State().input.pending[2]==0);
            keys.forward=keys.right=true;
            Check(free.UpdateFreeInput(keys,basis,{0,0,0},.125f,dispatch));
            Check(std::abs(last[0]-.70710678f)<.001f && last[1]==0 && std::abs(last[2]-.70710678f)<.001f);
            keys={};keys.forward=true;keys.mouse_world=std::array<float,3>{1,99,0};
            Check(free.UpdateFreeInput(keys,basis,{0,0,0},.125f,dispatch) && near(last,{0,0,1}));
            keys.mouse_world=std::array<float,3>{1000,99,0};
            Check(free.UpdateFreeInput(keys,basis,{0,0,0},.125f,dispatch) && near(last,{1,0,0}));
            keys={};Check(!free.UpdateFreeInput(keys,basis,{0,0,0},.125f,dispatch));
            keys.mouse_world=std::array<float,3>{17,0,0};const auto before_threshold=dispatched;
            Check(!free.UpdateFreeInput(keys,basis,{0,0,0},.125f,dispatch) && dispatched==before_threshold);
            keys.mouse_world=std::array<float,3>{19,0,0};
            Check(free.UpdateFreeInput(keys,basis,{0,0,0},.125f,dispatch) && near(last,{1,0,0}));
            keys={};Check(!free.UpdateFreeInput(keys,basis,{0,0,0},.125f,dispatch));
            keys.left=true;ControlCameraBasis rotated{{0,0,-1},{1,0,0}};
            Check(free.UpdateFreeInput(keys,rotated,{0,0,0},.125f,dispatch) && near(last,{0,0,1}));
            bool rejected=false;try{free.UpdateFreeInput(keys,basis,{0,0,0},std::numeric_limits<float>::quiet_NaN(),dispatch);}catch(const std::invalid_argument&){rejected=true;}Check(rejected);
            free.SelectActor(0,0);rejected=false;try{free.UpdateFreeInput(keys,basis,{0,0,0},.125f,dispatch);}catch(const std::logic_error&){rejected=true;}Check(rejected);
            free.ResetAndSync(false,reset);const auto count=dispatched;
            Check(!free.UpdateFreeInput(keys,basis,{0,0,0},.125f,{}) && dispatched==count);
        }
        {
            ActorController events;events.Initialize(1280,720,Dummy);events.SelectActor(0,0);
            Services reset;FreeControlInput input;input.forward=true;std::vector<unsigned> commands;
            const auto dispatch=[&](std::uint32_t command,float elapsed){Check(elapsed==.125f);commands.push_back(command);};
            Check(!events.UpdateEventInput(input,.125f,dispatch) && commands.empty());
            events.ResetAndSync(true,reset);
            input.backward=input.right=input.left=true;
            Check(events.UpdateEventInput(input,.125f,dispatch) && commands==std::vector<unsigned>({1,3}));
            commands.clear();input={};input.backward=input.left=true;
            Check(events.UpdateEventInput(input,.125f,dispatch) && commands==std::vector<unsigned>({2,4}));
            commands.clear();input={};Check(!events.UpdateEventInput(input,.125f,dispatch) && commands.empty());
            input.mouse_world=std::array<float,3>{};bool rejected=false;
            try{events.UpdateEventInput(input,.125f,dispatch);}catch(const std::logic_error&){rejected=true;}Check(rejected);
            events.SelectActor(0,1);rejected=false;input={};
            try{events.UpdateEventInput(input,.125f,dispatch);}catch(const std::logic_error&){rejected=true;}Check(rejected);
        }
        auto& dirty=const_cast<ControllerState&>(controller.State());
        dirty.input.result=7;dirty.input.pending={1,2,3,4};dirty.input_vector={5,6,7};dirty.transient.fill(9);
        dirty.motion.active=dirty.motion.pending=true;dirty.motion.reached=false;dirty.motion.mode=1;dirty.motion.speed=123;
        dirty.motion.target={11,12,13};dirty.animation_scale=.5f;
        dirty.database.flags={8,8,8,0,1,0,1};
        Services services;services.observed=&dirty;services.ui.jump_hint_visible=services.ui.progress_visible=true;
        controller.ResetAndSync(false,services);
        Check(!dirty.enabled && dirty.reset_count==1 && dirty.actor==1 && dirty.database.actor==1);
        Check(dirty.input.result==0 && dirty.input.pending==std::array<std::uint32_t,4>{1,2,3,0});
        Check(dirty.input_vector==std::array<float,3>{} && dirty.transient==std::array<std::uint32_t,8>{});
        Check(!dirty.motion.active && !dirty.motion.pending && dirty.motion.reached && dirty.motion.mode==0);
        Check(dirty.motion.speed==123 && dirty.motion.target==std::array<float,3>{11,12,13} && dirty.animation_scale==1);
        Check(dirty.database.flags==std::array<std::uint32_t,7>{0,0,0,1,1,0,1});
        Check(services.order=="CHIM" && services.stopped && services.inputs==1 && !services.ui.progress_visible && !services.ui.jump_hint_visible);
        controller.ResetAndSync(true,services);Check(dirty.enabled && dirty.reset_count==2 && !services.stopped && services.enabled_at_cursor);
        services.ui.game_state=4;services.ui.ShowScene(true);Check(!services.ui.hud_visible && services.ui.scene_events==0);
        services.ui.game_state=1;services.ui.ShowScene(true);Check(services.ui.hud_visible && services.ui.scene_events==1 && services.ui.help_topic==1);
        services.ui.ShowScene(false);Check(!services.ui.hud_visible && services.ui.scene_events==2);
        failed=false;try{controller.SelectActor(4,1);}catch(const std::logic_error&){failed=true;}Check(failed && controller.State().actor==1);
        std::ostringstream report;WriteControllerReport(report,controller);Check(report.str().find("\"initialized\":true")!=std::string::npos);
        controller.Shutdown();controller.Shutdown();Check(!controller.Initialized() && !controller.FloorCallbacksRegistered());
        Check(!controller.State().actor && !controller.State().database.actor);
        std::cout<<"controller initialization checks passed\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
