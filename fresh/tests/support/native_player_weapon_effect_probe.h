#pragma once
#include "native_combat_effect_group_probe.h"
#include "pal4/native_player_weapon_effect_host.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_effect_xml_objects.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_gob_clump_asset.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_player_transform_runtime.h"
#include "pal4/base_object_hand_effect.h"
#include "pal4/native_player_equipment_visual_runtime.h"
#include "pal4/native_player_equipment_session.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_game_database.h"
#include "pal4/player_manager_object.h"
#include "pal4/game_world_object.h"
#include "native_particle_probe_camera.h"
#include "native_normal_map_probe.h"
#include "native_effect_model_world_probe.h"
#include <array>
#include <iostream>
#include <source_location>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativePlayerWeaponEffects(const std::filesystem::path& root) {
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) throw std::runtime_error("native weapon effects line "+std::to_string(at.line()));
    };
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    struct Directory {
        std::filesystem::path old=std::filesystem::current_path();
        ~Directory(){std::filesystem::current_path(old);}
    } directory;
    std::filesystem::current_path(root);
    NativePkFileManagerRuntime packages;
    std::array<std::uint32_t,20> filesystem{};filesystem[10]=0x1010u;
    packages.ProcessPackFiles(packages.InitializePackFileManager(),address(filesystem.data()));
    check(packages.LoadAllScripts()!=0u);
    VerifyNativeNormalMap(root);
    VerifyNativeEffectModelWorld(root);
    const auto package_baseline=packages.LoadedPackageCount();
    NativeAnimatedActorAsset actor;check(actor.Load(root,"103"));
    struct World {
        RpWorld* p{};
        ~World(){if(p) RpWorldDestroy(p);}
    } world;
    RwBBox bounds{{1000,1000,1000},{-1000,-1000,-1000}};
    world.p=RpWorldCreate(&bounds);check(world.p);
    // The model-effect service borrows this real SDK world through the same
    // typed arena boundary used by scene composition. Keep it alive until the
    // effect session has retired its pooled model owners.
    GameWorldObjectState arena;
    arena.words[4]=address(world.p);arena.words[12]=1u;
    PlayerManagerObjectState players;auto& slot=players.slots[0];
    auto& state=slot.base;state.Write(0x14u,address(actor.Main()));
    const char role[]="Player-ZIYING";state.Write(4u,address(role));
    struct Resources final : INativePlayerWeaponResources {
        NativePlayerNamedWeaponRuntime* weapons{};
        void CleanupNamedResource(BaseObjectState& s,std::uint32_t o,std::uint32_t c) override {
            if(!weapons) throw std::logic_error("weapon resource owner not bound");
            weapons->CleanupClump(s,o,c);
        }
    } resources;
    NativeGobClumpAsset ordinary;
    // Billboard healing effects borrow the scene camera from creation through drain.
    // Keep the camera alive after the session and initialize its actual SDK frame.
    NativeParticleProbeCamera camera;
    camera.Begin();(void)camera.EndPixels();
    NativeEffectSceneSession effects(root,packages);
    effects.BindCamera(address(camera.camera));
    auto host=effects.CreateWeaponHost(resources);
    NativePlayerTransformRuntime transforms;
    NativePlayerNamedWeaponRuntime weapons(root,*world.p,0x7000u,packages,*host,transforms);
    resources.weapons=&weapons;
    struct Cleanup {
        BaseObjectState& state;NativePlayerNamedWeaponRuntime& weapons;NativeEffectSceneSession& effects;
        ~Cleanup(){(void)BaseObjectHandEffect::CleanupAll(state,weapons);effects.Clear();}
    } cleanup{state,weapons,effects};
    const char empty[]="",eft[]="foot_2",hef[]="ZL-20",missing[]="__missing_weapon_hef__";
    check(!host->EffectAvailable(0u) && !host->EffectAvailable(address(empty)) && host->EffectAvailable(address(hef)));
    check(ordinary.Load(root,"gamedata\\PALWeapon\\WTA\\","WTA"));
    const std::array<float,3> scale{1,1,1};
    check(BaseObjectWeapon::SetupRight(state,address(ordinary.Clump()),address(scale.data()),address(eft),weapons)==1u);
    check(host->EffectNeedsUpdate(state,0x520u) && effects.ActiveCount()==1u);
    effects.Update(0.05);
    camera.Begin();effects.Draw(address(camera.camera));(void)camera.EndPixels();
    check(BaseObjectWeapon::CleanupRight(state,weapons)==1u && !state.Read(0x518u) && !state.Read(0x524u));
    check(!RwFrameGetParent(RpClumpGetFrame(ordinary.Clump())) && !RpClumpGetWorld(ordinary.Clump()));
    ordinary.Unload();effects.Update(20.0);effects.Clear();
    check(!effects.ActiveCount() && !effects.PendingCount());
    const char path[]="PALWeapon\\";
    for(unsigned pass=0;pass<2u;++pass) {
        check(BaseObjectNamedWeapon::SetupRight(state,"WTA","",address(path),address(hef),address(missing),weapons)==1u);
        check(host->EffectReady(state,0x4f8u) && !host->EffectReady(state,0x500u));
        check(effects.XmlEffects().Count()==1u && weapons.OwnedCount()==1u);
        check(BaseObjectNamedWeapon::SetupLeft(state,"WTA","",address(path),address(hef),0u,weapons)==1u);
        check(host->EffectReady(state,0x508u) && effects.XmlEffects().Count()==2u && weapons.OwnedCount()==2u);
        effects.Update(0.05);camera.Begin();effects.Draw(address(camera.camera));(void)camera.EndPixels();
        // The original cleanup transfers HEFs to timed ownership before freeing
        // named resources. Subsequent update/draw runs after both clumps die.
        check(BaseObjectNamedWeapon::CleanupRight(state,weapons)==1u);
        check(BaseObjectNamedWeapon::CleanupLeft(state,weapons)==1u);
        check(!weapons.OwnedCount() && !state.Read(0x22cu) && !state.Read(0x39cu));
        check(!state.Read(0x4fcu) && !state.Read(0x50cu) && effects.XmlEffects().Count()==2u);
        effects.Update(0.05);camera.Begin();effects.Draw(address(camera.camera));(void)camera.EndPixels();
        effects.Update(20.0);check(!effects.XmlEffects().Count());
        check(packages.LoadedPackageCount()==package_baseline);
    }
    check(BaseObjectHandEffect::CleanupAll(state,weapons)==1u && !weapons.OwnedCount());
    {
        NativeAnimatedActorAsset healer;
        check(healer.Load(root,"106"));
        state.Write(0x14u,address(healer.Main()));
        const char healing[]="H_TL03.2";
        check(host->EffectAvailable(address(healing)));
        for(unsigned pass=0;pass<2u;++pass) {
            check(BaseObjectHandEffect::SetupRight(state,address(healing),weapons)==1u);
            check(host->EffectNeedsUpdate(state,0x520u) && state.Read(0x524u));
            effects.Update(0.05);camera.Begin();effects.Draw(address(camera.camera));(void)camera.EndPixels();
            check(BaseObjectHandEffect::UpdateWeaponEffects(state,weapons)==1u);
            check(state.Read(0x520u)==0xffffffffu && !state.Read(0x524u) &&
                  !host->EffectNeedsUpdate(state,0x520u));
            effects.Update(20.0);effects.Clear();
            check(!effects.ActiveCount() && !effects.PendingCount());
        }
        state.Write(0x14u,address(actor.Main()));
        healer.Unload();
        check(packages.LoadedPackageCount()==package_baseline);
        std::cout<<"Native Mengli106 H_TL03.2: real right-hand hook, attached effect update/draw, repeated detach/drain and package balance pass\n";
    }
    NativeGameDatabase database;check(database.Load(root));
    NativeInventorySession inventory;inventory.BindDatabase(database);check(inventory.Start());
    NativePlayerEquipmentVisualRuntime equipment(inventory,weapons);
    const auto sword=inventory.ProcessEquipment(inventory.Address()+0x7cu,3501u);check(sword);
    auto& weapon_sequence=slot.tail[(0x920u-BaseObjectState::kSize)/4u];
    weapon_sequence=inventory.ReadDword(sword+400u);
    const auto read_text=[&](std::uint32_t at) {
        const auto p=inventory.ReadDword(at);return p?std::string(reinterpret_cast<const char*>(p)):std::string{};
    };
    check(read_text(sword+220u)=="WTA");
    std::cout<<"Original Equip3501 visual snapshot: model="<<read_text(sword+220u)
        <<" texture="<<read_text(sword+236u)<<" HEF0="<<read_text(sword+204u)
        <<" HEF1="<<read_text(sword+252u)<<" left="<<read_text(sword+284u)
        <<" right="<<read_text(sword+300u)<<'\n';
    const char* roles[]{"Player-TIANHE","Player-LINSHA","Player-MENGLI","Player-ZIYING"};
    const auto alternate=weapons.ResolveTexturePath(weapons.TextureDictionary(),"PALWeapon\\WTA\\WTA1");
    check(alternate && packages.LoadedPackageCount()==package_baseline);
    for(unsigned role_index=0;role_index<4u;++role_index) {
        state.Write(4u,address(roles[role_index]));
        equipment.Apply(slot);
        const bool left=role_index!=3u,right=role_index==1u || role_index==3u;
        check((state.Read(0x39cu)!=0u)==left && (state.Read(0x22cu)!=0u)==right);
        check(weapons.OwnedCount()==(role_index==1u ? 2u : 1u));
        if(left) {
            check(std::string(reinterpret_cast<const char*>(state.Read(0x388u+16u)))=="WTA1");
            struct TextureCheck {std::uint32_t expected;unsigned materials{};bool valid{true};} textures{alternate};
            (void)RpClumpForAllAtomics(reinterpret_cast<RpClump*>(state.Read(0x39cu)),
                [](RpAtomic* atomic,void* data)->RpAtomic* {
                    auto& result=*static_cast<TextureCheck*>(data);
                    if(auto* geometry=RpAtomicGetGeometry(atomic))
                        (void)RpGeometryForAllMaterials(geometry,[](RpMaterial* material,void* p)->RpMaterial* {
                            auto& r=*static_cast<TextureCheck*>(p);++r.materials;
                            r.valid &= reinterpret_cast<std::uint32_t>(RpMaterialGetTexture(material))==r.expected;
                            return material;
                        },&result);
                    return atomic;
                },&textures);
            check(textures.materials && textures.valid);
            // Original WTA1 configuration re-enables the environment effect
            // after LoadTexture4 resets the old MatFX/default pipeline state.
            check(state.Read(0x388u+78u*4u)==1u && state.Read(0x388u+86u*4u)!=0u);
        } else check(!state.Read(0x388u+16u) && !state.Read(0x388u+20u));
        check(packages.LoadedPackageCount()==package_baseline);
        effects.Update(0.05);camera.Begin();effects.Draw(address(camera.camera));(void)camera.EndPixels();
        equipment.Cleanup(slot);effects.Update(20.0);effects.Clear();
        check(!weapons.OwnedCount() && !effects.XmlEffects().Count());
        check(!state.Read(0x388u+16u) && !state.Read(0x218u+16u));
        check(packages.LoadedPackageCount()==package_baseline);
    }
    weapon_sequence=0xffffffffu;equipment.Apply(slot);check(!weapons.OwnedCount());
    weapon_sequence=inventory.ReadDword(sword+400u);
    state.Write(4u,address("Player-unknown"));equipment.Apply(slot);check(!weapons.OwnedCount());
    // Mutate the live inventory snapshot, as opposed to the immutable database
    // catalog. This is a host-authored HEF override, not a save-file restoration.
    auto& effect_override=*reinterpret_cast<std::uint32_t*>(sword+204u);
    const auto original_effect=effect_override;
    effect_override=address(hef);state.Write(4u,address(roles[0]));
    for (unsigned pass=0;pass<2u;++pass) {
        {
            NativePlayerEquipmentSession session(root,*world.p,0x7000u,players,inventory,packages,effects,transforms);
            PlayerManagerSlotState foreign;
            bool rejected{};
            try {session.Apply(foreign);}catch(const std::invalid_argument&){rejected=true;}
            check(rejected);
            session.Apply(slot);
            check(session.OwnedCount()==1u && effects.XmlEffects().Count()==1u);
            // The two base-actor handles are independent of hand/weapon
            // effects and must retire before their scene/frame owner dies.
            for(const auto offset:{0x534u,0x53cu}) {
                auto created=host->CreateEffect(host->EffectSingleton(),address(eft),0x84b1dcu,1u,0u);
                state.Write(offset,created.persistent.first);
                state.Write(offset+4u,created.persistent.second);
                host->ResetTemporary(created.temporary);
                host->SetEffectProperties(state,offset,address(RpClumpGetFrame(actor.Main())));
                host->RefreshEffect(state,offset);
                check(session.DialogNeedsUpdate(state,offset));
            }
            effects.Update(0.05);camera.Begin();effects.Draw(address(camera.camera));(void)camera.EndPixels();
            if(!pass) {
                session.Clear();session.Clear();
                check(!session.OwnedCount() && !state.Read(0x39cu));
                check(!session.DialogNeedsUpdate(state,0x534u) &&
                      !session.DialogNeedsUpdate(state,0x53cu));
            }
            // The second pass leaves a live weapon for the session destructor.
        }
        check(!state.Read(0x39cu) && !state.Read(0x388u+16u));
        check(!host->EffectNeedsUpdate(state,0x534u) && !host->EffectNeedsUpdate(state,0x53cu));
        effects.Update(0.05);camera.Begin();effects.Draw(address(camera.camera));(void)camera.EndPixels();
        effects.Update(20.0);effects.Clear();
        check(!effects.XmlEffects().Count() && packages.LoadedPackageCount()==package_baseline);
    }
    effect_override=original_effect;
    state.Write(4u,address(role));
    // Combat death also exercises full shared cache/pool retirement. Run it
    // after scene weapon consumers have drained and finished using this group.
    effects.BindArena(address(&arena));
    VerifyNativeCombatEffectGroup(effects,root,packages);
    effects.UnbindArena();
    std::cout<<"Native equipment session: live inventory HEF override, both base-actor EFT handles, foreign-slot rejection, explicit repeated clear/destructor, release before timed update/draw, two fresh sessions and package balance verified\n";
    std::cout<<"Native equipment visual selection: original inventory sequence/Equip3501 WTA/WTA1, actual cached texture identity, environment reload, four role/hand branches, package balance, missing sequence and unknown role verified\n";
    std::cout<<"Native weapon effect host: original103/WTA EFT attachment/drain; both named hands original ZL-20 HEF, missing slot, release-before-timed-frame and reload twice verified\n";
}
}
