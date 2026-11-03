#pragma once
#include "pal4/native_script_party_membership.h"
#include "pal4/native_script_inventory_change.h"
#include "pal4/native_player_manager_session.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include <cstring>
#include <iostream>
#include <source_location>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativePartyMembership(NativePlayerManagerSession& source,
    NativeInventorySession& inventory, NativeScriptInventoryChange& hints) {
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) throw std::runtime_error("native party probe line="+std::to_string(at.line()));
    };
    // Independent slots exercise actual inventory/DB/UI/audio without changing
    // the opening's persistent players or introducing a scene transition.
    PlayerManagerObjectState players;
    NativeBaseObjectMemory memory;
    NativeScriptPartyMembership runtime(players,memory,inventory,hints);
    const auto player=[&](unsigned i){return runtime.PlayerByIndex(runtime.PlayerManager(),i);};
    const auto read=[&](unsigned i,unsigned offset){return memory.ReadDword(player(i)+offset);};
    const auto write=[&](unsigned i,unsigned offset,unsigned value){memory.WriteDword(player(i)+offset,value);};
    for(unsigned i=0;i<4u;++i) {
        const auto original=source.Address()+i*PlayerManagerObject::kElementBytes;
        write(i,0x9fcu,memory.ReadDword(original+0x9fcu));
    }
    NativeAngelScriptEngine engine;
    NativeGameScriptServices services{};services.party_membership=&runtime;
    NativeGameScriptCallbacks callbacks(services);engine.SetCallbacks(&callbacks);
    constexpr char code[]=R"(
        void Add() { giPlayerInTeam(1,false); }
        void Remove() { giPlayerOutTeam(1,false); }
        void HintAdd() { giPlayerInTeam(1,true); }
        void HintRemove() { giPlayerOutTeam(1,true); }
        void Invalid() { giPlayerInTeam(-1,true); giPlayerOutTeam(4,true); }
    )";
    check(engine.Raw()->AddScriptSection("party","party.cs",code,sizeof(code)-1)>=0 && engine.Raw()->Build("party")>=0);
    asIScriptContext* context{};check(engine.Raw()->CreateContext(&context)>=0 && context);
    const auto run=[&](const char* function) {
        check(context->Prepare(engine.FunctionId("party",function))>=0);
        check(context->Execute()==asEXECUTION_FINISHED);
    };
    try {
        write(0,0xb08u,1u);runtime.SetPartyOrder(player(0)+0x654u,8u);
        write(2,0xb08u,1u);runtime.SetPartyOrder(player(2)+0x654u,0xffffffffu);
        write(1,0x920u,0xfffffffeu); // Missing sequence must be cleared on activation.
        const auto gear=memory.ReadDword(source.Address()+0x920u);
        check(gear!=0u); // Original NewGame equips the bow.
        write(0,0x920u,gear);write(1,0x924u,gear); // Active-player collision.
        run("Add");
        check(read(1,0xb08u)==1u && read(1,0x920u)==0u && read(1,0x924u)==0u);
        check(read(2,0x8f0u)==0u && read(0,0x8f0u)==1u && read(1,0x8f0u)==2u);
        for(unsigned i=0;i<3u;++i) check(read(i,0x7a0u)==read(i,0x8f0u));
        const auto before=players;run("Add");run("Invalid");
        check(std::memcmp(&before,&players,sizeof(players))==0);
        run("Remove");check(read(1,0xb08u)==0u && read(2,0x8f0u)==0u && read(0,0x8f0u)==1u);
        // No active player owns gear now: existing sequence must survive validation.
        write(0,0x920u,0u);write(1,0x920u,gear);
        run("HintAdd");check(read(1,0xb08u)==1u && read(1,0x920u)==gear && runtime.HintText().empty());
        run("HintRemove");check(read(1,0xb08u)==0u && runtime.HintText().empty());
        const auto removed=players;run("Remove");check(std::memcmp(&removed,&players,sizeof(players))==0);
        runtime.InitializeHintText(0u);runtime.InitializeHintText(1u);
        runtime.FormatHintText("%% %s", "name");runtime.AppendHintText();check(runtime.HintText()=="% name");
        bool rejected=false;try{runtime.FormatHintText("%d","name");}catch(const std::runtime_error&){rejected=true;}check(rejected);
        rejected=false;try{runtime.SetPartyOrder(source.Address()+0x654u,0u);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
        context->Release();
    } catch(...) {context->Release();throw;}
    std::cout<<"Party membership: compiled add/remove, signed order and dual fields, repeat/invalid indices, missing/duplicate/valid equipment, original DB hint/sound and foreign-owner rejection verified\n";
}
}
