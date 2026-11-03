#pragma once
#include "pal4/native_role_state_window.h"
#include "pal4/native_player_hud_stats_data.h"
#include "pal4/native_player_script_property_runtime.h"
#include "pal4/native_inventory_session.h"
#include "pal4/input_manager.h"
#include "pal4/role_state_window_members.h"
#include "pal4/role_state_window_level_text.h"
#include "../../native/ui/cegui_exports_native.h"
#include <cmath>
#include <array>
#include <string>
#include <stdexcept>
#include <iostream>
namespace pal4::test {
inline void VerifyNativeRoleSpecialInput(NativeRoleStateWindow& role,PlayerManagerObjectState& players,
    InputManager& input,NativeInventorySession& inventory){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    constexpr std::array<std::uint32_t,4> offsets{0x1ee4u,0x1ed8u,0x1ef4u,0x1ee8u};
    NativePlayerHudStatsData stats(players);
    struct Restore {
        InputManager& input;NativeInventorySession& inventory;NativeRoleStateWindow& role;
        std::array<std::uint32_t,4> keys{},scripts{},rage{};std::int32_t money;
        ~Restore(){constexpr std::array<std::uint32_t,4> saved_offsets{0x1ee4u,0x1ed8u,0x1ef4u,0x1ee8u};
            for(unsigned i=0;i<4;++i){input.WriteDword(saved_offsets[i],keys[i]);
            if(scripts[i])(void)NativePlayerScriptPropertyRuntime::Set(scripts[i],68u,8u,rage[i]);}
            (void)inventory.UpdateCounter(-static_cast<std::int32_t>(inventory.ReadDword(inventory.Address()+308u)));
            (void)inventory.UpdateCounter(money);role.SetVisible(true);}
    } restore{input,inventory,role,{}, {},{},static_cast<std::int32_t>(inventory.ReadDword(inventory.Address()+308u))};
    const auto manager=reinterpret_cast<std::uint32_t>(&players);
    for(unsigned i=0;i<4;++i){restore.keys[i]=input.ReadDword(offsets[i]);
        const auto player=PlayerManagerObject::GetPlayerByIndex(manager,static_cast<std::int32_t>(i));
        if(*reinterpret_cast<const std::uint32_t*>(player+0xb08u)){
            restore.scripts[i]=player+0x654u;restore.rage[i]=stats.PlayerStat(restore.scripts[i],8u);}}
    std::array<std::uint32_t,8> event{};
    const auto dispatch=[&](std::uint32_t code){event[4]=code;return role.DispatchActivePredicate(role.InputAddress(),4u,
        reinterpret_cast<std::uint32_t>(event.data()));};
    check(dispatch(6u)==0u,"Role special input ignores unrelated code");
    for(unsigned failed=0;failed<4;++failed){for(unsigned i=0;i<4;++i)input.WriteDword(offsets[i],i==failed?0u:1u);
        check(dispatch(5u)==1u && inventory.ReadDword(inventory.Address()+308u)==static_cast<std::uint32_t>(restore.money),
            "each original input guard prevents money mutation");}
    for(const auto offset:offsets)input.WriteDword(offset,1u);
    check(dispatch(5u)==1u && inventory.ReadDword(inventory.Address()+308u)==
        static_cast<std::uint32_t>(restore.money>9999499?9999999:restore.money+500),
        "Role original code5 reaches actual inventory clamp and GameInfo event");
    for(auto script:restore.scripts)if(script)(void)NativePlayerScriptPropertyRuntime::Set(script,68u,8u,17u);
    check(dispatch(10u)==1u,"Role original code10 is handled by native backend");
    const auto module=GetModuleHandleA("CEGUIBase.dll");
    const auto text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto utf8=detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ");
    for(unsigned i=0;i<4;++i){if(restore.scripts[i])check(stats.PlayerStat(restore.scripts[i],8u)==100,"Role rage uses original scalar setter68");
        const auto member=role.FrameAddress()+RoleStateWindowMembers::Offset(i);
        const auto panel=*reinterpret_cast<const std::uint32_t*>(member+12u);
        if(detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z")(reinterpret_cast<const void*>(panel),false))
            check(std::string(utf8(text(reinterpret_cast<const void*>(*reinterpret_cast<const std::uint32_t*>(member+56u)))))=="100/100",
                "Role special input redraws actual rage DLL text");}
    std::cout<<"Native Role special input: four original guards, real money/GameInfo and rage setter68/DLL text pass\n";
}
inline void VerifyNativeRoleStateText(NativeRoleStateWindow& role,PlayerManagerObjectState& players){
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    const auto manager=reinterpret_cast<std::uint32_t>(&players);auto module=GetModuleHandleA("CEGUIBase.dll");
    const auto get=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto utf8=detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ");
    const auto visible=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z");
    const auto text=[&](std::uint32_t a){return std::string(utf8(get(reinterpret_cast<const void*>(a))));};
    NativePlayerHudStatsData stats(players);unsigned count{};std::uint32_t first_member{},first_script{};
    check(word(role.FrameAddress()+596u)==PlayerManagerObject::GetActivePlayerCount(players),"Role page uses actual active player count");
    for(unsigned i=0;i<4u;++i){const auto member=role.FrameAddress()+RoleStateWindowMembers::Offset(i);
        if(!visible(reinterpret_cast<const void*>(word(member+12u)),false))continue;
        const auto index=word(member);check(index<4u,"Role card identifies an actual player");
        const auto player=PlayerManagerObject::GetPlayerByIndex(manager,static_cast<std::int32_t>(index)),script=player+0x654u;
        check(word(player+0xb08u)!=0u && word(script+167u*4u)==i && word(script+83u*4u)==i,
            "Role display order writes both actual original script fields");
        check(text(word(member+20u))==RoleStateWindowLevelText::FormatText(stats.PlayerStat(script,4u)) &&
            text(word(member+48u))==std::to_string(stats.PlayerStat(script,7u))+"/"+std::to_string(stats.PlayerMaxStat(script,0u)) &&
            text(word(member+52u))==std::to_string(stats.PlayerStat(script,9u))+"/"+std::to_string(stats.PlayerMaxStat(script,2u)) &&
            text(word(member+56u))==std::to_string(stats.PlayerStat(script,8u))+"/100",
            "original DLL Role controls display actual level, HP, MP and rage");
        if(!count){first_member=member;first_script=script;}++count;
    }
    check(count==PlayerManagerObject::GetActivePlayerCount(players) && count>0u,"Role cards exactly cover the live party");
    const auto hp=stats.PlayerStat(first_script,7u);
    if(hp>1){struct Restore{std::uint32_t script;std::int32_t hp;~Restore(){(void)NativePlayerScriptPropertyRuntime::Set(script,68u,7u,static_cast<std::uint32_t>(hp));}} restore{first_script,hp};
        (void)NativePlayerScriptPropertyRuntime::Set(first_script,68u,7u,static_cast<std::uint32_t>(hp/2));role.SetVisible(true);
        check(text(word(first_member+48u))==std::to_string(hp/2)+"/"+std::to_string(stats.PlayerMaxStat(first_script,0u)),
            "repeat Role show reads actual changed HP rather than cached UI values");
        const auto width=detail::Export<float (__thiscall*)(const void*)>(module,"?getWidth@Window@CEGUI@@QBEMXZ")(reinterpret_cast<const void*>(word(first_member+60u)));
        const auto baseline=*reinterpret_cast<const float*>(first_member+104u);
        const auto expected=static_cast<float>(static_cast<double>(static_cast<float>(hp/2))/static_cast<float>(stats.PlayerMaxStat(first_script,0u))*baseline);
        check(std::fabs(width-expected)<0.0001f,"Role HP mask uses original stored width and actual HP fraction");
    }
    role.SetVisible(true);
    std::cout<<"Native Role page: actual original layout/cards="<<count<<", level/HP/MP/rage DLL text, live HP mask, order mirror and repeat show pass\n";
}
}
