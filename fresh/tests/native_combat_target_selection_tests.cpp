#include "pal4/native_combat_target_selection.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_script_event_router.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
void Check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
struct Services final : pal4::INativeCombatTargetSelectionServices {
    // Explicit protocol fixture, not the production CombatSystem/InputManager.
    std::array<std::uint32_t,130> combat{};
    std::uint32_t input{};
    std::vector<std::string> trace;
    std::function<void()> during;
    bool fail{},missing_combat{},missing_input{};
    std::uint32_t CombatSystemAddress() override{return missing_combat?0u:reinterpret_cast<std::uint32_t>(combat.data());}
    std::uint32_t InputManagerAddress() override{return missing_input?0u:reinterpret_cast<std::uint32_t>(&input);}
    std::uint32_t DetermineTargetMode(std::uint32_t c) override {
        auto* w=reinterpret_cast<std::uint32_t*>(c);
        Check(w[10]==716u+123u && w[11]==456u && w[12]==789u && w[13]==6u &&
              *reinterpret_cast<std::uint8_t*>(c+82u)==1u && w[17]==0u,"start copies payload before mode calculation");
        trace.push_back("mode");if(during)during();if(fail)throw std::runtime_error("fixture mode failure");return 5u;
    }
    std::uint32_t RefreshSelection(std::uint32_t c) override {
        auto* w=reinterpret_cast<std::uint32_t*>(c);Check(w[18]==5u,"mode stored before candidate refresh");
        trace.push_back("refresh");w[16]=0x123456u;return 0u;
    }
    std::uint32_t InitializeInput(std::uint32_t a) override {
        Check(a==reinterpret_cast<std::uint32_t>(&input),"real supplied input identity reaches reset");trace.push_back("input");return 0u;
    }
    std::uint32_t SetTargetSelected(std::uint32_t a,std::uint32_t v) override {
        Check(a==0x123456u && !v,"stop unselects current target");trace.push_back("unselect");return 0u;
    }
};
}
int main(){try {
    pal4::NativeScriptEventRouter router;Services services;
    pal4::NativeCombatSystemImage parent;
    static_assert(sizeof(parent)==520u);auto suffix=std::as_writable_bytes(std::span(&parent,1u)).subspan(84u);std::fill(suffix.begin(),suffix.end(),std::byte{0xfe});
    const auto payload=std::array<std::uint32_t,4>{839u,456u,789u,6u};
    const auto payload_address=reinterpret_cast<std::uint32_t>(payload.data());
    const auto hide=router.Subscribe(router.Key("hide_monster_tips"),1u,[&](std::uint32_t a){
        Check(*reinterpret_cast<std::uint8_t*>(a)==0u,"hide payload is byte0");services.trace.push_back("hide");
        Check(parent.target_selection.words[16]==0x123456u,"hide dispatch precedes target clear");
    });
    for(unsigned cycle=0;cycle<3u;++cycle){
      {
        pal4::NativeCombatTargetSelection owner(router,services,parent.target_selection);
        Check(owner.Address()==reinterpret_cast<std::uint32_t>(&parent) && owner.SubscriptionCount()==5u &&
              router.SubscriptionCount()==6u,"five original subscriptions in actual parent base");
        Check(parent.target_selection.words[10]==0u && parent.target_selection.words[13]==6u && parent.target_selection.words[18]==2u &&
              parent.target_selection.words[19]==0u && (*reinterpret_cast<std::uint32_t*>(owner.Address()+80u)&0xffffffu)==0u,
              "original facet/default mode/time/flags");
        const auto before=parent.target_selection.words;bool rejected=false;
        try {pal4::NativeCombatTargetSelection duplicate(router,services,parent.target_selection);}
        catch(const std::logic_error&){rejected=true;}
        Check(rejected && parent.target_selection.words==before && owner.SubscriptionCount()==5u,"duplicate rejects before mutation");
        owner.Subscribe();Check(owner.SubscriptionCount()==5u && router.SubscriptionCount()==6u,"resubscribe replaces five callbacks");
        auto start=[&]{services.trace.clear();(void)router.Dispatch("start_select_target",payload_address);};
        auto stop=[&](const char* name){(void)router.Dispatch(name,0u);Check(!parent.target_selection.words[11] &&
            !parent.target_selection.words[16] && !parent.target_selection.words[17] && !*reinterpret_cast<std::uint8_t*>(owner.Address()+82u),
            "stop clears original selection fields");};
        services.combat[127]=3u;start();
        Check(services.trace==std::vector<std::string>{"mode","refresh","input"},"start service order");
        stop("stop_select_target");Check(services.trace==std::vector<std::string>{"mode","refresh","input","unselect","hide"},"stop service order");
        services.combat[127]=7u;start();stop("combat_over");
        Check(std::find(services.trace.begin(),services.trace.end(),"unselect")==services.trace.end(),"phase7 skips actor unselect");
        for(const auto name:{"close_system","combat_prepare"}){start();stop(name);}
        services.during=[&]{bool caught=false;try{owner.Close();}catch(const std::logic_error&){caught=true;}Check(caught,"reject close during callback");};
        start();services.during={};stop("stop_select_target");
        services.during=[&]{bool caught=false;try{(void)router.Dispatch("start_select_target",payload_address);}catch(const std::logic_error&){caught=true;}Check(caught,"reject nested callback");};
        start();services.during={};stop("stop_select_target");
        services.fail=true;rejected=false;try{start();}catch(const std::runtime_error&){rejected=true;}
        Check(rejected && parent.target_selection.words[18]==5u,"mode exception propagates without invented rollback");services.fail=false;
        start();stop("stop_select_target");
        rejected=false;try{(void)router.Dispatch("start_select_target",0u);}catch(const std::invalid_argument&){rejected=true;}Check(rejected,"null start payload rejected");
        services.missing_input=true;rejected=false;try{start();}catch(const std::logic_error&){rejected=true;}Check(rejected,"missing input rejected");services.missing_input=false;
        services.missing_combat=true;rejected=false;try{stop("stop_select_target");}catch(const std::logic_error&){rejected=true;}Check(rejected,"missing combat rejected");services.missing_combat=false;
        start();stop("stop_select_target");
        if(cycle!=1u){owner.Close();owner.Close();Check(owner.SubscriptionCount()==0u && router.SubscriptionCount()==1u,"idempotent close retains foreign subscriber");
          rejected=false;try{owner.Subscribe();}catch(const std::logic_error&){rejected=true;}Check(rejected,"closed owner cannot resubscribe");}
      }
      Check(router.SubscriptionCount()==1u && std::all_of(suffix.begin(),suffix.end(),[](auto v){return v==std::byte{0xfe};}),"destruction unbinds parent and preserves other fields");
    }
    (void)router.Unsubscribe(hide);
    {pal4::NativeCombatTargetSelection standalone(router,services);Check(standalone.SubscriptionCount()==5u,"standalone base supports same lifetime");}
    Check(!router.SubscriptionCount(),"all owned callbacks retired");
    std::cout<<"Native target selection: real shared ctor/start/stop, five host subscriptions, modes/phase7, ordering, failure/reentry, parent reuse and cleanup pass; game services are explicit fixtures\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
