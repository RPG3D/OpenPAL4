#pragma once
#include "pal4/native_combat_player_actor_owner.h"
#include "pal4/native_player_manager_session.h"
#include "pal4/actor_manager_player_combat_actor.h"
#include "pal4/ai_action_task.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace pal4::test {
inline void VerifyNativeCombatPlayerActors(NativePlayerManagerSession& source) {
    const auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    const auto word=[](std::uint32_t a){std::uint32_t v;std::memcpy(&v,reinterpret_cast<const void*>(a),4u);return v;};
    const auto text=[&](std::uint32_t r){const auto p=word(r+4u),n=word(r+8u);check(p || !n,"source Player text bounds");return p?std::string(reinterpret_cast<const char*>(p),n):std::string{};};
    const auto snapshot=source.State();
    std::array<NativeCombatPlayerActorOwner,4> actors;
    std::size_t array_words{},strings{};
    for(unsigned i=0;i<4u;++i) {
        const auto s=source.Address()+i*PlayerManagerObject::kElementBytes;
        const auto a=actors[i].Construct(source,i);
        check(word(a)==ActorManagerPlayerCombatActor::kVtable && word(a+804u)==1u,"actual Player derived type and actor mode1");
        check(word(word(a+800u))==0x846130u && word(word(a+1288u))==AiActionSecondaryLocaleFacet::kVtable && word(word(a+1288u)+4u)==a,"actual Player count property and self facet");
        check(!std::memcmp(reinterpret_cast<const void*>(a+4u),reinterpret_cast<const void*>(s+1624u),332u),"Player first stat block excluding original reset336");
        check(!std::memcmp(reinterpret_cast<const void*>(a+340u),reinterpret_cast<const void*>(s+1960u),332u),"Player current stat block excluding original reset672");
        check(!word(a+336u) && !word(a+672u) && !word(a+808u) && !word(a+1296u),"constructor resets counters and leaves model list detached");
        const auto array=[&](std::uint32_t d,std::uint32_t r) {
            const auto begin=word(r+4u),bytes=word(r+8u)-begin;
            check(word(d+8u)-word(d+4u)==bytes,"Player copied collection length");
            if(bytes) {check(begin!=word(d+4u) && !std::memcmp(reinterpret_cast<const void*>(begin),reinterpret_cast<const void*>(word(d+4u)),bytes),"Player collection deep copy preserves values/order");array_words+=bytes/4u;}
        };
        array(a+676u,s+2296u);array(a+692u,s+2312u);
        for(unsigned j=0;j<5u;++j){array(a+920u+16u*j,s+2360u+16u*j);array(a+1000u+16u*j,s+2440u+16u*j);}
        array(a+1080u,s+2520u);array(a+1096u,s+2536u);
        for(unsigned j=0;j<9u;++j) {
            const auto d=j<4u?a+1112u+16u*j:a+1208u+16u*(j-4u);
            const auto r=j<4u?s+2552u+16u*j:s+2648u+16u*(j-4u);
            check(text(d)==text(r),"Player copied model/action text");
            if(word(r+8u)){check(word(d+4u)!=word(r+4u),"Player host text independent from source");++strings;}
        }
        check(actors[i].OwnedAllocations()==5u,"Player damage/trees/queue/facet actual owned allocations");
        bool rejected=false;try{actors[i].Construct(source,i);}catch(const std::logic_error&){rejected=true;}
        check(rejected,"duplicate Player construct rejects");
    }
    check(!std::memcmp(&snapshot,&source.State(),sizeof(snapshot)),"four live Combat Player copies preserve entire actual scene session");
    const auto src=source.Address()+2584u;
    const auto original=text(src);source.CopyRuntime().AssignText(src,"probe-independent-source");
    try {check(text(actors[0].Address()+1144u)==original,"Combat Player text survives source replacement");}
    catch(...) {source.CopyRuntime().AssignText(src,original);throw;}
    source.CopyRuntime().AssignText(src,original);
    NativeCombatPlayerActorOwner bad;bool rejected=false;
    try{bad.Construct(source,4u);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected && !bad.Address(),"Player invalid index leaves no partial owner");
    // Corrupt only a borrowed array view for the failure-path call, then restore
    // it before any live source runtime can consume it.
    const auto r=source.Address()+2360u,old=word(r+8u);
    const auto invalid=word(r+12u)+4u;std::memcpy(reinterpret_cast<void*>(r+8u),&invalid,4u);
    try {rejected=false;try{bad.Construct(source,0u);}catch(const std::invalid_argument&){rejected=true;}}
    catch(...) {std::memcpy(reinterpret_cast<void*>(r+8u),&old,4u);throw;}
    std::memcpy(reinterpret_cast<void*>(r+8u),&old,4u);
    check(rejected && !bad.Address(),"malformed source collection leaves no partial Player owner");
    for(unsigned i=0;i<4u;++i) {
        auto& owner=actors[i];const auto a=owner.Address();
        auto* child=reinterpret_cast<std::uint32_t*>(a+808u);*child=1u;
        rejected=false;try{owner.Destroy();}catch(const std::logic_error&){rejected=true;}*child=0u;
        check(rejected && owner.Address()==a,"bound model rejects Player teardown before mutation");
        auto* foreign_text=reinterpret_cast<std::uint32_t*>(a+1212u);const auto saved_text=*foreign_text;
        *foreign_text=1u;rejected=false;try{owner.Destroy();}catch(const std::logic_error&){rejected=true;}*foreign_text=saved_text;
        check(rejected && word(a)==ActorManagerPlayerCombatActor::kVtable,"foreign text rejects before shared destructor mutates actor");
        owner.Destroy();owner.Destroy();check(!owner.Address() && !owner.OwnedAllocations() && !owner.OwnedTexts() && !owner.OwnedArrays(),"Player real detached destruction balanced");
        (void)owner.Construct(source,i);owner.Destroy();
    }
    {
        // NewGame's original learned/active collections are empty here. Exercise
        // populated source views independently, restoring every borrowed field.
        std::array<std::uint32_t,6> values{1001u,2u,3u,1002u,5u,6u};
        std::vector<std::array<std::uint32_t,4>> saved;
        struct Restore {std::vector<std::array<std::uint32_t,4>>& saved;~Restore(){for(const auto& r:saved)std::memcpy(reinterpret_cast<void*>(r[0]+4u),r.data()+1u,12u);}} restore{saved};
        const auto fixture=[&](unsigned offset) {
            const auto r=source.Address()+offset;saved.push_back({r,word(r+4u),word(r+8u),word(r+12u)});
            const auto begin=reinterpret_cast<std::uint32_t>(values.data());
            const std::array<std::uint32_t,3> view{begin,begin+sizeof(values),begin+sizeof(values)};
            std::memcpy(reinterpret_cast<void*>(r+4u),view.data(),12u);
        };
        for(auto o:{2296u,2312u,2520u,2536u})fixture(o);
        for(unsigned i=0;i<5u;++i){fixture(2360u+16u*i);fixture(2440u+16u*i);}
        NativeCombatPlayerActorOwner populated;const auto a=populated.Construct(source,0u);
        check(populated.OwnedArrays()==14u,"all14 populated Player collections use host owners");
        for(auto offset:{676u,692u,1080u,1096u,920u,936u,952u,968u,984u,1000u,1016u,1032u,1048u,1064u}) {
            check(word(a+offset+8u)-word(a+offset+4u)==sizeof(values) &&
                !std::memcmp(reinterpret_cast<const void*>(word(a+offset+4u)),values.data(),sizeof(values)),"word/pair/triple source copies preserve populated records");
        }
        values.fill(0u);check(word(word(a+680u))==1001u,"populated Player collections survive source mutation");
        auto* foreign_array=reinterpret_cast<std::uint32_t*>(a+680u);const auto saved_array=*foreign_array;
        *foreign_array=1u;rejected=false;try{populated.Destroy();}catch(const std::logic_error&){rejected=true;}*foreign_array=saved_array;
        check(rejected && word(a)==ActorManagerPlayerCombatActor::kVtable,"foreign array rejects before shared destructor");
        populated.Destroy();check(!populated.OwnedArrays(),"populated Player teardown retires all host collections");
    }
    std::cout<<"Native537940 Player actors: actual4 scene slots, count property846130, original copied words="<<array_words<<" strings="<<strings<<", populated fixture14 arrays/84 words, source replacement independence, foreign ownership rejection/retry and balanced repeated teardown pass\n";
}
}
