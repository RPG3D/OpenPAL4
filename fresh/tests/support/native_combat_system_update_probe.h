#pragma once
#include "pal4/native_combat_system_update.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_combat_system_manager.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_player_model_cache.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_actor_model_owner.h"
#include "pal4/native_actor_instance_owner.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_script_event_router.h"
#include "pal4/input_manager.h"
#include "pal4/window_message_processor.h"
#include <bit>
#include <cmath>
#include <limits>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <rphanim.h>
#include <rtanim.h>
namespace pal4::test {
inline void VerifyNativeCombatSystemUpdate(NativeCombatSystemUpdate& frame,NativeCombatSystemImage& image,
    NativeCombatSystemManager& systems,NativeCombatPlayerManager& players,NativeCombatMonsterManager& monsters,
    NativeCombatPlayerModelCache& model,NativeScriptEventRouter& router,InputManager& input,WindowMessageConfig& config,
    const std::function<std::array<float,2>()>& current_target_pixel){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    auto& active=*reinterpret_cast<std::uint8_t*>(systems.Address()+124u);
    const auto saved_state=image.State(),elapsed=image.session_storage[29];const auto mode=image.Mode(),saved_active=active;
    const auto words=image.target_selection.words;const auto saved_input=input;
    const auto x=config[12u],y=config[13u];const auto slot=image.SystemManagerSlot();
    std::uint32_t route{};
    struct Restore{NativeCombatSystemImage& i;NativeScriptEventRouter& r;InputManager& input;WindowMessageConfig& c;
        std::uint8_t& active;std::uint32_t state,elapsed,slot;std::uint8_t mode,saved_active;
        std::array<std::uint32_t,21> words;InputManager saved;std::uint32_t x,y;std::uint32_t& route;
        ~Restore(){if(route)(void)r.Unsubscribe(route);i.State()=state;i.Mode()=mode;i.session_storage[29]=elapsed;
            i.SystemManagerSlot()=slot;i.target_selection.words=words;input=saved;c[12u]=x;c[13u]=y;active=saved_active;}
    } restore{image,router,input,config,active,saved_state,elapsed,slot,mode,saved_active,words,saved_input,x,y,route};
    const auto pc=players.Count(),mc=monsters.Count();check(pc && mc,"full update needs actual combat actors");
    auto* ph=model.Current().Asset()->Hierarchy();
    auto* mh=monsters.ModelOwner(monsters.StageActorAt(0u)).Model()->Asset()->Hierarchy();
    check(ph && mh && ph->currentAnim && mh->currentAnim,"full update requires actual SDK clocks");
    for(auto o:{0x21e4u,0x21d8u,0x21d4u,0x21c8u})input.WriteDword(o,0u);
    reinterpret_cast<std::uint8_t*>(image.target_selection.words.data())[80u]=0u;
    const auto position=[&](std::string_view phase){const auto pixel=current_target_pixel();
        config[12u]=std::bit_cast<std::uint32_t>(pixel[0]);config[13u]=std::bit_cast<std::uint32_t>(pixel[1]);
        std::cout<<"Full frame current hit: phase="<<phase<<" pixel="<<pixel[0]<<","<<pixel[1]<<std::endl;};
    position("initial");
    //Controlled original IsActive gate prevents initiating an unrelated AI turn
    //while checking all575F40 states. The real SystemManager still receives each
    //states1..5 call; its original active-byte gate decides whether to advance.
    active=1u;
    for(std::uint32_t s=1u;s<=6u;++s)for(auto inactive:{0u,1u}){
        image.State()=s;image.Mode()=static_cast<std::uint8_t>(inactive);image.session_storage[29]=0u;
        image.target_selection.words[19]=0u;const auto p=ph->currentAnim->currentTime,m=mh->currentAnim->currentTime;
        frame.Advance(.001f,0u);
        check(image.State()==s && image.session_storage[29]==(s>=2u && s<=5u?std::bit_cast<std::uint32_t>(.001f):0u),
            "full native update preserves original active elapsed gate");
        check(ph->currentAnim->currentTime!=p && mh->currentAnim->currentTime!=m,
            "states1..6 including inactive advance both actual actor SDK clocks");
        check(image.target_selection.words[19]==(!inactive && s>=2u && s<=5u?std::bit_cast<std::uint32_t>(.001f):0u),
            "full native state/mode gates retain actual target input timer behavior");
    }
    image.State()=2u;image.Mode()=0u;image.session_storage[29]=0u;
    for(auto dt:{-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
        bool rejected{};try{frame.Advance(dt,0u);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected && !image.session_storage[29],"invalid full frame delta fails before elapsed mutation");
    }
    image.SystemManagerSlot()=0u;bool foreign{};try{frame.Advance(.01f,0u);}catch(const std::logic_error&){foreign=true;}
    image.SystemManagerSlot()=slot;check(foreign && !image.session_storage[29],"full update validates actual published manager before mutation");
    // The twelve real actor-frame updates above invalidate the old projected
    // triangle sample. Require a current actual hit before testing observers.
    position("observer");
    unsigned seen{},reentry{};bool fail=true;
    route=router.Subscribe(router.Key("show_monster_tips"),1u,[&](std::uint32_t){
        ++seen;try{frame.Advance(.001f,0u);}catch(const std::logic_error&){++reentry;}
        if(fail)throw std::runtime_error("intentional full combat frame observer failure");
    });
    image.target_selection.words[19]=std::bit_cast<std::uint32_t>(.3f);bool failed{};
    std::string observed_error;
    try{frame.Advance(.001f,0u);}catch(const std::runtime_error& e){observed_error=e.what();failed=observed_error=="intentional full combat frame observer failure";}
    std::cout<<"Full frame observer boundary: expected_error="<<failed<<" seen="<<seen<<" reentry="<<reentry
        <<" player_cursors="<<players.ActiveIterators()<<" monster_cursors="<<monsters.ActiveIterators()
        <<" state="<<image.State()<<" mode="<<unsigned(image.Mode())<<" active="<<unsigned(active)
        <<" hover="<<image.target_selection.words[17]<<" timer="<<std::bit_cast<float>(image.target_selection.words[19])
        <<" pixel="<<std::bit_cast<float>(static_cast<std::uint32_t>(config[12u]))<<","<<std::bit_cast<float>(static_cast<std::uint32_t>(config[13u]))
        <<" error="<<observed_error<<std::endl;
    check(failed && seen==1u && reentry==1u && !players.ActiveIterators() && !monsters.ActiveIterators(),
        "full frame observer exception drains real roster cursors and rejects reentry");
    //53C500 resets its timer before the synchronous tips event. Retry with a
    //new elapsed threshold; a failed observer does not roll that field back.
    fail=false;position("retry");image.target_selection.words[19]=std::bit_cast<std::uint32_t>(.3f);frame.Advance(.001f,0u);
    check(seen==2u && reentry==2u && pc==players.Count() && mc==monsters.Count(),"full native frame retries after actual observer failure");
    std::cout<<"Native575F40 update: states1..6 active/inactive12 actual Player/Monster SDK frames, original SystemManager active gate, world/input timer, invalid delta/owner/reentry and observer retry pass; direct quick_end and direct state7 are separate paths\n";
}
}
