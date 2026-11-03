#pragma once
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_player_event_runtime.h"
#include "pal4/native_upgrade_record_catalog.h"
#include "pal4/native_script_event_router.h"
#include <array>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatPlayerGrowth(NativeCombatPlayerManager& players,
    const std::vector<std::unique_ptr<NativeCombatActorSkillRuntime>>& runtimes,
    NativePlayerEventRuntime& data,const NativeUpgradeRecordCatalog& upgrades,NativeScriptEventRouter& router) {
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    unsigned checked{};
    for(std::size_t i=0;i<players.Count();++i) {
        const auto actor=players.ActorAt(i);auto& owner=players.Actor(actor);
        NativeCombatActorSkillRuntime* skills{};
        for(const auto& r:runtimes)if(r->ActorAddress()==actor)skills=r.get();
        check(skills!=nullptr,"growth borrows actual Player skill runtime");
        const auto index=word(actor+1192u);
        const NativeUpgradeRecordCatalog::Words* row{};
        for(std::size_t n=0;n<upgrades.Count();++n) {
            const auto& candidate=upgrades.Record(n);
            if(candidate[37u]==index+1001u && candidate[38u]==3u){row=&candidate;break;}
        }
        check(row!=nullptr,"growth uses original role's level3 UpgradeData");
        // Only this original call's scalar operands are controlled. Collections,
        // model ownership and persistent Scene players are never replaced.
        std::array<std::byte,672> saved{};
        std::memcpy(saved.data(),reinterpret_cast<const void*>(actor+4u),saved.size());
        struct Restore {std::uint32_t actor;decltype(saved)& saved;
            ~Restore(){std::memcpy(reinterpret_cast<void*>(actor+4u),saved.data(),saved.size());}
        } restore{actor,saved};
        const auto allocations=owner.OwnedAllocations(),texts=owner.OwnedTexts(),arrays=owner.OwnedArrays(),buffers=skills->OwnedBuffers();
        check(!owner.AnimationCount(),"growth fixture starts with the actual Player animation queue empty");
        std::array<std::uint32_t,5> modifiers{},elements{};
        for(unsigned n=0;n<5u;++n){modifiers[n]=word(actor+128u+4u*n);elements[n]=word(actor+464u+4u*n);}
        (void)skills->SetCurrentStatViaActor(4u,3);
        (void)skills->SetCurrentStatViaActor(7u,1);
        (void)skills->SetCurrentStatViaActor(9u,0);
        bool inject=true,in_growth{},reentered{},close_rejected{},destroy_rejected{};unsigned notifications{};
        const auto token=router.Subscribe(router.Key("re_attr_int_ext_changed"),3u,[&](std::uint32_t){
            if(!in_growth)return;
            ++notifications;
            try{(void)skills->RefreshPlayerGrowth(data);}catch(const std::logic_error&){reentered=true;}
            try{skills->Close();}catch(const std::logic_error&){close_rejected=true;}
            try{owner.Destroy();}catch(const std::logic_error&){destroy_rejected=true;}
            if(inject)throw std::runtime_error("controlled growth property observer");
        });
        struct Unsubscribe{NativeScriptEventRouter& router;std::uint32_t token;~Unsubscribe(){(void)router.Unsubscribe(token);}} unsubscribe{router,token};
        bool threw{};
        in_growth=true;
        try{(void)skills->RefreshPlayerGrowth(data);}catch(const std::runtime_error& e){threw=std::string_view(e.what())=="controlled growth property observer";}
        in_growth=false;
        check(threw && notifications && reentered && close_rejected && destroy_rejected,
            "actual growth events preserve actor pins and reject recursive growth/close/destruction");
        inject=false;
        // Original exceptions do not promise rollback. Reinitialize only the
        // controlled HP/MP operands before retrying this same native service.
        (void)skills->SetCurrentStatViaActor(7u,1);
        (void)skills->SetCurrentStatViaActor(9u,0);
        in_growth=true;
        const auto returned=skills->RefreshPlayerGrowth(data);
        in_growth=false;
        check(skills->CurrentStat(4u)==3 && returned==skills->StatParameter(2u) &&
            skills->CurrentStat(7u)==skills->StatParameter(0u) && skills->CurrentStat(9u)==skills->StatParameter(2u),
            "actual53A440/581690 retries and restores Player level, full HP/MP and original return");
        for(unsigned n=1u;n<31u;++n)check(word(actor+4u+4u*n)==(*row)[n],"growth copies original UpgradeData base fields");
        for(unsigned n=0;n<5u;++n)check(word(actor+128u+4u*n)==modifiers[n] && word(actor+464u+4u*n)==elements[n],
            "growth preserves cached modifier and original element property values");
        // Actual re_attr_int_ext_changed subscribers retain one owned queue
        // node per floating-value animation. These are live work until the real
        // queue releases them; they are not growth scratch or a leaked callback.
        check(owner.AnimationCount() && owner.OwnedAllocations()==allocations+owner.AnimationCount(),
            "actual Player property events own all additional animation queue nodes");
        owner.ClearAnimations();
        check(owner.OwnedAllocations()==allocations && owner.OwnedTexts()==texts && owner.OwnedArrays()==arrays && skills->OwnedBuffers()==buffers,
            "growth releases scratch and retains actual actor collection/model/skill ownership");
        ++checked;
    }
    check(checked==2u,"growth verifies both actual NewGame combat Players");
    std::cout<<"Native53A440/581690 growth: two actual Players/original level3 UpgradeData, preserved five modifiers, real stat rebuild/full HP-MP, event pins/reentry guards, exception retry and actual floating-animation ownership/cleanup pass\n";
}
}
