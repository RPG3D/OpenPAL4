#pragma once
#include "pal4/native_game_script_callbacks.h"
#include "pal4/script_player_gob_attachment.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace pal4::test {
inline void VerifyNativePlayerHandCallbacks() {
    const auto check=[](bool ok) { if(!ok) throw std::runtime_error("player hand compiled callback contract"); };
    struct Runtime final : IScriptPlayerGobAttachmentRuntime {
        std::vector<std::string> calls;
        std::uint32_t ReadWord(std::uint32_t address) override {
            if(address==0x5c50u) return 0u;
            if(address!=0x1014u) throw std::runtime_error("player hand unexpected GOB read");
            return 0x2000u;
        }
        std::uint32_t EmptyStringAddress() override { return reinterpret_cast<std::uint32_t>(""); }
        std::uint32_t PlayerManager() override { return 0x3000u; }
        std::uint32_t PlayerByIndex(std::uint32_t manager,std::int32_t index) override {
            if(manager!=PlayerManager()) throw std::runtime_error("player hand manager projection");
            return index==0?0x4000u:index==2?0x6000u:0u;
        }
        std::uint32_t SetupLeftHandEffect(std::uint32_t,std::uint32_t text) override {
            calls.emplace_back("left:"+std::string(reinterpret_cast<const char*>(text)));return 1u;
        }
        std::uint32_t SetupRightHandEffect(std::uint32_t,std::uint32_t text) override {
            calls.emplace_back("right:"+std::string(reinterpret_cast<const char*>(text)));return 1u;
        }
        std::uint32_t UpdateWeaponEffects(std::uint32_t) override { calls.emplace_back("update");return 1u; }
        bool NameDiffersFromEmptyError(std::uint32_t,std::uint32_t) override {
            throw std::runtime_error("native script text was not projected");
        }
        std::uint32_t ObjectManager() override { return 0x5000u; }
        std::uint32_t FindObject(std::uint32_t manager,std::uint32_t text) override {
            if(manager!=ObjectManager()) throw std::runtime_error("player hand GOB manager projection");
            return std::string(reinterpret_cast<const char*>(text))=="MO005"?0x1000u:0u;
        }
        std::uint32_t ArenaManager() override { return 0x6000u; }
        void RemoveAnimatedObject(std::uint32_t arena,std::uint32_t object) override {
            if(arena!=ArenaManager() || object!=0x1000u) throw std::runtime_error("player hand GOB removal");
            calls.emplace_back("remove");
        }
        std::uint32_t RenderUnit(std::uint32_t npc,std::uint32_t clump,const std::array<float,3>& scale,
            std::uint32_t text,bool left) override {
            if(npc!=0x4000u || clump || scale!=std::array<float,3>{1,1,1}) throw std::runtime_error("player hand unit branch");
            calls.emplace_back(std::string(left?"unit-left:":"unit-right:")+reinterpret_cast<const char*>(text));return 1u;
        }
        std::uint32_t RenderObject(std::uint32_t npc,std::uint32_t clump,std::uint32_t scale,
            std::uint32_t text,bool left) override {
            if(npc!=0x4000u || clump!=0x2000u || scale!=0x10b8u) throw std::runtime_error("player hand GOB borrow projection");
            calls.emplace_back(std::string(left?"gob-left:":"gob-right:")+reinterpret_cast<const char*>(text));return 1u;
        }
        std::uint32_t CleanupWeapons(std::uint32_t) override { calls.emplace_back("detach");return 1u; }
    } runtime;
    NativeAngelScriptEngine engine;
    NativeGameScriptServices services{};services.player_gob_attachment=&runtime;
    NativeGameScriptCallbacks callbacks(services);engine.SetCallbacks(&callbacks);
    constexpr char source[]=R"(
        void Probe() {
            giGOBAttachToPlayer("MO005","H_083",0,true);
            giGOBDetachFromPlayer(0);
            giGOBAttachToPlayer("MO005","",0,false);
            giGOBAttachToPlayer("","H_083",0,false);
            giGOBAttachToPlayer("","H_083",0,true);
            giGOBAttachToPlayer("","",0,true);
            giGOBAttachToPlayer("MO005","H_083",7,true);
            giGOBAttachToPlayer("missing","H_083",0,true);
            giGOBDetachFromPlayer(7);
        }
        void ProbeEffects() {
            giEffectAttachToPlayer(2,"H_TL03.2",false);
            giEffectAttachToPlayer(2,"H_TL03.2",true);
            giEffectAttachToPlayer(7,"H_TL03.2",false);
            giEffectAttachToCurrentPlayer("current_right",false);
            giEffectAttachToCurrentPlayer("current_left",true);
            giEffectAttachToCurrentPlayer("",false);
            giEffectDetachFromPlayer(2);
            giEffectDetachFromPlayer(7);
            giEffectDetachFromCurrentPlayer();
        }
    )";
    check(engine.Raw()->AddScriptSection("player_hand","player_hand.cs",source,sizeof(source)-1)>=0 && engine.Raw()->Build("player_hand")>=0);
    asIScriptContext* context{};check(engine.Raw()->CreateContext(&context)>=0 && context);
    const auto prepared=context->Prepare(engine.FunctionId("player_hand","Probe"));
    const auto result=prepared>=0?context->Execute():prepared;
    context->Release();
    check(result==asEXECUTION_FINISHED);
    check(runtime.calls==std::vector<std::string>{"remove","gob-left:H_083","detach","remove","gob-right:",
        "unit-right:H_083","unit-left:H_083"});
    runtime.calls.clear();
    check(engine.Raw()->CreateContext(&context)>=0 && context);
    check(context->Prepare(engine.FunctionId("player_hand","ProbeEffects"))>=0);
    check(context->Execute()==asEXECUTION_FINISHED);
    check(runtime.calls==std::vector<std::string>{"right:H_TL03.2","left:H_TL03.2",
        "right:current_right","left:current_left","right:","update","update"});
    callbacks.SetServices({});
    check(context->Prepare(engine.FunctionId("player_hand","ProbeEffects"))>=0);
    check(context->Execute()==asEXECUTION_EXCEPTION);
    check(std::string(context->GetExceptionString())=="PAL4 game script service -168 is unavailable");
    context->Release();
    std::cout<<"player effect compiled callbacks: indexed/current, bool hand direction, original H_TL03.2 name, empty/missing player, effect-only detach and unavailable service pass\n";
    std::cout<<"player hand compiled callbacks: two native bridges, distinct string arguments, both hands, removal-before-borrow, unit/empty/missing player/object branches and detach pass; domain services are contract fixtures\n";
}
}
