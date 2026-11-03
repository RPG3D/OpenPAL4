#pragma once
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_effect_xml_objects.h"
#include "pal4/native_gob_clump_asset.h"
#include "pal4/effect_handle.h"
#include "native_particle_probe_camera.h"
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeCombatXml(NativeEffectSceneSession& effects,const std::filesystem::path& root) {
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto reject=[&](auto&& f){bool blocked=false;try{f();}catch(const std::logic_error&){blocked=true;}check(blocked,"Combat XML foreign owner rejection");};
    NativeGobClumpAsset weapon;
    check(weapon.Load(root,"gamedata\\PALWeapon\\WTA\\","WTA"),"Combat XML original WTA");
    struct Cleanup {
        NativeEffectSceneSession& effects;
        EffectSystemHandleWords scene{};
        ~Cleanup(){try{EffectHandle::RemoveContext(scene,effects.ContextHandles());effects.ClearCombatEffects();}catch(...){}}
    } cleanup{effects};
    const auto clump=reinterpret_cast<std::uint32_t>(weapon.Clump());
    const auto scene_before=effects.XmlEffects().Count();
    const auto scene=effects.XmlEffects().LoadFile("ZL-20",clump);
    cleanup.scene={scene.id,scene.system};
    check(EffectHandle::IsReadyToShow(cleanup.scene,effects.ContextHandles()),"real scene XML control handle");
    const auto scene_copy=cleanup.scene;
    reject([&]{(void)effects.CombatXmlEffectReady(cleanup.scene);});
    reject([&]{effects.RemoveCombatXmlEffect(cleanup.scene);});
    reject([&]{effects.DrainCombatXmlEffect(cleanup.scene);});
    check(cleanup.scene.id==scene_copy.id && cleanup.scene.system==scene_copy.system &&
          effects.XmlEffects().Count()==scene_before+1u,"foreign rejection preserves scene owner and handle");
    check(!effects.CombatXmlEffectCount(),"Combat XML starts drained");
    auto missing=effects.LoadCombatXmlEffect("__missing_combat_hef__",clump);
    check(!missing.system && !effects.CombatXmlEffectReady(missing) && !effects.CombatXmlEffectCount(),"missing Combat XML stays invalid");
    NativeParticleProbeCamera camera;
    for(unsigned pass=0;pass<2u;++pass) {
        auto first=effects.LoadCombatXmlEffect("ZL-20",clump);
        auto second=effects.LoadCombatXmlEffect("ZL-20",clump);
        check(first.system && first.system==second.system && first.system!=scene.system && first.id!=second.id &&
              effects.CombatXmlEffectCount()==2u,"two actual Combat XML objects in independent owner");
        check(effects.CombatXmlEffectReady(first) && effects.CombatXmlEffectReady(second),"Combat context lookup uses actual Combat tree");
        check(!EffectHandle::IsReadyToShow(first,effects.ContextHandles()),"scene context does not claim Combat handles");
        effects.UpdateCombatEffects(0.05);
        camera.Begin();effects.DrawCombatEffects(0u);(void)camera.EndPixels();
        const auto stale=first;
        effects.RemoveCombatXmlEffect(first);
        effects.RemoveCombatXmlEffect(first);
        check(!first.system && first.id==0xffffffffu && !effects.CombatXmlEffectReady(stale) &&
              effects.CombatXmlEffectCount()==1u && effects.CombatXmlEffectReady(second),"immediate removal resets handle and preserves sibling");
        effects.DrainCombatXmlEffect(second);
        check(!second.system && second.id==0xffffffffu && effects.CombatXmlEffectCount()==1u,
              "drain transfers context to real timed list before retiring handle");
        effects.UpdateCombatEffects(20.0);
        check(!effects.CombatXmlEffectCount(),"timed Combat XML resource retires through frame update");
        check(EffectHandle::IsReadyToShow(cleanup.scene,effects.ContextHandles()) &&
              effects.XmlEffects().Count()==scene_before+1u,"Combat removal and drain preserve scene XML");
    }
    auto cleared=effects.LoadCombatXmlEffect("ZL-20",clump);
    check(effects.CombatXmlEffectReady(cleared),"Combat XML before group clear");
    effects.ClearCombatEffects();
    check(!effects.CombatXmlEffectCount() && !effects.CombatXmlEffectReady(cleared),"group clear retires XML payload and leaves stale lookup false");
    effects.RemoveCombatXmlEffect(cleared);
    check(!cleared.system,"stale Combat context handle can be reset after clear");
    EffectHandle::RemoveContext(cleanup.scene,effects.ContextHandles());
    check(effects.XmlEffects().Count()==scene_before,"scene control XML balanced");
    std::cout<<"Native Combat XML: original WTA/ZL-20, distinct scene/Combat context owners, missing input, two immediate/timed cycles, real update/draw, stale/foreign handles and group-clear retirement pass\n";
}
}
