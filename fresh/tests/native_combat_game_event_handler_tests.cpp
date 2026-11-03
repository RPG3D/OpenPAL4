#include "pal4/native_combat_game_event_handler.h"
#include "pal4/native_game_database.h"
#include "pal4/native_script_event_router.h"
#include "pal4/combat_game_event_iterations.h"
#include <array>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

namespace {
void Check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F> void Reject(F f){bool b{};try{f();}catch(const std::logic_error&){b=true;}Check(b,"expected lifecycle/service rejection");}
// Protocol fixture only: this does not supply the application's live roster.
class Actors final : public pal4::ICombatGameEventIterationRuntime {
public:
    std::map<std::uint32_t,std::uint32_t> words;
    unsigned destroyed{},advanced{};
    std::uint32_t ReadWord(std::uint32_t a) override{return words.at(a);}
    void WriteWord(std::uint32_t,std::uint32_t) override{throw std::logic_error("foreign write");}
    std::uint32_t ActorFromFacet(std::uint32_t e) override{Check(e==0x1000u,"actor event");return 0x2000u;}
    std::uint32_t UpdateStat(std::uint32_t,std::uint32_t,std::uint32_t) override{throw std::logic_error("must use native stats");}
    std::uint32_t GetActorManager() override{return 0x4000u;}
    std::uint32_t CreateIterator(std::uint32_t c) override{Check(c==0x4020u,"Player first collection");advanced=0;return 1u;}
    std::uint32_t IteratorDone(std::uint32_t) override{return advanced;}
    std::uint32_t IteratorCurrent(std::uint32_t) override{return 0x2000u;}
    void IteratorAdvance(std::uint32_t) override{++advanced;}
    std::uint32_t DestroyIterator(std::uint32_t,std::uint32_t f) override{Check(f==1u,"iterator deletion");return ++destroyed;}
    std::uint32_t MonsterState(std::uint32_t) override{return 2u;}
    std::uint32_t ActorProperty52(std::uint32_t,std::uint32_t) override{return 10u;}
    std::uint32_t ActorProperty48(std::uint32_t,std::uint32_t) override{return 10u;}
    std::uint32_t ActorMetric252(std::uint32_t) override{return 0u;}
    std::int32_t AverageProperty4(std::uint32_t c) override{return c==0x4000u?5:8;}
};
}
int main(int argc,char** argv){try {
    pal4::NativeGameDatabase db;
    Check(db.Load(argc>1?argv[1]:"I:/Games/original"),"load original database");
    const auto& rows=db.Tables().at("CombatPrize").rows;
    std::array<std::uint32_t,32> coeff{},limit{};
    // Independent original 4E8500 field order; packed DB columns differ.
    constexpr std::array<const char*,32> names{
        "KEY","BASE","PRIZE_S","PRIZE_1","PRIZE_2","PRIZE_3",
        "EST_PROTECT","EST_JOINT_ATK","EST_CRITICAL","EST_MISSED",
        "EST_FEND_OFF","EST_REACT_ATK","EST_CAST_MAGIC","EST_STUNT",
        "EST_USE_PROPERTY","EST_REVIVE","EST_FLEE_FAIL","EST_ROLE_DEAD",
        "EST_UNATTACKED","EST_HP_IN_MAX","EST_HP_IN_DANGER","EST_TAKE_ACTION",
        "EST_MONSTER_DIDNT_ACT","EST_MONSTER_FLEE","EST_MONSTER_DEAD",
        "EST_LEVEL_UPPER","EST_LEVEL_LOWER","EST_PHYSICAL_ATK","EST_KNOCK_OUT",
        "EST_WIN_AT_THE_FIRST_BOUT","EST_KO_BY_ONE_ROLE","EST_ALL_SURVIVE"};
    const auto& fields=db.Tables().at("CombatPrize").fields;
    for(const auto& row:rows){auto id=std::get<std::uint32_t>(row[0]);if(id<2u)for(unsigned i=0;i<32u;++i){
        std::size_t column{};while(column<fields.size() && fields[column].name!=names[i])++column;
        Check(column<fields.size(),"original CombatPrize named column");
        (id?limit:coeff)[i]=std::get<std::uint32_t>(row[column]);}}
    Check(coeff[2]==230u && coeff[3]==180u && coeff[4]==130u && coeff[5]==80u,
        "original asset rank thresholds are independent of event coefficient columns");
    // Borrowed CombatSystem prefix fixture; full production owner is pending.
    std::array<std::uint32_t,130> combat{};
    pal4::NativeScriptEventRouter router;
    const auto unrelated=router.Subscribe(router.Key("unrelated"),1u,[](std::uint32_t){});
    const auto combat_address=reinterpret_cast<std::uint32_t>(combat.data());
    std::uint32_t foreign_slot{};
    Reject([&]{pal4::NativeCombatGameEventHandler bad(db,router,combat_address,foreign_slot);});
    Check(!foreign_slot && router.SubscriptionCount()==1u,"foreign publication rejected before subscriptions");
    pal4::NativeGameDatabase missing_database;
    bool missing_rejected{};
    try{pal4::NativeCombatGameEventHandler bad(missing_database,router,combat_address,combat[104]);}
    catch(const std::runtime_error&){missing_rejected=true;}
    Check(missing_rejected && !combat[104] && router.SubscriptionCount()==1u,
          "failed member construction leaves slot/router unchanged");
    pal4::NativeCombatGameEventHandler owner(db,router,combat_address,combat[104]);
    Check(combat[104]==owner.Address(),"member416 publication follows successful construction");
    Reject([&]{pal4::NativeCombatGameEventHandler duplicate(db,router,combat_address,combat[104]);});
    Check(combat[104]==owner.Address() && router.SubscriptionCount()==18u,"duplicate publication preserves live owner");
    auto word=[&](unsigned offset){return *reinterpret_cast<const std::uint32_t*>(owner.Address()+offset);};
    Check(owner.SubscriptionCount()==17u && router.SubscriptionCount()==18u,"17 native subscriptions");
    for(unsigned i=0;i<26u;++i)Check(word(448+4*i)==coeff[6+i] && word(344+4*i)==limit[6+i],"original coefficient/limit copy");
    for(unsigned i=0;i<5u;++i)Check(word(600+4*i)==coeff[1+i],"original rank thresholds");
    std::cout<<"Original CombatPrize baseline="<<owner.Total()<<" thresholds";
    for(unsigned i=2;i<6u;++i)std::cout<<' '<<static_cast<std::int32_t>(coeff[i]);
    std::cout<<'\n';
    Reject([&]{(void)router.Dispatch("ae_joint_attack",0x1000u);});
    std::vector<std::uint32_t> deltas;
    auto observer=router.Subscribe(router.Key("score_changed"),1u,[&](std::uint32_t p){deltas.push_back(*reinterpret_cast<const std::uint32_t*>(p));});
    for(unsigned i=0;i<26u;++i){
        owner.Reset();
        (void)owner.UpdateStat(i,1000u);
        const auto raw=coeff[6+i]*1000u;
        auto expected=raw;
        const auto s=static_cast<std::int32_t>(raw),l=static_cast<std::int32_t>(limit[6+i]);
        if((s>0 && s>l)||(s<=0 && s<l))expected=limit[6+i];
        Check(word(136+4*i)==expected && word(240+4*i)==1u && deltas.back()==raw,"signed clamp/count/raw event delta");
        Check(owner.Total()==coeff[1]+expected,"total uses clamped score");
    }
    for(unsigned i=0;i<4u;++i){const auto threshold=static_cast<std::int32_t>(coeff[2+i]);
        auto expected=[&](std::int32_t score){for(unsigned j=0;j<4u;++j)if(score>static_cast<std::int32_t>(coeff[2+j]))return j;return 4u;};
        Check(owner.Rank(threshold)==expected(threshold) && owner.Rank(threshold+1)==expected(threshold+1),"strict threshold boundaries");}
    combat[129]=1u;Check(owner.Rank(100000)==4u,"inactive Combat rank");combat[129]=0u;
    Actors actors;actors.words={{0x1004u,0x3000u},{0x2000u+804u,1u},{0x2000u+1192u,0u},{0x3000u+804u,1u},{0x3000u+1192u,0u}};
    owner.BindActors(&actors);owner.Reset();
    const std::array<std::pair<const char*,unsigned>,14> cases{{
        {"ae_physical_attack",21},{"ae_stunt",7},{"ae_cast_magic",6},{"ae_use_property",8},
        {"ae_become_dead",11},{"ae_critical_attack",2},{"ae_fend_offed",4},{"ae_revive",9},
        {"ae_react_attack",5},{"ae_attack_missed",3},{"ae_joint_attack",1},{"ae_protect_action",0},
        {"ae_flee_fail",10},{"ae_fleed",17}}};
    for(const auto& [name,stat]:cases){actors.words[0x2000u+804u]=stat==17u?2u:1u;const auto before=word(240+4*stat);
        Check(router.Dispatch(name,0x1000u)==1u && word(240+4*stat)==before+1u,"named callback routes expected stat");}
    Check(router.Dispatch("re_can_take_action",0x1000u)==1u && word(568)==1u,"action count tail");
    owner.Reset();Check(router.Dispatch("combat_start",0)==1u && word(240+14*4)==1u,"combat start monster iteration");
    Check(router.Dispatch("combat_over",0)==1u && actors.destroyed==3u && word(240+12*4)==4u && word(240+13*4)==1u && word(240+16*4)==1u && word(240+19*4)==1u,"combat over scoring iterations");
    Check(router.Unsubscribe(observer),"remove observer");
    // The native composition creates stats storage before the action services.
    // A final death can synchronously dispatch combat_over and prepare a report.
    // Rebinding after those services must retain scores and finalize both events
    // before the report observes them (equal event modes are newest first).
    owner.Reset();
    (void)owner.UpdateStat(6u,1u);
    const auto retained_total=owner.Total();
    bool report_seen{};
    const auto report=router.Subscribe(router.Key("combat_over"),1u,[&](std::uint32_t){
        Check(word(240u+18u*4u)==1u,"final Monster death counted before report");
        Check(word(240u+12u*4u)==4u && word(240u+13u*4u)==1u &&
              word(240u+15u*4u)==1u,"battle-end stats finalized before report");
        report_seen=true;
    });
    const auto terminal=router.Subscribe(router.Key("ae_become_dead"),3u,[&](std::uint32_t){
        (void)router.Dispatch("combat_over",0u);
    });
    owner.Subscribe();
    Check(owner.Total()==retained_total,"rebind retains accumulated scores");
    actors.words[0x2000u+804u]=2u;
    (void)router.Dispatch("ae_become_dead",0x1000u);
    Check(report_seen,"nested final-death report completed");
    Check(router.Unsubscribe(terminal) && router.Unsubscribe(report),"retire report fixture");
    owner.Reset();
    bool nested{};
    observer=router.Subscribe(router.Key("score_changed"),1u,[&](std::uint32_t){
        Reject([&]{owner.Close();});Reject([&]{owner.Reset();});Reject([&]{owner.Subscribe();});Reject([&]{owner.BindActors(nullptr);});
        if(!nested){nested=true;(void)owner.UpdateStat(0,1u);}
    });
    (void)owner.UpdateStat(1,1u);Check(nested,"nested score change");
    Check(router.Unsubscribe(observer),"nested observer removed");
    observer=router.Subscribe(router.Key("score_changed"),1u,[](std::uint32_t){throw std::runtime_error("observer failure");});
    bool threw{};try{(void)owner.UpdateStat(0,1u);}catch(const std::runtime_error&){threw=true;}Check(threw,"observer exception propagated");
    Check(router.Unsubscribe(observer),"throw observer removed");owner.Reset();
    for(unsigned cycle=0;cycle<3u;++cycle)owner.Subscribe();
    Check(owner.SubscriptionCount()==17u && router.SubscriptionCount()==18u,"repeat subscription no leak");
    Reject([&]{(void)owner.UpdateStat(26u,1u);});
    const auto saved_owner=combat[104];combat[104]=0u;
    Reject([&]{owner.Close();});
    Check(owner.SubscriptionCount()==17u && router.SubscriptionCount()==18u,"foreign slot mutation rejects before unsubscribe");
    combat[104]=saved_owner;
    owner.Close();owner.Close();Check(!combat[104] && !owner.Address() && owner.SubscriptionCount()==0u && router.SubscriptionCount()==1u && router.Contains(unrelated),"close clears publication and only own handlers");
    Reject([&]{owner.Reset();});
    {pal4::NativeCombatGameEventHandler scoped(db,router,combat_address,combat[104]);
     Check(combat[104]==scoped.Address(),"retired member can bind new owner");}
    Check(!combat[104] && router.SubscriptionCount()==1u,"destructor clears member and subscriptions");
    std::cout<<"PASS original CombatPrize, 17 named callbacks, raw score events, nested/throwing observers and cleanup; actor/CombatSystem fixtures only\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
