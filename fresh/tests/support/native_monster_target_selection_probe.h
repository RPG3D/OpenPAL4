#pragma once
#include "pal4/native_combat_monster_target_selection.h"
#include "pal4/native_property_record_catalog.h"
#include "pal4/input_manager.h"
#include <algorithm>
namespace pal4::test {
inline void VerifyNativeMonsterTargetSelection(NativeGameDatabase& database,
    NativeCombatMonsterManager& manager,NativeSkillSystemOwner& skills,
    NativeEffectSceneSession& effects,NativeScriptEventRouter& router,std::uint32_t combat) {
    const auto check=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
    const auto reject=[&](auto&& call){bool rejected=false;try{call();}catch(const std::logic_error&){rejected=true;}check(rejected,"Monster target ownership guard");};
    const auto actor=manager.StageActorAt(0u);
    const auto cursor=manager.CreateStageIterator();manager.NextStageIterator(cursor);
    const auto clone=manager.CloneStageIterator(cursor);
    check(manager.StageIteratorActor(clone)==manager.StageActorAt(1u),"stage cursor clone preserves actual position");
    reject([&]{manager.LeaveStageAndScene(actor);});
    manager.DeleteStageIterator(cursor);manager.DeleteStageIterator(clone);
    NativePropertyRecordCatalog properties(database);InputManager input;
    NativeCombatMonsterTargetSelection actors(combat,manager,effects);
    NativeCombatTargetCommandServices commands(database,properties,input,skills,actors);
    NativeCombatTargetSelectionImage image;NativeCombatTargetSelection selection(router,commands,image);
    const auto& table=database.Tables().at("Magic");
    const auto column=[&](const char* name){const auto i=std::find_if(table.fields.begin(),table.fields.end(),[&](const auto& f){return f.name==name;});check(i!=table.fields.end(),"original Magic column");return i-table.fields.begin();};
    const auto id=column("ID"),ai=column("AI_CMD_TYPE"),target=column("TARGET");
    const auto row=std::find_if(table.rows.begin(),table.rows.end(),[&](const auto& r){return std::get<std::uint32_t>(r[target])==1u && std::get<std::uint32_t>(r[ai])!=5u;});
    check(row!=table.rows.end(),"actual Magic Monster target record");
    std::array<std::uint32_t,4> payload{actor+716u,0u,std::get<std::uint32_t>((*row)[id]),0u};
    const auto handle=[&](unsigned offset){return *reinterpret_cast<const EffectSystemHandleWords*>(actor+offset);};
    unsigned picked{},unpicked{};bool throw_select{};
    const auto picked_token=router.Subscribe(router.Key("ae_ui_select"),1u,[&](std::uint32_t p){
        check(*reinterpret_cast<const std::uint32_t*>(p)==actor+716u,"real candidate first stage actor");
        check(EffectHandle::DialogNeedsUpdate(handle(852u),effects.ContentHandles()) && EffectHandle::DialogNeedsUpdate(handle(860u),effects.ContentHandles()),"manager retains actual pick/OM03 adapters");
        reject([&]{manager.Close();});reject([&]{manager.CreateMonster(2579,0u);});
        reject([&]{actors.RefreshSelection(selection.Address());});++picked;
        if(throw_select)throw std::logic_error("intentional Monster candidate observer failure");
    });
    const auto unpicked_token=router.Subscribe(router.Key("ae_ui_unselect"),1u,[&](std::uint32_t){++unpicked;});
    struct Observers {NativeScriptEventRouter& router;std::uint32_t a,b;~Observers(){(void)router.Unsubscribe(a);(void)router.Unsubscribe(b);}} observers{router,picked_token,unpicked_token};
    const auto start=[&]{(void)router.Dispatch("start_select_target",reinterpret_cast<std::uint32_t>(payload.data()));};
    const auto stop=[&]{(void)router.Dispatch("stop_select_target",0u);effects.ClearCombatEffects();};
    try {
        input.WriteByte(0x68u,255u);input.WriteDword(0x21d4u,1u);input.WriteDword(0x21c8u,1u);input.WriteDword(0x10u,123u);
        start();
        check(image.words[16]==actor && image.words[18]==0u && !manager.ActiveIterators(),"shared refresh selects actual Monster and retires both cursors");
        check(!input.ReadByte(0x68u) && !input.ReadDword(0x21d4u) && !input.ReadDword(0x21c8u) && input.ReadDword(0x10u)==123u,"real command resets actual input");
        effects.UpdateCombatEffects(0.04);NativeParticleProbeCamera camera;camera.Begin();effects.DrawCombatEffects(0u);(void)camera.EndPixels();
        image.words[18]=2u;check(actors.RefreshSelection(selection.Address())==2u && image.words[16]==actor,"mode2 preserves target");
        image.words[18]=1u;reject([&]{actors.RefreshSelection(selection.Address());});
        check(!manager.ActiveIterators(),"missing Player candidates do not leak Monster cursors");
        stop();check(!image.words[16] && !handle(852u).system && !handle(860u).system,"stop retires manager owned selection handles");
        throw_select=true;reject(start);throw_select=false;
        check(!manager.ActiveIterators() && image.words[16]==actor,"observer failure retires cloned iterator while preserving original writes");
        stop();start();stop();selection.Close();
        check(picked==3u && unpicked==3u,"real event lifecycle survives failure and reuse");
    } catch(...) {throw_select=false;(void)manager.SetSelected(actor,0u,effects);effects.ClearCombatEffects();throw;}
    std::cout<<"Native Monster target selection: actual Magic/stage/status648/input/pick/OM03, cursor clone and exception cleanup, mode2, explicit Player rejection, 3 selects/3 unselects pass\n";
}
}
