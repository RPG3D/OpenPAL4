#pragma once
#include "pal4/native_game_script_callbacks.h"
#include "pal4/script_npc_hand_effect.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace pal4::test {
inline void VerifyNativeNpcHandCallbacks() {
    const auto check=[](bool ok) { if(!ok) throw std::runtime_error("NPC hand compiled callback contract"); };
    struct Runtime final : IScriptNpcHandEffectRuntime {
        std::vector<std::string> calls;
        std::uint32_t ReadWord(std::uint32_t address) override {
            if(address!=0x1014u) throw std::runtime_error("NPC hand unexpected GOB read");
            return 0x2000u;
        }
        std::uint32_t EmptyStringAddress() override { return reinterpret_cast<std::uint32_t>(""); }
        std::uint32_t NpcManager() override { return 0x3000u; }
        std::uint32_t FindNpc(std::uint32_t manager,std::uint32_t text) override {
            const std::string name=reinterpret_cast<const char*>(text);
            if(manager!=NpcManager()) throw std::runtime_error("NPC hand manager projection");
            return name=="MN005"?0x4000u:0u;
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
            if(manager!=ObjectManager()) throw std::runtime_error("NPC hand GOB manager projection");
            return std::string(reinterpret_cast<const char*>(text))=="MO005"?0x1000u:0u;
        }
        std::uint32_t ArenaManager() override { return 0x6000u; }
        void RemoveAnimatedObject(std::uint32_t arena,std::uint32_t object) override {
            if(arena!=ArenaManager() || object!=0x1000u) throw std::runtime_error("NPC hand GOB removal");
            calls.emplace_back("remove");
        }
        std::uint32_t RenderUnit(std::uint32_t npc,std::uint32_t clump,const std::array<float,3>& scale,
            std::uint32_t text,bool left) override {
            if(npc!=0x4000u || clump || scale!=std::array<float,3>{1,1,1}) throw std::runtime_error("NPC hand unit branch");
            calls.emplace_back(std::string(left?"unit-left:":"unit-right:")+reinterpret_cast<const char*>(text));return 1u;
        }
        std::uint32_t RenderObject(std::uint32_t npc,std::uint32_t clump,std::uint32_t scale,
            std::uint32_t text,bool left) override {
            if(npc!=0x4000u || clump!=0x2000u || scale!=0x10b8u) throw std::runtime_error("NPC hand GOB borrow projection");
            calls.emplace_back(std::string(left?"gob-left:":"gob-right:")+reinterpret_cast<const char*>(text));return 1u;
        }
        std::uint32_t CleanupWeapons(std::uint32_t) override { calls.emplace_back("detach");return 1u; }
    } runtime;
    NativeAngelScriptEngine engine;
    NativeGameScriptServices services{};services.npc_hand_effect=&runtime;
    NativeGameScriptCallbacks callbacks(services);engine.SetCallbacks(&callbacks);
    constexpr char source[]=R"(
        void Probe() {
            giGOBAttachToNpc("MO005","H_083","MN005",true);
            giGOBDetachFromNPC("MN005");
            giGOBAttachToNpc("MO005","","MN005",false);
            giGOBAttachToNpc("","H_083","MN005",false);
            giGOBAttachToNpc("","H_083","MN005",true);
            giGOBAttachToNpc("","","MN005",true);
            giGOBAttachToNpc("MO005","H_083","missing",true);
            giGOBAttachToNpc("missing","H_083","MN005",true);
            giEffectAttachToNpc("MN005","H_083",true);
            giEffectAttachToNpc("MN005","H_083",false);
            giEffectDetachFromNpc("MN005");
            giGOBDetachFromNPC("missing");
        }
    )";
    check(engine.Raw()->AddScriptSection("npc_hand","npc_hand.cs",source,sizeof(source)-1)>=0 && engine.Raw()->Build("npc_hand")>=0);
    asIScriptContext* context{};check(engine.Raw()->CreateContext(&context)>=0 && context);
    const auto prepared=context->Prepare(engine.FunctionId("npc_hand","Probe"));
    const auto result=prepared>=0?context->Execute():prepared;
    context->Release();
    check(result==asEXECUTION_FINISHED);
    check(runtime.calls==std::vector<std::string>{"remove","gob-left:H_083","detach","remove","gob-right:",
        "unit-right:H_083","unit-left:H_083","left:H_083","right:H_083","update"});
    std::cout<<"NPC hand compiled callbacks: four native bridges, distinct string arguments, both hands, removal-before-borrow, unit/empty/missing branches and detach pass; domain services are contract fixtures\n";
}
}
