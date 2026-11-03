#pragma once
#include "pal4/native_combat_world_input.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_combat_roster_targets.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_world_resources.h"
#include "pal4/native_world_asset.h"
#include "pal4/native_script_event_router.h"
#include "pal4/input_manager.h"
#include "pal4/window_message_processor.h"
#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <rwcore.h>
namespace pal4::test {
inline void VerifyNativeCombatWorldInput(NativeCombatWorldInput& frame,NativeCombatSystemImage& image,
    NativeCombatRosterTargets& targets,NativeCombatMonsterManager& monsters,NativeScriptEventRouter& router,InputManager& input,WindowMessageConfig& config,
    NativeCombatWorldResources& world,const std::filesystem::path& root,std::uint32_t player,std::uint32_t monster,
    const std::array<float,2>& monster_pixel,const std::array<float,2>& player_pixel){
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto words=image.target_selection.words;const auto state=image.State();const auto mode=image.Mode();const auto saved_input=input;
    const auto x=static_cast<std::uint32_t>(config[12u]),y=static_cast<std::uint32_t>(config[13u]);
    std::vector<std::uint32_t> observers;bool started{};
    struct Restore{NativeCombatSystemImage& image;NativeScriptEventRouter& router;InputManager& input;WindowMessageConfig& config;
        std::array<std::uint32_t,21> words;std::uint32_t state;std::uint8_t mode;InputManager record;std::uint32_t x,y;
        bool& started;std::vector<std::uint32_t>& observers;
        ~Restore(){for(auto id:observers)(void)router.Unsubscribe(id);if(started){image.State()=3u;try{(void)router.Dispatch("stop_select_target",0u);}catch(...){}}
            image.target_selection.words=words;image.State()=state;image.Mode()=mode;input=record;config[12u]=x;config[13u]=y;}
    } restore{image,router,input,config,words,state,mode,saved_input,x,y,started,observers};
    const auto position=[&](const std::array<float,2>& pixel){config[12u]=std::bit_cast<std::uint32_t>(pixel[0]);config[13u]=std::bit_cast<std::uint32_t>(pixel[1]);};
    const auto neutral=[&]{for(auto o:{0x21e4u,0x21d8u,0x21d4u,0x21c8u})input.WriteDword(o,0u);};
    const auto start=[&]{image.State()=3u;image.Mode()=0u;const std::array<std::uint32_t,4> p{player+716u,0u,0u,4u};started=true;(void)router.Dispatch("start_select_target",reinterpret_cast<std::uint32_t>(p.data()));neutral();reinterpret_cast<std::uint8_t*>(image.target_selection.words.data())[80u]=0u;};
    // Establish selection before observing periodic hover: start_select_target
    // may synchronously publish the initial target through the visible Main UI.
    start();position(monster_pixel);image.target_selection.words[19]=0u;
    unsigned tips{},reentry{},restarts{};std::uint32_t last{};bool fail{};
    observers.push_back(router.Subscribe(router.Key("show_monster_tips"),1u,[&](std::uint32_t payload){
        const auto facet=*reinterpret_cast<const std::uint32_t*>(payload);last=facet?facet-716u:0u;
        if(last!=monster && last!=player)
            std::cout<<"Native hover mismatch: actor="<<last<<" expected_monster="<<monster<<" expected_player="<<player
                <<" tips="<<tips<<" pixel="<<std::bit_cast<float>(static_cast<std::uint32_t>(config[12u]))<<","<<std::bit_cast<float>(static_cast<std::uint32_t>(config[13u]))<<std::endl;
        check(last==monster || last==player,"periodic hover emits actual actor716 facet payload");++tips;
        try{(void)frame.Update(.01f);}catch(const std::logic_error&){++reentry;}
        for(unsigned which=0;which<3u;++which){bool rejected{};try{
            if(which==0u)world.Reset();
            if(which==1u)world.ClearGeometryQueues();
            if(which==2u)(void)world.UpdateRenderParameters(.01f);
        }catch(const std::logic_error&){rejected=true;}check(rejected,"world update pins creating resources through actual input callbacks");}
        if(fail)throw std::runtime_error("intentional periodic hover observer failure");
    }));
    observers.push_back(router.Subscribe(router.Key("re_start_ui_control"),1u,[&](std::uint32_t){++restarts;}));
    const auto before=world.Asset().FirstAnimationTime();(void)frame.Update(.1f);const auto after=world.Asset().FirstAnimationTime();
    check(frame.WorldUpdated() && !tips && std::bit_cast<float>(image.target_selection.words[19])==.1f,"world UV phase precedes original sub-threshold input timer");
    if(before && after)check(*before!=*after,"actual Combat world UV interpolator advances");
    (void)frame.Update(.25f);check(tips==1u && last==monster && image.target_selection.words[17]==monster && !image.target_selection.words[19],"periodic actual preferred Monster scan runs past0.3 and resets timer");
    {
        // Original53C500 prefers any eligible Monster, even behind/over a
        // Player sample. Make the fallback precondition explicit through the
        // real CandidateAllowed health3 gate; restore every actor before retry.
        struct IneligibleMonsters{
            std::vector<std::pair<std::uint32_t,std::uint32_t>> health;
            std::uint8_t& gate;std::uint8_t saved;
            explicit IneligibleMonsters(std::uint8_t& g):gate(g),saved(g){}
            ~IneligibleMonsters(){for(auto [actor,value]:health)*reinterpret_cast<std::uint32_t*>(actor+648u)=value;gate=saved;}
        } excluded(reinterpret_cast<std::uint8_t*>(image.target_selection.words.data())[80u]);
        for(std::size_t i=0;i<monsters.StageCount();++i){
            const auto actor=monsters.StageActorAt(i);auto& health=*reinterpret_cast<std::uint32_t*>(actor+648u);
            excluded.health.emplace_back(actor,health);health=3u;
        }
        check(!excluded.health.empty(),"fallback fixture excludes actual preferred stage actors");excluded.gate=1u;
        position(player_pixel);(void)frame.Update(.35f);
        check(tips==2u && last==player && image.target_selection.words[17]==player,"periodic actual Player fallback follows ineligible Monster stage scan");
    }
    fail=true;position(monster_pixel);bool failed{};try{(void)frame.Update(.35f);}catch(const std::runtime_error& e){failed=std::string_view(e.what())=="intentional periodic hover observer failure";}
    fail=false;check(failed && !targets.ActiveIterators(),"periodic observer failure drains actual roster cursors");
    (void)frame.Update(.35f);check(last==monster && reentry==tips && !targets.ActiveIterators(),"periodic frame releases guards and retries after observer exception");
    const auto timer=image.target_selection.words[19];const auto count=tips;image.Mode()=1u;
    check(frame.Update(.5f)==1u && frame.WorldUpdated() && image.target_selection.words[19]==timer && tips==count,"world update remains first while inactive input gate preserves timer");image.Mode()=0u;
    image.State()=1u;check(!frame.Update(.5f) && image.target_selection.words[19]==timer && tips==count,"non-active state1 input gate preserves timer");image.State()=3u;
    check(image.target_selection.words[16]!=0u,"actual start/select command retains a real selected target");
    input.WriteDword(0x21e4u,1u);input.WriteDword(0x21d8u,1u);const auto restarted=restarts;
    (void)frame.Update(.01f);
    check(restarts==restarted+1u && !image.target_selection.words[16] && !image.target_selection.words[11] && !image.target_selection.words[17] && !reinterpret_cast<std::uint8_t*>(image.target_selection.words.data())[82u],"actual frame restart key dispatches once then runs original stop-selection cleanup");neutral();
    // No-target key gates are bounded: no command is fabricated or queued.
    image.target_selection.words[16]=0u;input.WriteDword(0x21e4u,1u);input.WriteDword(0x21d8u,1u);
    check(!frame.Update(.01f) && restarts==restarted+1u,"start key without target returns before timer");neutral();
    const auto nan_bits=0x7fc01234u;image.target_selection.words[19]=nan_bits;
    check(frame.Update(.01f)==0x4100u && tips==count,"original unordered timer branch does not scan targets");image.target_selection.words[19]=0u;
    auto& ready=*reinterpret_cast<std::uint32_t*>(world.Address()+48u);const auto saved_ready=ready;
    {struct Ready{std::uint32_t& value;std::uint32_t saved;~Ready(){value=saved;}} restore_ready{ready,saved_ready};ready=0u;(void)frame.Update(.01f);check(!frame.WorldUpdated(),"not-ready UV gate preserves subsequent input phase");}
    {
        // M01 has no animated UV clock. Temporarily borrow actual original ZJM
        // materials in a single schema slot; restore before their owner retires.
        NativeWorldAsset uv;if(!uv.LoadUi(root,"ZJM"))throw std::runtime_error("original ZJM UV fixture load: "+uv.LastError());
        check(uv.FirstAnimationTime().has_value(),"original ZJM provides actual SDK UV animation clock");
        unsigned source_slot=6u;for(unsigned i=0;i<6u;++i){const auto list=uv.AnimationSchema(i).list;
            if(list && _rwSListGetBegin(reinterpret_cast<RwSList*>(list))!=_rwSListGetEnd(reinterpret_cast<RwSList*>(list))){source_slot=i;break;}}
        check(source_slot<6u,"original ZJM provides a populated UV schema");
        auto& schema=const_cast<RwDictSchema&>(world.Asset().AnimationSchema(0u));const auto saved_schema=schema;
        struct Schema{RwDictSchema& schema;RwDictSchema saved;~Schema(){schema=saved;}} restore_schema{schema,saved_schema};
        schema=uv.AnimationSchema(source_slot);schema.time_scale=1.f;
        const auto clock=uv.FirstAnimationTime();(void)frame.Update(.125f);const auto advanced=uv.FirstAnimationTime();
        check(frame.WorldUpdated() && clock && advanced && *clock!=*advanced,"world/input path advances actual borrowed original SDK UV material clock");
        {struct Ready{std::uint32_t& value;std::uint32_t saved;~Ready(){value=saved;}} restore_ready{ready,saved_ready};ready=0u;(void)frame.Update(.125f);check(!frame.WorldUpdated() && uv.FirstAnimationTime()==advanced,"not-ready world skips actual SDK UV advance");}
    }
    failed=false;try{(void)frame.Update(-1.f);}catch(const std::invalid_argument&){failed=true;}check(failed,"negative frame time rejects before changing world/input");
    std::cout<<"Native combat world/input: actual six-schema world update UV_clock="<<(before.has_value()?1u:0u)<<" original_ZJM_UV_fixture=1 preferred Monster/player fallback tips="<<tips<<" real input state gates/timer/NaN, actor716 payload, cursor/reentry/observer recovery and world ready gate pass\n";
}
}
