#include "native_combat_ending_levelup_probe.h"
#include "pal4/native_combat_ending_level_up.h"
#include "pal4/native_combat_ending_window.h"
#include "pal4/native_combat_report.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_task_queue.h"
#include "pal4/native_combat_hints.h"
#include "pal4/native_miles_sound_2d_runtime.h"
#include "pal4/native_game_timer_queue.h"
#include "pal4/native_game_database.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_cegui_text_conversion.h"
#include "../../native/ui/cegui_exports_native.h"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <map>
#include <set>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace pal4::test {
namespace {
template<class... T>std::string Format(std::string_view value,T... args){const std::string f(value);const auto n=std::snprintf(nullptr,0,f.c_str(),args...);if(n<0)throw std::runtime_error("upgrade fixture format");std::vector<char>b(n+1u);std::snprintf(b.data(),b.size(),f.c_str(),args...);return {b.data(),static_cast<std::size_t>(n)};}
}
void VerifyNativeCombatEndingLevelUp(NativeCombatEndingLevelUp& owner,NativeCombatEndingWindow& frame,NativeCombatActorSkillRuntime& actor,
    NativeCombatTaskQueue& queue,NativeCombatHints& hints,NativeMilesSound2dRuntime& sounds,NativeGameTimerQueue& timers,
    const NativeGameDatabase& db,NativeEffectSceneSession& effects,const std::function<void(float)>& advance,const std::function<void()>& restore_motion){
    const auto check=[](bool b,const char* s){if(!b)throw std::runtime_error(s);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    const auto a=actor.ActorAddress(),slot=word(a+1192u),row=frame.Address()+116u,hint=frame.Word(684u),record=frame.Address()+472u+40u*slot;
    check(slot==0u && !queue.Count() && !frame.Report().Learned(slot).empty(),"upgrade fixture uses actual first Player and retained original-data learned IDs");
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");const auto text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto cstr=detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ");
    const auto level_control=frame.Word(120u);const std::string prior_text(cstr(text(reinterpret_cast<void*>(level_control))));
    struct Restore{NativeCombatTaskQueue& q;NativeCombatEndingWindow& f;NativeMilesSound2dRuntime& s;std::set<std::uint32_t> samples;
        std::vector<std::pair<std::uint32_t,std::uint32_t>> fields;std::uint32_t row,level,control;HMODULE module;std::string text;const std::function<void()>& motion;
        ~Restore(){try{q.Clear();f.SetVisible(false);for(auto h:samples)(void)s.StopSample(h);for(auto [p,v]:fields)*reinterpret_cast<std::uint32_t*>(p)=v;motion();
            auto lease=f.BorrowPresentation();f.WritePresentationWord(row-f.Address()+72u,level);detail::NativeString old(module,text,detail::NativeStringEncoding::Utf8);
            detail::Export<void (__thiscall*)(void*,const void*)>(module,"?setText@Window@CEGUI@@QAEXABVString@2@@Z")(reinterpret_cast<void*>(control),old.Data());}catch(...){}}
    } restore{queue,frame,sounds,{}, {},row,frame.Word(188u),level_control,module,prior_text,restore_motion};
    for(unsigned o=340u;o<676u;o+=4u)restore.fields.emplace_back(a+o,word(a+o));restore.fields.emplace_back(a+828u,word(a+828u));
    const auto& table=db.Tables().at("Stunt");const auto column=[&](std::string_view name){const auto i=std::find_if(table.fields.begin(),table.fields.end(),[&](const auto& f){return f.name==name;});check(i!=table.fields.end(),"original Stunt expected column");return static_cast<std::size_t>(i-table.fields.begin());};
    const auto key=column("ID"),name=column("NAME"),requirement=column("ePT_TIANHE_LV_LMT");std::map<std::int32_t,std::vector<std::string>> learned;
    for(auto id:frame.Report().Learned(slot)){const auto r=std::find_if(table.rows.begin(),table.rows.end(),[&](const auto& row){return std::get<std::uint32_t>(row[key])==id;});check(r!=table.rows.end(),"retained stable ID resolves in original Stunt table");
        learned[std::bit_cast<std::int32_t>(std::get<std::uint32_t>((*r)[requirement]))].push_back(std::get<std::string>((*r)[name]));}
    auto found=std::find_if(learned.begin(),learned.end(),[](const auto& pair){return pair.first>1 && pair.first<=99;});check(found!=learned.end(),"retained reward fixture contains a real learned level above one");
    auto unmatched=99;while(learned.contains(unmatched) && unmatched>1)--unmatched;check(!learned.contains(unmatched),"original learned set supplies a no-learning notification level");
    const std::string actor_name(reinterpret_cast<const char*>(word(a+1116u)));const auto base=db.FindTableTextById("GameString",202u),extra=db.FindTableTextById("GameString",184u);check(base && extra,"upgrade original GameString formats");
    unsigned branch{};for(const auto level:{found->first,unmatched}){
        {auto lease=frame.BorrowPresentation();frame.WritePresentationWord(188u,level-1u);}
        auto expected=Format(*base,actor_name.c_str(),level);if(const auto i=learned.find(level);i!=learned.end())for(const auto& n:i->second)expected+="<br/>"+Format(*extra,n.c_str());
        const auto before=effects.CombatActiveCount();owner.Apply(row,level,a,hint,record);const auto samples=owner.LastSamples();for(auto h:samples)if(h)restore.samples.insert(h);
        check(frame.Word(188u)==static_cast<std::uint32_t>(level) && std::string(cstr(text(reinterpret_cast<void*>(level_control))))==std::to_string(level),"native upgrade writes actual level row/control");
        check(std::string(cstr(text(reinterpret_cast<void*>(word(hint+40u)))))==*ConvertGameTextToUtf8(expected) && word(hints.LevelUpRoot(0u)+68u),"native learned IDs produce exact original role/Stunt rich hint text");
        check(samples[0] && sounds.IsPlaying(samples[0]) && (branch==0u?(samples[1] && samples[1]!=samples[0] && sounds.IsPlaying(samples[1])):!samples[1]),"original independent WA18 and conditional WA17 samples play through real Miles");
        check(effects.CombatActiveCount()==before+1u && word(a+828u)==7u && queue.Count()==1u,"actual EFF17 hook attachment and real state7 LevelUp task registration");
        const auto count=sounds.ActiveCount();owner.Apply(row,level,a,hint,record);check(sounds.ActiveCount()==count && effects.CombatActiveCount()==before+1u && queue.Count()==1u,"same displayed level is a true no-op");++branch;
    }
    bool rejected{};try{owner.Apply(row,5,a,hint+4u,record);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"foreign hint rejected before native upgrade side effects");
    unsigned frames{};while(queue.Count() && frames<1200u){timers.Update(1.f/60.f);advance(1.f/60.f);sounds.Update();++frames;}
    check(!queue.Count(),"native notification's real LevelUp/AMF task finishes");frame.SetVisible(false);
    std::cout<<"Native Ending upgrade consumer: retained learned IDs -> original Stunt role levels/names, exact GameString/DLL hint and row, independent WA18/WA17, real EFF17/hook_ef_pos and one coalesced z16 task, no-op/identity guards and retirement pass; controlled displayed levels do not award Player XP\n";
}
}
