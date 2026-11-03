#pragma once
#include "pal4/native_script_player_status.h"
#include "pal4/native_script_player_rest.h"
#include "pal4/native_player_manager_session.h"
#include "pal4/native_player_event_runtime.h"
#include "pal4/native_player_script_refresh_runtime.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <iostream>
#include <source_location>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativePlayerStatus(NativePlayerManagerSession& source,
    NativeInventorySession& inventory,const NativeGameDatabase& database) {
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) throw std::runtime_error("native player status probe line="+std::to_string(at.line()));
    };
    // Copy scalar state only. Owned scene pointers, equipment, skill arrays and
    // learned arrays remain empty; the probe cannot retire live player resources.
    PlayerManagerObjectState players;
    NativeBaseObjectMemory memory;
    NativeUpgradeRecordCatalog upgrades(database);
    NativePlayerScriptRefreshRuntime refresh(memory,inventory);
    NativePlayerEventRuntime events(upgrades,memory,refresh);
    NativeScriptPlayerStatus runtime(players,memory,database,events,refresh);
    const auto player=[&](unsigned i){return runtime.PlayerByIndex(runtime.PlayerManager(),static_cast<int>(i));};
    for(unsigned i=0;i<4u;++i) {
        std::memcpy(reinterpret_cast<void*>(player(i)+0x654u),
            reinterpret_cast<const void*>(source.Address()+i*PlayerManagerObject::kElementBytes+0x654u),0x2a4u);
        memory.WriteDword(player(i)+0xa48u,i);
    }
    const auto learned=[&](unsigned i) {
        std::vector<std::uint32_t> result;
        for(auto p=memory.ReadDword(player(i)+0x9dcu);p!=memory.ReadDword(player(i)+0x9e0u);p+=4u)
            result.push_back(memory.ReadDword(p));
        return result;
    };
    const auto& stunts=database.Tables().at("Stunt");
    constexpr std::array<std::string_view,4> level_fields{
        "ePT_TIANHE_LV_LMT","ePT_LINSHA_LV_LMT","ePT_MENGLI_LV_LMT","ePT_ZIYING_LV_LMT"};
    const auto column=[&](std::string_view name) {
        const auto it=std::find_if(stunts.fields.begin(),stunts.fields.end(),[&](const auto& f){return f.name==name;});
        check(it!=stunts.fields.end());return static_cast<std::size_t>(it-stunts.fields.begin());
    };
    const auto expected=[&](unsigned i,unsigned level) {
        std::vector<std::uint32_t> result;
        for(const auto& row:stunts.rows) {
            const auto need=std::bit_cast<std::int32_t>(std::get<std::uint32_t>(row[column(level_fields[i])]));
            if(need>0 && need<=static_cast<int>(level))result.push_back(std::get<std::uint32_t>(row[column("ID")]));
        }
        std::sort(result.begin(),result.end());result.erase(std::unique(result.begin(),result.end()),result.end());return result;
    };
    const auto verify_stats=[&](unsigned i,unsigned level) {
        const auto script=player(i)+0x654u;
        check(refresh.GetValue(script,4u)==level);
        const NativeUpgradeRecordCatalog::Words* row=nullptr;
        for(std::size_t n=0;n<upgrades.Count();++n)
            if(upgrades.Record(n)[37]==i+1001u && upgrades.Record(n)[38]==level){row=&upgrades.Record(n);break;}
        check(row!=nullptr);
        check(refresh.GetMaximum(script,0u)==(*row)[1] && refresh.GetMaximum(script,2u)==(*row)[3]);
        check(refresh.GetValue(script,7u)==refresh.GetMaximum(script,0u) && refresh.GetValue(script,9u)==refresh.GetMaximum(script,2u));
    };
    NativeAngelScriptEngine engine;NativeGameScriptServices services{};services.player_status=&runtime;services.party_recovery=&runtime;services.party_magic_query=&runtime;services.player_favor=&runtime;services.forbidden_skill=&runtime;
    unsigned rest_samples{};
    NativeScriptPlayerRest rest(runtime, [] { return 73u; },
        [&](std::uint32_t manager,std::string_view path,std::uint32_t mode) {
            check(manager==73u && path=="gamedata\\PALSound\\WJ063.wav" && mode==1u);
            ++rest_samples;return 91u;
        });
    services.player_rest=&rest;
    NativeGameScriptCallbacks callbacks(services);engine.SetCallbacks(&callbacks);
    constexpr char code[]=R"(
        void Ten() { giSetPlayerLevel(0,10); giSetPlayerLevel(1,10); giSetPlayerLevel(2,10); giSetPlayerLevel(3,10); }
        void High() { giSetPlayerLevel(0,99); giSetPlayerLevel(1,99); giSetPlayerLevel(2,99); giSetPlayerLevel(3,99); }
        void Low() { giSetPlayerLevel(0,1); giSetPlayerLevel(1,1); giSetPlayerLevel(2,1); giSetPlayerLevel(3,1); }
        void Invalid() { giSetPlayerLevel(-1,10); giSetPlayerLevel(4,10); giSetPlayerLevel(0,0); giSetPlayerLevel(1,100); }
        void Hp() { giSetFullHP(); }
        void Mp() { giSetFullMP(); }
        void Rest() { giPlayerTakeARest(); }
        bool HasMagic() { return giCheckMagicMastered(); }
        void FavorPair() { giAddPlayerFavor(0,1,20); giAddPlayerFavor(1,0,20); }
        void FavorClamp() { giAddPlayerFavor(0,1,-30); }
        void FavorOverflow() { giAddPlayerFavor(0,1,1); }
        void FavorMissing() { giAddPlayerFavor(4,1,20); giAddPlayerFavor(0,4,20); }
        void Forbid() { giPlayerForbidenSkill(0,5501,true); giPlayerForbidenSkill(0,5502,true); giPlayerForbidenSkill(0,5503,true); giPlayerForbidenSkill(0,5504,true); giPlayerForbidenSkill(0,5506,true); }
        void ForbidRepeat() { giPlayerForbidenSkill(0,5501,true); }
        void Allow() { giPlayerForbidenSkill(0,5502,false); giPlayerForbidenSkill(0,5506,false); }
        void MissingForbid() { giPlayerForbidenSkill(4,5501,true); giPlayerForbidenSkill(-1,5501,true); }
        void OtherForbid() { giPlayerForbidenSkill(1,5500,true); }
    )";
    check(engine.Raw()->AddScriptSection("level","level.cs",code,sizeof(code)-1)>=0 && engine.Raw()->Build("level")>=0);
    asIScriptContext* context{};check(engine.Raw()->CreateContext(&context)>=0 && context);
    const auto run=[&](const char* name) {
        check(context->Prepare(engine.FunctionId("level",name))>=0 && context->Execute()==asEXECUTION_FINISHED);
    };
    try {
        // Role ID, not the second player's slot index, selects the relation.
        memory.WriteDword(player(1)+0xa48u,3u);
        auto favor_expected=players;
        favor_expected.Word(0xa44u/4u)=20u;
        favor_expected.Word((PlayerManagerObject::kElementBytes+0xa38u)/4u)=20u;
        run("FavorPair");check(std::memcmp(&favor_expected,&players,sizeof(players))==0);
        run("FavorClamp");check(memory.ReadDword(player(0)+0xa44u)==0u);
        memory.WriteDword(player(0)+0xa44u,0x7fffffffu);
        run("FavorOverflow");check(memory.ReadDword(player(0)+0xa44u)==0u);
        const auto before_missing=players;run("FavorMissing");
        check(std::memcmp(&before_missing,&players,sizeof(players))==0);
        for(const auto address:{source.Address()+0xa38u,player(0)+0xa48u,player(0)+0xa39u}) {
            bool rejected=false;try{runtime.WriteWord(address,1u);}catch(const std::invalid_argument&){rejected=true;}
            check(rejected);
        }
        memory.WriteDword(player(1)+0xa48u,1u);
        std::cout<<"Player favor: compiled -219, reciprocal20, role-to-slot mapping, negative/overflow clamp, missing players and foreign/invalid write rejection pass on isolated native slots\n";
        run("HasMagic");check(context->GetReturnDWord()==0u);
        for(unsigned i=0;i<4u;++i){
            const auto collection=player(i)+0x938u,values=memory.Allocate(4u);memory.WriteDword(values,5001u);
            memory.WriteDword(collection+4u,values);memory.WriteDword(collection+8u,values+4u);memory.WriteDword(collection+12u,values+4u);
            run("HasMagic");check(context->GetReturnDWord()==0u);
            memory.WriteDword(player(i)+0xb08u,1u);run("HasMagic");check(context->GetReturnDWord()==1u);
            memory.WriteDword(player(i)+0xb08u,0u);
            for(unsigned offset:{4u,8u,12u})memory.WriteDword(collection+offset,0u);
            (void)memory.Free(values);
        }
        bool foreign=false;try{(void)runtime.HasAnyValues(player(0u)+0x939u);}catch(const std::invalid_argument&){foreign=true;}check(foreign);
        run("HasMagic");check(context->GetReturnDWord()==0u && !memory.OutstandingAllocations());
        std::cout<<"Party Magic query: compiled giCheckMagicMastered false/true, all4 membership gates, actual collection owner and zero allocations pass\n";
        const auto before_forbidden = players;
        const auto forbidden = [&](unsigned i) {
            std::vector<std::uint32_t> values;
            for (auto p=memory.ReadDword(player(i)+0x9ecu);
                 p!=memory.ReadDword(player(i)+0x9f0u);p+=4u)
                values.push_back(memory.ReadDword(p));
            return values;
        };
        run("Forbid");run("ForbidRepeat");
        check(forbidden(0)==std::vector<std::uint32_t>({5501,5502,5503,5504,5506}));
        auto only_forbidden = before_forbidden;
        for (unsigned offset : {0x9ecu,0x9f0u,0x9f4u})
            only_forbidden.Word(offset/4u)=players.Word(offset/4u);
        check(std::memcmp(&only_forbidden,&players,sizeof(players))==0);
        run("Allow");check(forbidden(0)==std::vector<std::uint32_t>({5501,5503,5504}));
        const auto before_invalid_forbidden=players;run("MissingForbid");
        check(std::memcmp(&before_invalid_forbidden,&players,sizeof(players))==0);
        run("OtherForbid");check(forbidden(1)==std::vector<std::uint32_t>{5500});
        bool foreign_forbidden=false;
        try{(void)runtime.SetPresent(source.Address()+0x938u,5501,1u);}
        catch(const std::invalid_argument&){foreign_forbidden=true;}
        check(foreign_forbidden);
        for(unsigned i:{0u,1u}) {
            (void)memory.Free(memory.ReadDword(player(i)+0x9ecu));
            for(unsigned offset:{0x9ecu,0x9f0u,0x9f4u})memory.WriteDword(player(i)+offset,0u);
        }
        check(!memory.OutstandingAllocations() &&
              std::memcmp(&before_forbidden,&players,sizeof(players))==0);
        std::cout<<"Forbidden skills: compiled -204, five original IDs, duplicate/remove order, inactive role, missing/foreign owners, unchanged learned/scalar state and allocation cleanup pass\n";
        run("Ten");
        for(unsigned i=0;i<4u;++i){verify_stats(i,10u);check(learned(i)==expected(i,10u));}
        run("High");
        for(unsigned i=0;i<4u;++i){verify_stats(i,99u);check(learned(i)==expected(i,99u));}
        check(memory.OutstandingAllocations()==4u); // Four learned arrays, no temporary result leaks.
        const auto old=players;run("High");run("Invalid");
        check(std::memcmp(&old,&players,sizeof(players))==0 && memory.OutstandingAllocations()==4u);
        run("Low");
        for(unsigned i=0;i<4u;++i){verify_stats(i,1u);check(learned(i)==expected(i,99u));}
        for(unsigned i=0;i<4u;++i) {
            memory.WriteDword(player(i)+0xb08u,i%2u);
            (void)runtime.WriteCurrentStat(player(i)+0x654u,7u,1u);
            (void)runtime.WriteCurrentStat(player(i)+0x654u,9u,1u);
        }
        const auto before_rest=players;
        run("Hp");
        for(unsigned i=0;i<4u;++i) {
            check(refresh.GetValue(player(i)+0x654u,7u)==(i%2u ? refresh.GetMaximum(player(i)+0x654u,0u):1u));
            check(refresh.GetValue(player(i)+0x654u,9u)==1u);
        }
        run("Mp");run("Hp");run("Mp");
        for(unsigned i=0;i<4u;++i)
            check(refresh.GetValue(player(i)+0x654u,9u)==(i%2u ? refresh.GetMaximum(player(i)+0x654u,2u):1u));
        const auto expected_rest=players;
        players=before_rest;
        run("Rest");
        check(rest_samples==1u && std::memcmp(&expected_rest,&players,sizeof(players))==0);
        run("Rest");
        check(rest_samples==2u && std::memcmp(&expected_rest,&players,sizeof(players))==0);
        for(unsigned i=0;i<4u;++i)memory.WriteDword(player(i)+0xb08u,0u);
        const auto empty_party=players;
        run("Rest");
        check(rest_samples==3u && std::memcmp(&empty_party,&players,sizeof(players))==0);
        std::cout<<"Player rest: compiled -199, active-only HP/MP via native setters, whole player images (including nonparty/SP/status) retained, repeat and empty-party WJ063 mode1 samples pass\n";
        bool rejected=false;try{runtime.WriteLevel(source.Address()+0x654u,4u,10);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
        context->Release();
    } catch(...) {context->Release();throw;}
    std::cout<<"Player levels/recovery: compiled callbacks, all four original growth/HP/MP and Stunt requirements,1/10/99, repeat/invalid calls, learned skills retained on lowering, active-only HP/MP recovery, no temporary allocations and foreign-owner rejection verified\n";
}
}
