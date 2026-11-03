#pragma once
#include "pal4/native_arena_skill_manager_owner.h"
#include "pal4/native_effect_scene_session.h"
#include "pal4/native_gob_object_image.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/player_manager_object.h"
#include "native_signpost_actor_probe.h"
#include <rwcore.h>
#include <rpworld.h>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeSignpostManager(const std::filesystem::path& root,
    NativePalGameIVTextureStorage& game,NativePkFileManagerRuntime& packages,
    PlayerManagerObjectState& players,NativeEffectSceneSession& effects,std::uint32_t arena,
    const std::function<void(std::uint32_t)>& verify_frame = {}){
    const auto check=[](bool v){if(!v)throw std::runtime_error("actual signpost manager assertion");};
    auto& owner=effects.ArenaSkills();auto& memory=owner.Storage();const auto manager=owner.Address();
    for(auto o:{4u,20u,36u})check(memory.ReadDword(manager+o)==memory.ReadDword(manager+o+4u));
    check(!owner.SignpostActorCount());
    owner.BindSignpostActors(game,packages,root,players);
    struct Retire {
        NativeArenaSkillManagerOwner& owner;
        ~Retire(){owner.Clear();owner.UnbindSignpostActors();}
    } retire{owner};
    std::uint32_t first{};RpWorld* world{};std::size_t baseline{};
    for(unsigned i=0;i<11u;++i){
        check(owner.ShowSignpost(0u)==1u);
        const auto record=memory.ReadDword(memory.ReadDword(manager+20u));
        const auto actor=memory.ReadDword(record+32u);
        check(actor && memory.ReadDword(actor)==0x8448a0u && memory.ReadDword(actor+368u)==0x8448ccu);
        auto* clump=reinterpret_cast<RpClump*>(memory.ReadDword(actor+20u));
        check(clump && RpClumpGetWorld(clump));
        for(unsigned o=0;o<12u;o+=4u)check(memory.ReadDword(record+8u+o)==players.slots[0].base.Read(172u+o));
        if(!i){first=record;world=RpClumpGetWorld(clump);baseline=SignpostWorldClumps(world)-1u;}
        check(memory.ReadDword(manager+52u)==(i<10u?i+1u:10u));
    }
    check(owner.SignpostActorCount()==11u && SignpostWorldClumps(world)==baseline+11u &&
          memory.ReadDword(manager+40u)-memory.ReadDword(manager+36u)==12u &&
          memory.ReadDword(memory.ReadDword(manager+36u)+8u)==first);
    if(verify_frame)verify_frame(first);
    const auto first_actor=memory.ReadDword(first+32u);
    auto* first_clump=reinterpret_cast<RpClump*>(memory.ReadDword(first_actor+20u));
    RpMaterial* material{};
    (void)RpClumpForAllAtomics(first_clump,[](RpAtomic* a,void* p)->RpAtomic*{
        auto*& m=*static_cast<RpMaterial**>(p);auto* g=RpAtomicGetGeometry(a);
        if(!m && g && RpGeometryGetNumMaterials(g))m=RpGeometryGetMaterial(g,0);return a;
    },&material);
    check(material!=nullptr);owner.UpdateSignpostFade(0.5f);
    check(RpMaterialGetColor(material)->alpha==128u && owner.SignpostActorCount()==11u);
    owner.UpdateSignpostFade(0.5f);
    check(owner.SignpostActorCount()==10u && memory.ReadDword(manager+40u)==memory.ReadDword(manager+36u) &&
          SignpostWorldClumps(world)==baseline+10u);
    bool rejected=false;try{owner.UnbindSignpostActors();}catch(const std::logic_error&){rejected=true;}
    check(rejected && owner.SignpostActorCount()==10u);
    (void)owner.CleanupPlayerData(manager);check(SignpostWorldClumps(world)==baseline && owner.SignpostActorCount()==10u);
    (void)owner.UpdatePlayerData(manager);check(SignpostWorldClumps(world)==baseline+10u);
    owner.Clear();check(!owner.SignpostActorCount() && SignpostWorldClumps(world)==baseline);
    std::size_t retained=1u;for(auto o:{4u,20u,36u}){
        check(memory.ReadDword(manager+o)==memory.ReadDword(manager+o+4u));
        if(memory.ReadDword(manager+o))++retained;
    }
    check(memory.OutstandingAllocations()==retained);
    {
        NativeArenaSkillManagerOwner failing(effects.ContentHandles());
        failing.BindSignpostActors(game,packages,root/"missing-signpost-root",players);
        bool reentry=false;EffectSystemHandle handle{};
        failing.BindScene(arena,[&](std::string_view name,const std::array<float,3>& pos){
            check(name=="M_001");try{failing.Clear();}catch(const std::logic_error&){reentry=true;}
            handle=owner.CreateSignpostEffect(pos);return handle;
        });
        const auto scripts=packages.LoadedPackageCount();
        rejected=false;try{(void)failing.ShowSignpost(0u);}catch(const std::runtime_error&){rejected=true;}
        check(rejected && reentry && !failing.SignpostActorCount() &&
              failing.Storage().OutstandingAllocations()==1u && packages.LoadedPackageCount()==scripts &&
              !EffectHandle::DialogNeedsUpdate({handle.id,handle.system},effects.ContentHandles()));
        failing.UnbindSignpostActors();failing.UnbindScene();failing.Destroy();
        check(!failing.Storage().OutstandingAllocations());
    }
    std::cout<<"Native signpost manager: actual Player, eleven M001 actors, ten-record rollover, half-alpha fade, hide/restore, shared cleanup, reentry rejection and failed-load rollback pass\n";
}
}
