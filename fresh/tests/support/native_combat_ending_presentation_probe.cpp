#include "native_combat_ending_presentation_probe.h"
#include "pal4/native_combat_ending_window.h"
#include "pal4/native_combat_report.h"
#include "pal4/native_combat_player_manager.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_game_database.h"
#include "pal4/native_cegui_text_conversion.h"
#include "../../native/ui/cegui_exports_native.h"
#include <array>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
namespace {
template<class... Args> std::string Format(std::string_view f,Args... args){const std::string fmt(f);const auto n=std::snprintf(nullptr,0u,fmt.c_str(),args...);
    if(n<0)throw std::runtime_error("ending expected format failed");std::vector<char> b(static_cast<std::size_t>(n)+1u);std::snprintf(b.data(),b.size(),fmt.c_str(),args...);return {b.data(),static_cast<std::size_t>(n)};}
}
void VerifyNativeCombatEndingPresentation(NativeCombatEndingWindow& window,const NativeCombatReport& source,
    NativeCombatPlayerManager& players,NativeInventorySession& inventory,const NativeGameDatabase& db,NativeCombatEndingPresentation::ActorSkills skills){
    const auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
    const auto reject=[&](auto f){bool b{};try{f();}catch(const std::logic_error&){b=true;}check(b,"ending presentation callback rejects lifecycle/reentry");};
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto cstr=detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ");
    const auto get_text=[&](unsigned offset){return std::string(cstr(text(reinterpret_cast<void*>(window.Word(offset)))));};
    const auto height=detail::Export<float (__thiscall*)(const void*,unsigned)>(module,"?getHeight@Window@CEGUI@@QBEMW4MetricsMode@2@@Z");
    const auto width=detail::Export<float (__thiscall*)(const void*)>(module,"?getWidth@Window@CEGUI@@QBEMXZ");
    const auto y=detail::Export<float (__thiscall*)(const void*)>(module,"?getYPosition@Window@CEGUI@@QBEMXZ");
    const auto image=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getImage@StaticImage@CEGUI@@QBEPBVImage@2@XZ");
    const auto utf8=[&](std::string_view s){const auto r=ConvertGameTextToUtf8(s);check(bool(r),"ending expected game text conversion");return *r;};
    const auto game_string=[&](unsigned id){const auto v=db.FindTableTextById("GameString",id);check(bool(v),"ending expected original GameString");return std::string(*v);};
    NativeCombatEndingPresentation presentation(window,players,inventory,db,skills);
    bool guarded{};try{presentation.Show(source);}catch(const std::logic_error& e){guarded=std::string_view(e.what())=="Combat ending task sequence services are not bound";}
    check(guarded && presentation.PreparedReports()==1u && !presentation.ActiveCursors() && !players.ActiveIterators(),"actual presentation completes before sequence guard and drains first-roster cursor");
    const auto rank=source.Word(8u);
    check(get_text(68u)==utf8(game_string(0xc3u+rank)) && image(reinterpret_cast<void*>(window.Word(64u)))==reinterpret_cast<void*>(window.Word(664u+4u*rank)),"actual DLL evaluation text/image for all five original ranks");
    check(get_text(92u)==utf8(Format(game_string(0xc2u),std::bit_cast<std::int32_t>(source.Word(0u)))),"actual DLL money uses original format/full signed scalar");
    for(unsigned i=0;i<5u;++i)check(window.Word(420u+4u*i)==source.Word(4u*i),"ending original report scalar copy");
    check(window.Report().Address()!=source.Address() && window.Report().Items()==source.Items() && window.Report().Items(true)==source.Items(true),"ending report owns independent dropped and bonus containers");
    for(unsigned i=0;i<4u;++i){check(std::vector<std::uint32_t>(window.Report().Learned(i).begin(),window.Report().Learned(i).end())==std::vector<std::uint32_t>(source.Learned(i).begin(),source.Learned(i).end()),"ending learned stable IDs deep copied");
        if(!source.Learned(i).empty())check(window.Report().Word(56u+40u*i)!=source.Word(56u+40u*i),"ending learned vectors have distinct ownership");}
    for(unsigned row=0;row<players.Count();++row){const auto a=players.ActorAt(row);auto& actor=skills(a);const auto slot=*reinterpret_cast<const std::uint32_t*>(a+1192u),base=116u+76u*row;
        check(get_text(base+4u)==std::to_string(source.Word(52u+40u*slot+20u)) && get_text(base+20u)=="--" && get_text(base+24u)=="--","actual role row starts with previous level and XP placeholders");
        const auto hp=actor.CurrentStat(7u),mp=actor.CurrentStat(9u),rage=actor.CurrentStat(8u),max_hp=actor.StatParameter(0u),max_mp=actor.StatParameter(2u);
        check(get_text(base+8u)==Format("%d/%d",hp,max_hp) && get_text(base+12u)==Format("%d/%d",mp,max_mp) && get_text(base+16u)==Format("%d/%d",rage,100),"actual DLL full-width HP/MP/rage and recovered format denominators");
        const std::array<float,3> expected{float(double(hp)*std::bit_cast<float>(window.Word(base+48u))/max_hp),float(double(mp)*std::bit_cast<float>(window.Word(base+52u))/max_mp),float(double(rage)*std::bit_cast<float>(window.Word(base+56u))*.0099999998)};
        for(unsigned n=0;n<3u;++n)check(std::fabs(width(reinterpret_cast<void*>(window.Word(base+28u+4u*n)))-expected[n])<.0001f,"actual DLL original bar ratios");
        check(y(reinterpret_cast<void*>(window.Word(base)))<0.f && window.Word(base+72u)==source.Word(52u+40u*slot+20u),"actual role row stages above screen and retains prior level");
    }
    const auto name=[&](unsigned id)->std::optional<std::string>{for(const auto pair:{std::pair{"Property","PA_NAME"},std::pair{"Equip","EA_NAME"}}){const auto& table=db.Tables().at(pair.first);
        const auto id_it=std::find_if(table.fields.begin(),table.fields.end(),[](const auto& f){return f.name=="ID";});
        const auto name_it=std::find_if(table.fields.begin(),table.fields.end(),[&](const auto& f){return f.name==pair.second;});check(id_it!=table.fields.end() && name_it!=table.fields.end(),"ending expected source named DB columns");
        for(const auto& r:table.rows)if(std::get<std::uint32_t>(r[id_it-table.fields.begin()])==id)return std::get<std::string>(r[name_it-table.fields.begin()]);}return std::nullopt;};
    for(unsigned bonus=0;bonus<2u;++bonus){const auto entries=source.Items(bonus!=0u);if(entries.empty())continue;std::array<std::string,2> columns;unsigned n{};
        for(auto [id,count]:entries)if(auto text=name(id))columns[n++%2u]+=Format(game_string(0xffu),text->c_str(),std::bit_cast<std::int32_t>(count));
        const auto left=bonus?104u:96u,right=bonus?108u:100u;
        check(get_text(left)==utf8(columns[0]) && get_text(right)==utf8(columns[1]),"actual DLL columns preserve signed map order, original item/equipment names/counts and alternating columns");
        if(bonus)check(get_text(112u)==utf8(game_string(0x101u)),"actual bonus title");
    }
    const std::array before{height(reinterpret_cast<void*>(window.Word(76u)),1u),height(reinterpret_cast<void*>(window.Word(88u)),1u)};
    presentation.BindSequenceStarter([&]{reject([&]{window.Close();});reject([&]{window.BindPrizeHandler({});});reject([&]{presentation.Close();});reject([&]{presentation.Show(source);});
        check(players.ActiveIterators()==1u,"ending sequence starter borrows original first-roster cursor until return");throw std::runtime_error("controlled ending sequence failure");});
    bool failed{};try{presentation.Show(source);}catch(const std::runtime_error& e){failed=std::string_view(e.what())=="controlled ending sequence failure";}
    check(failed && presentation.PreparedReports()==2u && !presentation.ActiveCursors() && !players.ActiveIterators(),"ending sequence exception releases report borrow/frame lease/cursor and supports retry");
    check(std::fabs(height(reinterpret_cast<void*>(window.Word(76u)),1u)-before[0])<.01f && std::fabs(height(reinterpret_cast<void*>(window.Word(88u)),1u)-before[1])<.01f,"repeated actual text reflow preserves panel padding without cumulative height drift");
    presentation.Close();presentation.Close();
    std::cout<<"Native ending presentation rank="<<rank<<": actual545AE0 report/roles/grade/money,546770/546D40 original item/equipment columns and DLL reflow, deep stable-ID ownership, raw stat/denominator, frame/Player cursor guards and exception retry pass\n";
}
}
