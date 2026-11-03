#include "native_combat_ending_tasks_probe.h"
#include "pal4/native_combat_ending_window.h"
#include "pal4/native_combat_console_visuals.h"
#include "pal4/native_combat_console.h"
#include "pal4/ui_text_image_animation.h"
#include "pal4/native_combat_report.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_miles_sound_2d_runtime.h"
#include "pal4/native_upgrade_record_catalog.h"
#include "pal4/gom_task_base.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <set>
#include "pal4/native_combat_system_image.h"
namespace pal4::test {
void VerifyNativeCombatEndingTasks(NativeCombatEndingWindow& window,NativeCombatPlayerManager& players,
    NativeMilesSound2dRuntime& sounds,NativeCombatSystemImage& image,const NativeUpgradeRecordCatalog& upgrades,NativeCombatEndingTasks::ActorSkills skills,NativeCombatConsoleVisuals& visuals,NativeCombatConsole& console){
    const auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
    const auto reject=[&](auto f){bool b{};try{f();}catch(const std::logic_error&){b=true;}check(b,"Ending task lifecycle guard");};
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto visible=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z");
    const auto ypos=detail::Export<float (__thiscall*)(const void*)>(module,"?getYPosition@Window@CEGUI@@QBEMXZ");
    const auto get_text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto cstr=detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ");
    const auto atlas_image=[&](const char* name){
        detail::NativeString set(module,"zi0");
        const auto manager=detail::Export<void* (__cdecl*)()>(module,"?getSingletonPtr@ImagesetManager@CEGUI@@SAPAV12@XZ")();
        const auto atlas=detail::Export<void* (__thiscall*)(const void*,const void*)>(module,"?getImageset@ImagesetManager@CEGUI@@QBEPAVImageset@2@ABVString@2@@Z")(manager,set.Data());
        return reinterpret_cast<std::uint32_t>(detail::Export<const void* (__thiscall*)(const void*,const char*)>(module,"?getImage@Imageset@CEGUI@@QBEABVImage@2@PBD@Z")(atlas,name));
    };
    const auto baseline_visuals=visuals.OutstandingAllocations();
    const auto candidate=visuals.Allocate(68u);(void)visuals.ConstructFontElement(candidate);
    reject([&]{(void)visuals.Allocate(68u);});
    reject([&]{visuals.AddElement(0u,true,candidate);});
    visuals.DiscardPending();check(visuals.OutstandingAllocations()==baseline_visuals,"unpublished console visual cleanup retains shared binding");
    const auto samples_before=sounds.ActiveCount();
    auto& queue=window.Tasks();const auto initial_handles=sounds.SampleHandles();const std::set<std::uint32_t> initial_samples(initial_handles.begin(),initial_handles.end());std::set<std::uint32_t> probe_samples;
    struct ActiveMode{std::uint8_t& mode;std::uint8_t old;~ActiveMode(){mode=old;}} active_mode{image.Mode(),image.Mode()};image.Mode()=0u;
    std::uint32_t rank_visual{};float rank_age{};bool rank_clamped{};
    const auto update=[&](float delta){
        queue.Update(delta);
        for(auto h:sounds.SampleHandles())if(!initial_samples.contains(h))probe_samples.insert(h);
        console.Update(delta);
        if(rank_visual && !rank_clamped){rank_age+=delta;if(rank_age>.5f){
            check(*reinterpret_cast<const float*>(rank_visual+36u)==1.f && *reinterpret_cast<const float*>(rank_visual+40u)==1.f,
                "actual rank update shrinks both axes to the original unit clamp");rank_clamped=true;}}
        sounds.Update(); // The real game frame reaps completed samples too.
    };
    struct AudioCleanup{NativeMilesSound2dRuntime& sounds;const std::set<std::uint32_t>& initial;~AudioCleanup(){try{for(auto h:sounds.SampleHandles())if(!initial.contains(h))(void)sounds.StopSample(h);}catch(...){}}} audio_cleanup{sounds,initial_samples};
    const auto a=players.ActorAt(0u);auto& actor=skills(a);
    const auto level=actor.CurrentStat(4u),xp=actor.CurrentStat(5u),id=actor.AiTargetId();
    std::int32_t required{};for(std::size_t n=0;n<upgrades.Count();++n){const auto& r=upgrades.Record(n);if(r[37u]==static_cast<std::uint32_t>(id) && r[38u]==static_cast<std::uint32_t>(level)){required=std::bit_cast<std::int32_t>(r[39u]);break;}}
    const auto minimum=required-xp;check(level>1 && level<99 && xp>0,"Ending task probe real leveled Player/report operands");
    NativeCombatEndingTasks* owner{};std::uint32_t parent{};bool fail_score=true;unsigned scores{},ranks{},finishes{},level_ups{};
    NativeCombatEndingTaskServices services;
    services.score_text=[&]{++scores;reject([&]{owner->Close();});reject([&]{owner->Cancel(parent);});reject([&]{window.Close();});reject([&]{update(0.f);});
        if(fail_score)throw std::runtime_error("controlled Ending score callback");
        visuals.ShowVictory();
        const auto head=*reinterpret_cast<const std::uint32_t*>(console.Address()+4u);
        const auto node=*reinterpret_cast<const std::uint32_t*>(head+4u);
        const auto visual=*reinterpret_cast<const std::uint32_t*>(node+12u);
        const auto w=[&](unsigned o){return *reinterpret_cast<const std::uint32_t*>(visual+o);};
        check(w(0)==UiTextImageAnimation::kVtable && w(24)==atlas_image("zhangdoushengli") && std::bit_cast<float>(w(28))==400.f && std::bit_cast<float>(w(32))==280.f &&
            std::bit_cast<float>(w(56))==2.f && std::bit_cast<float>(w(60))==3.f,"actual original victory atlas/position/timing");};
    services.rank_icon=[&](std::uint32_t child){++ranks;reject([&]{owner->Close();});
        const auto before=visuals.OutstandingAllocations();visuals.ShowRank(child,window);
        const auto rank=window.Word(428u);check(visuals.OutstandingAllocations()==before+(rank<4u?1u:0u),"original four rank images and fifth-rank sound-only branch");
        if(rank<4u){const auto head=*reinterpret_cast<const std::uint32_t*>(console.Address()+4u);
            const auto node=*reinterpret_cast<const std::uint32_t*>(head+4u);const auto visual=*reinterpret_cast<const std::uint32_t*>(node+12u);
            rank_visual=visual;
            const auto w=[&](unsigned o){return *reinterpret_cast<const std::uint32_t*>(visual+o);};
            check(w(0)==CombatResultIconCreator::kVtable && w(24)==atlas_image(std::array{"tedeng2","yideng2","erdeng2","sandeng2"}[rank]) && std::bit_cast<float>(w(28))==450.f && std::bit_cast<float>(w(32))==430.f &&
                std::bit_cast<float>(w(40))==20.f && std::bit_cast<float>(w(68))==60.f,"actual rank atlas, original shrink vtable and rate");}
    };
    services.finish=[&](std::uint32_t result){check(result==2u,"Ending exact end-task payload");reject([&]{owner->Close();});++finishes;};
    services.level_up=[&](std::uint32_t row,std::int32_t to,std::uint32_t who,std::uint32_t hint,std::uint32_t record){
        check(row==window.Address()+116u && to==level && who==a && hint==window.Word(684u) && record==window.Address()+472u,
            "Ending XP child forwards original row/level/actor/hint/report identity");reject([&]{window.Close();});++level_ups;};
    NativeCombatEndingTasks tasks(window,players,sounds,image,upgrades,skills,services);owner=&tasks;parent=tasks.CreateSequence();
    constexpr std::array types{CombatEndingTaskType::ScoreText,CombatEndingTaskType::LevelText,CombatEndingTaskType::Window,CombatEndingTaskType::MoveDownWindow,
        CombatEndingTaskType::UpdateExperience,CombatEndingTaskType::ShowStep,CombatEndingTaskType::PrizeProperty,CombatEndingTaskType::CloseSystem};
    constexpr std::array vtables{0x8453ccu,0x8453e8u,0x845404u,0x845420u,0x845440u,0x84545cu,0x845478u,0x845494u};
    constexpr std::array delays{0.f,.1f,.2f,.25f,.25f,.4f,.5f,3.f};std::array<std::uint32_t,8> children{};
    const auto configure_xp=[&](std::uint32_t child,std::int32_t start_level,float remaining){
        tasks.ConfigureWord(child,60u,window.Address()+116u);tasks.ConfigureWord(child,64u,window.Address()+420u);tasks.ConfigureWord(child,68u,a);
        tasks.ConfigureWord(child,72u,std::bit_cast<std::uint32_t>(float(std::bit_cast<std::int32_t>(window.Report().Word(52u+28u)))));
        tasks.ConfigureWord(child,76u,std::bit_cast<std::uint32_t>(float(xp)*.5f));tasks.ConfigureWord(child,80u,static_cast<std::uint32_t>(start_level));
        tasks.ConfigureWord(child,84u,std::bit_cast<std::uint32_t>(remaining));tasks.ConfigureWord(child,88u,std::bit_cast<std::uint32_t>(minimum));
        tasks.ConfigureWord(child,92u,window.Address()+700u);tasks.ConfigureWord(child,96u,window.Word(684u));
    };
    for(unsigned n=0;n<types.size();++n){children[n]=tasks.CreateChild(types[n]);check(tasks.Word(children[n],0u)==vtables[n],"all eight actual shared factory vtables");
        tasks.Add(parent,children[n],"OriginalEndingChild",delays[n]);
        check(tasks.Word(children[n],12u)==2u && std::bit_cast<float>(tasks.Word(children[n],16u))==delays[n] && tasks.Word(children[n],56u)==parent+4u,
            "actual GOM absolute wait mode, child delay and original container context");
        reject([&]{tasks.DestroyChild(children[n]);});
    }
    tasks.ConfigureWord(children[0],60u,window.Address());tasks.ConfigureWord(children[1],60u,window.Address());
    tasks.ConfigureWord(children[2],60u,window.Word(72u));tasks.ConfigureByte(children[2],64u,1u);tasks.ConfigureByte(children[2],65u,1u);
    tasks.ConfigureWord(children[3],60u,window.Word(116u));tasks.ConfigureWord(children[3],64u,window.Word(176u));configure_xp(children[4],level,float(minimum+xp));
    tasks.ConfigureWord(children[5],60u,window.Address());tasks.ConfigureWord(children[5],64u,2u);tasks.ConfigureWord(children[6],60u,window.Address());tasks.ConfigureWord(children[7],60u,2u);
    check(queue.Register(parent)==parent && tasks.Count()==1u && tasks.ChildCount()==8u && queue.Count()==1u,"actual Ending frame queue adopts original EndTask/GOM graph");
    bool threw{};try{update(.01f);}catch(const std::runtime_error& e){threw=std::string_view(e.what())=="controlled Ending score callback";}
    check(threw && scores==1u && !ranks && tasks.Word(children[0],40u)==2u && queue.Count()==1u,"throwing BeginRun keeps GOM wait phase for retry");
    fail_score=false;update(0.f);check(scores==2u && !ranks,"retry enters exactly the pending original child");
    reject([&]{tasks.ConfigureWord(children[0],60u,0u);});
    unsigned frames{};while(queue.Count() && frames<300u){update(.016f);++frames;}
    check(!queue.Count() && !tasks.Count() && !tasks.ChildCount() && !tasks.OwnedAllocations() && ranks==1u && finishes==1u && !level_ups,
        "original GOM phases complete and retire all eight tasks/names/links/parent through real frame queue");
    check(std::fabs(ypos(reinterpret_cast<void*>(window.Word(116u)))-std::bit_cast<float>(window.Word(176u)))<.0001f && window.Word(716u)==2u &&
        visible(reinterpret_cast<void*>(window.Word(72u)),false) && visible(reinterpret_cast<void*>(window.Word(88u)),false),"actual DLL role entry clamp, panel visibility and state task");
    const auto shown=std::string(cstr(get_text(reinterpret_cast<void*>(window.Word(136u))))),remaining=std::string(cstr(get_text(reinterpret_cast<void*>(window.Word(140u)))));
    std::cout<<"Ending XP diagnostic: shown="<<shown<<" expected="<<(xp-std::bit_cast<std::int32_t>(window.Report().Word(52u+28u)))<<" remaining="<<remaining<<" expected="<<minimum<<" handle="<<window.Word(700u)<<" playing="<<sounds.IsPlaying(window.Word(700u))<<" samples="<<sounds.ActiveCount()<<" error="<<sounds.LastError()<<"\n";
    check(shown==std::to_string(xp-std::bit_cast<std::int32_t>(window.Report().Word(52u+28u))) && remaining==std::to_string(minimum) && !sounds.IsPlaying(window.Word(700u)) && window.Word(700u),
        "real XP text/remaining cap and original finish stops stored sample without zeroing handle");
    check(window.Word(428u)>=4u || rank_clamped,"rank shrink callback exercised by real queue updates");
    console.Update(.5f);console.Update(.01f);
    check(!visuals.OutstandingAllocations(),"victory and original rank shrink animations retire through shared console allocator");
    parent=tasks.CreateSequence();auto child=tasks.CreateChild(CombatEndingTaskType::UpdateExperience);tasks.Add(parent,child,"CrossLevel",0.f);configure_xp(child,level-1,1.f);
    (void)queue.Register(parent);update(.001f);update(.001f);check(level_ups==1u,"actual XP threshold invokes level-up boundary once");queue.Clear();
    const auto was_visible=visible(reinterpret_cast<void*>(window.Word(72u)),false);parent=tasks.CreateSequence();child=tasks.CreateChild(CombatEndingTaskType::Window);
    tasks.Add(parent,child,"DelayedCancelled",10.f);tasks.ConfigureWord(child,60u,window.Word(72u));tasks.ConfigureByte(child,64u,was_visible?0u:1u);
    (void)queue.Register(parent);queue.ResetFlags();update(0.f);check(!queue.Count() && !tasks.OwnedAllocations() && visible(reinterpret_cast<void*>(window.Word(72u)),false)==was_visible,
        "same queue ResetFlags cancels delayed graph without executing disabled child");
    {
        const auto before=sounds.SampleHandles();const std::set<std::uint32_t> existing(before.begin(),before.end());
        struct Mode{std::uint8_t& value;std::uint8_t old;~Mode(){value=old;}} mode{image.Mode(),image.Mode()};image.Mode()=1u;
        parent=tasks.CreateSequence();child=tasks.CreateChild(CombatEndingTaskType::Window);tasks.Add(parent,child,"InactiveWindowSound",0.f);
        tasks.ConfigureWord(child,60u,window.Word(72u));tasks.ConfigureByte(child,64u,1u);tasks.ConfigureByte(child,65u,1u);
        child=tasks.CreateChild(CombatEndingTaskType::UpdateExperience);tasks.Add(parent,child,"InactiveXpSound",0.f);configure_xp(child,level,float(minimum+xp));
        (void)queue.Register(parent);unsigned steps{};while(queue.Count() && ++steps<300u)update(.016f);
        check(!queue.Count() && !tasks.OwnedAllocations() && !window.Word(700u),"inactive Combat still runs UI/XP but suppresses original temporary and looping sound handles");
        for(auto h:sounds.SampleHandles())check(existing.contains(h),"inactive Combat creates no original Miles sample");
    }
    tasks.Close();tasks.Close();check(!queue.DomainCount(),"Ending graph domain detaches only after full ownership retirement");
    {NativeCombatEndingTasks missing(window,players,sounds,image,upgrades,skills);parent=missing.CreateSequence();child=missing.CreateChild(CombatEndingTaskType::ScoreText);
        missing.Add(parent,child,"MissingScore",0.f);missing.ConfigureWord(child,60u,window.Address());(void)queue.Register(parent);bool rejected{};
        try{update(.01f);}catch(const std::logic_error& e){rejected=std::string_view(e.what())=="Ending victory-banner service is not bound";}
        check(rejected,"unbound real visual service fails instead of completing a fake banner");queue.Clear();check(!missing.OwnedAllocations(),"missing service graph can be fully cancelled");missing.Close();}
    window.SetVisible(false);check(!queue.Count() && !queue.DomainCount() && !players.ActiveIterators(),"Ending task fixture restores actual frame activity/audio and all queue ownership");
    for(auto h:probe_samples)(void)sounds.StopSample(h);probe_samples.clear();sounds.Update();
    check(sounds.ActiveCount()<=samples_before,"Ending diagnostic returns its own sample handles without accumulating real Miles channels");
    std::cout<<"Ending task audio ownership: samples="<<samples_before<<"->"<<sounds.ActiveCount()<<" stored="<<window.Word(700u)<<" playing="<<sounds.IsPlaying(window.Word(700u))<<"\n";
    std::cout<<"Native Ending tasks:8 original factories, actual EndTask/ScriptTaskQueue/GOM timing and DLL move/panel/state/XP, original stored-sound stop, level-up boundary, BeginRun exception retry, delayed cancellation and zero ownership pass; actual victory/rank atlas animation and original independent/inactive Combat sound; level-up/finish callback fixtures do not establish full ending sequence\n";
}
}
