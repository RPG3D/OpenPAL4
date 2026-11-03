#include "native_combat_ending_sequence_probe.h"
#include "pal4/native_combat_ending_sequence.h"
#include "pal4/native_combat_ending_window.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_console_visuals.h"
#include "pal4/native_combat_console.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_upgrade_record_catalog.h"
#include "pal4/native_miles_sound_2d_runtime.h"
#include "pal4/native_combat_report.h"
#include "pal4/native_combat_result_actions.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/random_number_generator.h"
#include "../../native/ui/cegui_exports_native.h"
#include <bit>
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>
namespace pal4::test {
void VerifyNativeCombatEndingSequence(NativeCombatEndingWindow& window,NativeCombatPlayerManager& players,
    NativeMilesSound2dRuntime& sounds,NativeCombatSystemImage& image,const NativeUpgradeRecordCatalog& upgrades,
    NativeCombatEndingTasks::ActorSkills skills,NativeCombatConsoleVisuals& visuals,NativeCombatConsole& console,INativeSkillSystemServices& system){
    const auto check=[](bool v,const char* m){if(!v)throw std::runtime_error(m);};
    check(players.Count()==2u && !players.StageCount() && !window.TaskCount() && !window.Tasks().DomainCount(),
        "Ending sequence fixture borrows two real reward Players with naturally empty stage roster");
    NativeCombatTaskQueue battle(system);
    const auto before=sounds.SampleHandles();const std::set<std::uint32_t> initial(before.begin(),before.end());
    struct Restore{NativeMilesSound2dRuntime& sounds;const std::set<std::uint32_t>& initial;NativeCombatSystemImage& image;std::uint8_t mode;
        ~Restore(){image.Mode()=mode;try{for(auto h:sounds.SampleHandles())if(!initial.contains(h))(void)sounds.StopSample(h);}catch(...){}}
    } restore{sounds,initial,image,image.Mode()};image.Mode()=0u;
    unsigned scores{},ranks{},levels{},finishes{};NativeCombatEndingSequence* active{};
    NativeCombatEndingTaskServices services;
    services.score_text=[&]{++scores;visuals.ShowVictory();};
    services.rank_icon=[&](std::uint32_t c){++ranks;visuals.ShowRank(c,window);};
    services.level_up=[&](std::uint32_t row,std::int32_t level,std::uint32_t a,std::uint32_t hint,std::uint32_t record){
        const auto role=*reinterpret_cast<const std::uint32_t*>(a+1192u);++levels;
        check(role<2u && record==window.Address()+472u+40u*role && hint==window.Word(684u+4u*role),"sequence XP child supplies exact original Player report/hint");
        auto lease=window.BorrowPresentation();window.WritePresentationWord(row-window.Address()+72u,static_cast<std::uint32_t>(level));
    }; // Explicit protocol fixture: this test has no models or result-action owners.
    services.finish=[&](std::uint32_t result){++finishes;check(result==window.Word(436u),"original close payload survives full composed timeline");
        bool guard{};try{active->Start();}catch(const std::logic_error&){guard=true;}check(guard,"published live Ending sequence cannot compose from its own queue dispatch");
        (void)window.RunFinish([&]{window.SetVisible(false);return 0u;});
    }; // No synthetic Scene transition or synchronous576440 claim.
    NativeCombatEndingTasks tasks(window,players,sounds,image,upgrades,skills,services);
    NativeCombatEndingSequence sequence(window,tasks,players,battle,image,upgrades,skills,
        [](std::uint32_t)->NativeCombatResultActions&{throw std::logic_error("empty-stage fixture must not resolve a fabricated Win owner");});active=&sequence;
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto get_text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto cstr=detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ");
    for(const auto mode:{0u,1u}){
        image.Mode()=static_cast<std::uint8_t>(mode);sequence.Start();const auto root=sequence.LastSequence();
        check(window.Tasks().Contains(root) && tasks.Count()==1u && !players.ActiveIterators() && !battle.Count(),"shared547270 publishes actual EndTask and releases both roster cursors");
        const auto head=word(root+8u);std::vector<std::uint32_t> children;
        for(auto node=word(head);node!=head;node=word(node))children.push_back(word(node+8u));
        unsigned xp{},moves{};const auto prelude=mode==0u?2u:0u;
        check(children.size()>=11u && (mode?word(children[0])==0x845404u:word(children[0])==0x8453ccu),"active prelude and inactive skip use actual original child vtables");
        check(std::bit_cast<float>(word(children[prelude]+16u))==(mode?0.f:4.f),"original panel base delay follows Combat mode");
        for(auto child:children){
            check(word(child+12u)==2u && word(child+56u)==root+4u,"all composed GOM children retain absolute timing and shared container");
            if(word(child)==0x845420u)++moves;
            if(word(child)!=0x845440u)continue;++xp;
            const auto a=word(child+68u),role=word(a+1192u),key=word(a+1196u),record=52u+40u*role;
            const auto prior=std::bit_cast<std::int32_t>(window.Report().Word(record+28u));
            std::int32_t best=100,limit{};for(std::size_t n=0;n<upgrades.Count();++n){const auto& row=upgrades.Record(n);
                if(row[37]==key && std::bit_cast<std::int32_t>(row[39])>prior && std::bit_cast<std::int32_t>(row[38])<best){best=std::bit_cast<std::int32_t>(row[38]);limit=std::bit_cast<std::int32_t>(row[39]);}}
            check(best<100 && word(child+80u)==static_cast<std::uint32_t>(best) && std::bit_cast<float>(word(child+84u))==float(limit-prior-1),
                "actual sequence resolves prior XP against native UpgradeData using argument-free actor1196 key");
        }
        check(moves==2u && xp==2u && children.size()==11u+prelude+(window.Report().Items().empty()?0u:2u)+(window.Report().Items(true).empty()?0u:1u),
            "original sequence composes packed role rows and optional property/prize branches");
        unsigned frames{};while(window.TaskCount() && frames++<2400u){window.Update(.016);console.Update(.016f);sounds.Update();}
        check(!window.TaskCount() && !tasks.Count() && !tasks.ChildCount() && !tasks.OwnedAllocations() && window.Word(716u)==2u,
            "complete original547270 timeline runs through native DLL/XP tasks and retires parent/children/names/links");
        for(unsigned role=0;role<2u;++role){const auto row=116u+76u*role,a=players.ActorAt(role);
            const auto expected=skills(a).CurrentStat(5u)-std::bit_cast<std::int32_t>(window.Report().Word(52u+40u*role+28u));
            check(std::string(cstr(get_text(reinterpret_cast<void*>(window.Word(row+20u)))))==std::to_string(expected),"full sequence XP display reaches actual awarded amount");
        }
        console.Update(10.f);console.Update(.01f);check(!visuals.OutstandingAllocations(),"composed console banner/rank allocation retires");
        for(auto h:sounds.SampleHandles())if(!initial.contains(h))(void)sounds.StopSample(h);
    }
    check(scores==1u && ranks==1u && levels && finishes==2u,"both complete timelines retain original mode-dependent visual branches and XP upgrade calls");
    sequence.Close();sequence.Close();tasks.Close();battle.Clear();window.SetVisible(false);
    check(!window.Tasks().DomainCount() && !players.ActiveIterators(),"native sequence domain/iterator ownership fully restored");
    std::cout<<"Native547270 sequence: active/inactive full actual EndTask/GOM/DLL/XP timelines, original UpgradeData operands, packed role rows and optional panels, real victory/rank visuals and zero graph ownership pass; naturally empty stage roster, upgrade/finish endpoints remain protocol fixtures\n";
}
void VerifyNativeCombatEndingSequenceActions(NativeCombatEndingWindow& window,NativeCombatPlayerManager& players,
    NativeMilesSound2dRuntime& sounds,NativeCombatSystemImage& image,const NativeUpgradeRecordCatalog& upgrades,
    NativeCombatEndingTasks::ActorSkills skills,NativeCombatTaskQueue& battle,
    const std::function<NativeCombatResultActions&(std::uint32_t)>& actions,INativeSkillSystemServices& system,
    const std::function<INativeCombatActorTurnServices&(std::uint32_t)>& visuals,
    const std::function<void(float)>& advance,const std::function<void()>& restore_motion){
    const auto check=[](bool v,const char* m){if(!v)throw std::runtime_error(m);};
    check(!battle.Count() && !window.TaskCount() && !window.Tasks().DomainCount() && players.StageCount(),"Ending action-tail fixture requires idle actual battle queue and populated stage");
    struct Restore{NativeCombatEndingWindow& w;NativeCombatTaskQueue& b;INativeSkillSystemServices& system;
        RandomNumberGenerator rng;NativeCombatSystemImage& image;std::uint8_t mode;std::vector<std::pair<std::uint32_t,std::uint32_t>> fields;const std::function<void()>& motion;
        ~Restore(){try{b.Clear();w.Tasks().Clear();for(auto [a,v]:fields)*reinterpret_cast<std::uint32_t*>(a)=v;system.Random()=rng;image.Mode()=mode;motion();}catch(...){}}
    } restore{window,battle,system,system.Random(),image,image.Mode(),{},restore_motion};
    for(std::size_t n=0;n<players.Count();++n){const auto a=players.ActorAt(n);for(unsigned o=340;o<676;o+=4)restore.fields.emplace_back(a+o,*reinterpret_cast<const std::uint32_t*>(a+o));restore.fields.emplace_back(a+828u,*reinterpret_cast<const std::uint32_t*>(a+828u));}
    image.Mode()=0u;NativeCombatEndingTasks tasks(window,players,sounds,image,upgrades,skills);
    NativeCombatEndingSequence sequence(window,tasks,players,battle,image,upgrades,skills,actions,visuals);
    unsigned expected{};for(std::size_t n=0;n<players.StageCount();++n)if(skills(players.StageActorAt(n)).IsNotStateThree())++expected;
    check(expected,"actual stage provides at least one eligible Win actor");
    sequence.Start();check(battle.Count()==expected && window.TaskCount()==1u && !players.ActiveIterators(),"original stage38 tail publishes each real Win action after the frame graph and releases both cursors");
    for(std::size_t n=0;n<players.StageCount();++n){const auto a=players.StageActorAt(n);auto& actor=skills(a);if(!actor.IsNotStateThree())continue;
        check(actions(a).Count()==1u,"one result parent belongs to each eligible actual stage actor");
        for(const unsigned o:{632u,636u,640u,644u,652u,656u,664u})check(!*reinterpret_cast<const std::uint32_t*>(a+o),"589970 clears original modes, preserving health648 and subsequent Win action660");
        check(actor.ActionState()==0u,"Win virtual568 restores model without changing reset action660");
    }
    // This probe isolates the real stage tail; the full UI timeline is verified
    // above using awarded report data. No fake finish callback is installed here.
    window.Tasks().ResetFlags();window.Tasks().Update(0.f);
    check(!window.TaskCount() && !tasks.OwnedAllocations(),"cancelling frame graph preserves independently owned actor results");
    unsigned frames{};while(battle.Count() && frames++<1600u)advance(1.f/60.f);
    check(!battle.Count(),"actual547270 Win parents complete through real formation/model/AMF battle updates");
    for(std::size_t n=0;n<players.StageCount();++n){auto& owner=actions(players.StageActorAt(n));check(!owner.Count() && !owner.OwnedAllocations() && !owner.OwnedTexts(),"stage result parents/children/names retire after native sequence tail");}
    sequence.Close();tasks.Close();check(!players.ActiveIterators() && !window.Tasks().DomainCount(),"action-tail sequence restores roster and frame domain ownership");
    std::cout<<"Native547270 stage tail: actual first/stage rosters,589970 property resets, production battle queue ResetFlags, "<<expected<<" real Win parents and model/AMF completion, full scoped retirement pass; frame timeline cancelled after inspection\n";
}
}
