#pragma once
#include "pal4/native_combat_end_sequence.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_combat_main_session.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_monster_manager.h"
#include "pal4/native_combat_hints.h"
#include "pal4/native_script_event_router.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatCloseUi(NativeCombatEndSequence& lifecycle,
    NativeCombatSystemImage& image,NativeCombatMainSession& main,
    NativeCombatPlayerManager& players,NativeCombatMonsterManager& monsters,
    NativeCombatHints& hints,NativeScriptEventRouter& router){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto state=image.State();const auto mode=image.Mode();
    auto& result=image.session_storage[27];const auto saved_result=result;
    unsigned closed{};
    const auto token=router.Subscribe(router.Key("close_system"),3u,[&](std::uint32_t){++closed;});
    struct Restore{
        NativeCombatSystemImage& image;NativeCombatMainSession& main;NativeScriptEventRouter& router;
        std::uint32_t token,state,result;std::uint8_t mode;
        ~Restore(){try{main.SetVisible(false);}catch(...){}(void)router.Unsubscribe(token);
            image.State()=state;image.Mode()=mode;image.session_storage[27]=result;}
    } restore{image,main,router,token,state,saved_result,mode};
    const auto pc=players.Count(),mc=monsters.Count(),ps=players.StageCount(),ms=monsters.StageCount();
    check(pc && mc && ps && ms,"CloseUi probe borrows actual prepared combat rosters");
    image.State()=3u;result=0x1357u;
    for(const auto inactive:{0u,1u}){
        image.Mode()=static_cast<std::uint8_t>(inactive);main.SetVisible(true);
        check(main.SubscriptionCount()==19u,"CloseUi begins with actual visible Main frame");
        check(lifecycle.CloseUi()==1u,"576610 returns original literal one");
        check(main.SubscriptionCount()==(inactive?19u:0u),"active close hides Main; inactive close only targets Quick frames");
        for(unsigned v:{1u,2u}){
            const auto root=hints.Root(v);std::uint32_t parent{};
            check(root!=0u,"CloseUi ensures both real original hint roots");
            std::memcpy(&parent,reinterpret_cast<const void*>(root+68u),sizeof(parent));
            check(parent==0u,"CloseUi cleans and detaches actual DLL hints");
        }
        check(image.State()==3u && result==0x1357u && image.Mode()==inactive && !closed &&
            pc==players.Count() && mc==monsters.Count() && ps==players.StageCount() && ms==monsters.StageCount(),
            "direct CloseUi preserves scheduler/result/mode/rosters and does not emit close_system");
        check(lifecycle.CloseUi()==1u,"direct CloseUi can repeat without ending battle");
        main.SetVisible(false);
    }
    std::cout<<"Native Combat CloseUi: actual576610 active/Quick branches,19 Main subscriptions, both DLL hints, repeated close, preserved state/result/rosters and no close_system pass; full575F40 is verified separately\n";
}
}
