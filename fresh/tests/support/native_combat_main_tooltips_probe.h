#pragma once
#include "pal4/native_combat_main_tooltips.h"
#include "pal4/native_combat_main_window.h"
#include "pal4/native_combat_tooltip_data.h"
#include "pal4/native_combat_actor_owner.h"
#include "pal4/native_combat_actor_skill_runtime.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_cegui_text_conversion.h"
#include "../../native/ui/cegui_exports_native.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatMainTooltips(NativeCombatMainTooltips& tips,NativeCombatMainWindow& main,
    NativeCombatActorOwner& monster,NativeCombatActorSkillRuntime& monster_skills,
    NativeCombatActorSkillRuntime& player,NativeCombatSystemImage& system){
    const auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
    const auto word=[](std::uint32_t a){return *reinterpret_cast<const std::uint32_t*>(a);};
    auto& state=system.State();struct Restore{std::uint32_t& state;std::uint32_t old;~Restore(){state=old;}} restore{state,state};
    const auto module=GetModuleHandleW(L"CEGUIBase.dll");
    const auto get_text=detail::Export<const void* (__thiscall*)(const void*)>(module,"?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto cstr=detail::Export<const char* (__thiscall*)(const void*)>(module,"?c_str@String@CEGUI@@QBEPBDXZ");
    const auto visible=detail::Export<bool (__thiscall*)(const void*,bool)>(module,"?isVisible@Window@CEGUI@@QBE_N_N@Z");
    const auto parent=detail::Export<void* (__thiscall*)(const void*)>(GetModuleHandleW(L"CEGUIBase.dll"),"?getParent@Window@CEGUI@@QBEPAV12@XZ");
    const auto root=reinterpret_cast<void*>(main.Root());
    const auto root_visible=visible(root,true);const auto root_parent=parent(root);
    const auto width=detail::Export<float (__thiscall*)(const void*,std::uint32_t)>(module,"?getWidth@Window@CEGUI@@QBEMW4MetricsMode@2@@Z");
    const auto height=detail::Export<float (__thiscall*)(const void*,std::uint32_t)>(module,"?getHeight@Window@CEGUI@@QBEMW4MetricsMode@2@@Z");
    const auto panel=word(main.Address()+260u),edit=word(main.Address()+264u);
    const auto text=[&]{return std::string(cstr(get_text(reinterpret_cast<void*>(edit))));};
    std::uint32_t facet=monster.Address()+716u;state=4u;
    const auto before=text();check(!tips.Handle(reinterpret_cast<std::uint32_t>(&facet)) && text()==before,"Monster tips state gate preserves text");
    state=5u;check(tips.Handle(reinterpret_cast<std::uint32_t>(&facet)),"actual Monster tips display");
    std::string expected(monster.TooltipText());expected+="<br/>";
    for(const auto permanent:{true,false})for(auto id:monster_skills.StatusIconIds(permanent))
        if(id>0 && static_cast<std::size_t>(id)<main.StatusMarkupCount())expected+=main.StatusMarkup(id);
    const auto utf8=ConvertGameTextToUtf8(expected);check(utf8 && text()==*utf8,"actual Monster text/status order and game encoding");
    const auto w=width(reinterpret_cast<void*>(panel),1u),h=height(reinterpret_cast<void*>(panel),1u);
    check(std::isfinite(w) && std::isfinite(h) && w>5.f && h>5.f && visible(reinterpret_cast<void*>(panel),true),"actual DLL tooltip measures and shows panel");
    check(tips.Handle(reinterpret_cast<std::uint32_t>(&facet)) && std::abs(width(reinterpret_cast<void*>(panel),1u)-w)<0.01f && std::abs(height(reinterpret_cast<void*>(panel),1u)-h)<0.01f,"repeated layout has no size drift");
    NativeCombatTooltipData data;data.actor=monster.Address();data.system=reinterpret_cast<std::uint32_t>(&system);data.monster=true;
    data.text=monster.TooltipText();data.permanent={-1,0,1,29,28};data.timed={1};
    check(main.DisplayTooltip(data,5u,{100.f,100.f}),"explicit status-boundary snapshot displays");
    const auto boundary=ConvertGameTextToUtf8(data.text+"<br/>"+std::string(main.StatusMarkup(1u))+std::string(main.StatusMarkup(28u))+std::string(main.StatusMarkup(1u)));
    check(boundary && text()==*boundary,"native host status bounds and permanent-before-timed order");
    facet=player.ActorAddress()+716u;state=3u;check(!tips.Handle(reinterpret_cast<std::uint32_t>(&facet)),"Player tips gated before projection");
    state=5u;check(tips.Handle(reinterpret_cast<std::uint32_t>(&facet)),"actual Player tooltip projection and stats");
    const auto hp=std::to_string(player.CurrentStat(7u))+"/"+std::to_string(player.StatParameter(0u));
    check(text().find(hp)!=std::string::npos && text().find("dialog_simsun")!=std::string::npos,"actual Player HP and original font markup");
    bool failed{};try{tips.Handle(0u);}catch(const std::invalid_argument&){failed=true;}check(failed,"invalid tooltip record rejected");
    facet=0x1000u;failed=false;try{tips.Handle(reinterpret_cast<std::uint32_t>(&facet));}catch(const std::invalid_argument&){failed=true;}check(failed,"foreign tooltip actor rejected");
    main.HandlePanelEvent(0x54da00u);check(!visible(reinterpret_cast<void*>(panel),true) && visible(root,true)==root_visible && parent(root)==root_parent,"tooltip cleanup hides its panel and preserves Main visibility/attachment");
    std::cout<<"Native combat tooltips: actual Monster text/statuses, Player HP/projection, DLL layout/encoding, state gates, host markup bounds and hidden cleanup pass\n";
}
}
