#pragma once
#include "pal4/native_combat_world_resources.h"
#include "pal4/native_combat_translator.h"
#include "pal4/native_world_asset.h"
#include "pal4/native_package_archive.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_camera_manager_owner.h"
#include "native_combat_vertex_probe.h"
#include "native_combat_draw_probe.h"
#include <rpworld.h>
#include <cmath>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeCombatWorldResources(const std::filesystem::path& root,
    NativePkFileManagerRuntime& packages,NativePalGameIVTextureStorage& game,
    NativeCameraManagerOwner& cameras) {
    const auto check=[](bool ok,const char* message) {if(!ok) throw std::runtime_error(message);};
    const auto archives=packages.SceneArchiveCount();
    const auto allocations=packages.AllocationCount();
    const auto active=cameras.ActiveCameraPointer();
    const auto word=[](std::uint32_t a) {return *reinterpret_cast<const std::uint32_t*>(a);};
    const auto file=packages.BoundFileSystem();
    const auto depth=word(packages.ManagerAddress()+120u);
    const auto combat_archive=packages.LoadSceneArchive(
        root/"gamedata"/"PALWorld"/"CombatWorld"/"M01.cpk",NativeWorldArchiveKind::combat);
    check(combat_archive!=0u,"Combat package callback owner");
    check(packages.WithSceneArchive(combat_archive,[&](NativePackageArchive&) {
        using Exists=std::uint32_t(__cdecl*)(std::uint32_t,const char*);
        const auto exists=reinterpret_cast<Exists>(word(file+76u));
        check(exists(0u,"gamedata\\PALWorld\\CombatWorld\\M01\\M01.bsp")!=0u,
            "original Combat mapper resolves full world path");
        check(exists(0u,"gamedata\\PALWorld\\Q01\\M01\\M01.bsp")==0u,
            "Combat mapper must not use normal scene directory stripping");
        return true;
    }),"Combat archive mount");
    packages.DestroySceneArchive(combat_archive);
    check(word(packages.ManagerAddress()+120u)==depth && packages.SceneArchiveCount()==archives &&
        packages.AllocationCount()==allocations,"Combat callback scope restores package owners");
    NativeCombatWorldResources world(packages,game);
    check(!world.Loaded() && !world.TextAllocations() && !world.GeometryAllocations(),"cold Combat world");
    bool cold_rejected=false;try{(void)world.GeometryQueue();}catch(const std::logic_error&){cold_rejected=true;}
    check(cold_rejected,"unloaded Combat world cannot lend geometry queue");
    for(unsigned cycle=0;cycle<2u;++cycle) {
        world.Load(root,"CombatWorld","M01");
        const auto& asset=world.Asset();
        auto* image=reinterpret_cast<const std::uint32_t*>(world.Address());
        check(world.Loaded() && world.TextAllocations()==3u && asset.World() &&
            asset.Light() && asset.SectorCount()>0u && RpWorldGetNumMaterials(asset.World())>0,
            "original M01 combat geometry and ambient light");
        check(image[0]==0x845308u && image[4]==reinterpret_cast<std::uint32_t>(asset.World()) &&
            image[5]==reinterpret_cast<std::uint32_t>(asset.Light()) && image[12]==1u &&
            std::string_view(reinterpret_cast<const char*>(image[1]))=="gamedata\\PALWorld\\CombatWorld\\M01\\" &&
            std::string_view(reinterpret_cast<const char*>(image[2]))=="CombatWorld" &&
            std::string_view(reinterpret_cast<const char*>(image[3]))=="M01",
            "Combat embedded scene has original paths and resource fields");
        for(unsigned slot=0;slot<5u;++slot)
            check(image[13u+slot*5u]==reinterpret_cast<std::uint32_t>(asset.ClumpAt(slot)),"Combat clump slots");
        check(asset.TextureRequests()>0u && !asset.MissingTextures() && !asset.TextureDecodeFailures(),
            "original Combat textures decode without missing resources");
        struct VertexInputs { unsigned sectors{}, vertices{}, meshes{}, tangent_sectors{}; RwUInt32 usage{}; } inputs;
        RpWorldForAllWorldSectors(asset.World(), [](RpWorldSector* sector, void* data) -> RpWorldSector* {
            auto& info=*static_cast<VertexInputs*>(data);
            ++info.sectors;
            info.vertices+=sector->numVertices;
            if(sector->mesh) info.meshes+=sector->mesh->numMeshes;
            const auto usage=RpD3D9WorldSectorGetUsageFlags(sector);
            info.usage|=usage;
            if(usage&0x10000u) ++info.tangent_sectors;
            return sector;
        }, &inputs);
        check(inputs.sectors==asset.SectorCount() && inputs.vertices && inputs.meshes,
            "original Combat instancing input covers all populated sectors");
        std::cout<<"Native Combat vertex inputs: vertices="<<inputs.vertices<<" meshes="<<inputs.meshes
            <<" usage="<<inputs.usage<<" tangent-sectors="<<inputs.tangent_sectors<<'\n';
        VerifyNativeCombatVertices(*asset.World());
        VerifyNativeCombatDrawing(world,packages,cameras,root);
        NativePackageArchive reference;
        check(reference.Open((root/"gamedata"/"PALWorld"/"CombatWorld"/"M01.cpk").string().c_str()),"reference Combat archive");
        const auto bytes=reference.ReadMember("M01\\M01_ltMap.cfg");
        check(world.LightmapConfigLoaded()==bytes.has_value(),"original optional Combat lightmap presence");
        if(bytes) {
            check(bytes->size()>=16u,"original Combat lightmap size");
            const auto values=world.LightmapParameters();
            check(std::memcmp(values.data()+1u,bytes->data(),12u)==0 &&
                std::memcmp(values.data(),bytes->data()+12u,4u)==0,"original Combat lightmap byte order");
            for(const auto value:values) check(std::isfinite(value),"finite Combat lightmap parameters");
        }
        const auto first=asset.World();
        world.Load(root,"CombatWorld","M01");
        check(world.Asset().World()==first && packages.SceneArchiveCount()==archives+1u,"same Combat world reuses resources");
        bool rejected{};
        try {world.Load(root,"CombatWorld","missing");} catch(const std::logic_error&) {rejected=true;}
        check(rejected && world.Asset().World()==first,"replacement cannot discard borrowed Combat resources");
        std::cout<<"Native Combat world M01: sectors="<<asset.SectorCount()<<" clumps="<<asset.ClumpCount()
            <<" textures="<<asset.TextureCount()<<" lightmap="<<world.LightmapConfigLoaded()<<'\n';
        world.Reset();world.Reset();
        check(world.GeometryAllocations()==2u && word(world.Address()+200u)==word(world.Address()+204u) &&
              word(world.Address()+216u)==word(world.Address()+220u),
              "Combat reset clears both live queues and retains owned capacity until scene destruction");
        check(!world.Loaded() && !world.RenderPipelineReady() && !world.TextAllocations() && !world.Asset().World() &&
            *reinterpret_cast<const std::uint32_t*>(world.Address()+228u)==0u &&
            packages.SceneArchiveCount()==archives && packages.AllocationCount()==allocations,
            "Combat world shared cleanup retires resources/text/archive exactly once");
    }
    bool failed{};
    try {world.Load(root,"CombatWorld","missing");} catch(const std::runtime_error&) {failed=true;}
    check(failed && !world.Loaded() && !world.TextAllocations() && packages.SceneArchiveCount()==archives,
        "missing Combat archive leaves clean reusable owner");
    failed=false;
    try {world.Load(root,"../CombatWorld","M01");} catch(const std::invalid_argument&) {failed=true;}
    check(failed && !world.Loaded(),"invalid Combat path is rejected");
    world.Load(root,"CombatWorld","M01");world.Reset();
    {
        NativeCombatWorldResources implicit(packages,game);
        implicit.Load(root,"CombatWorld","M01");
        check(implicit.Loaded(),"implicit Combat cleanup starts with live resources");
    }
    // This is only a layout fixture, not a constructed CombatSystem. Exercise
    // the exact parent+116 scene address without inventing the missing owners.
    struct ParentImage {
        std::array<std::uint32_t,29> prefix;
        NativeCombatWorldImage scene;
        std::array<std::uint32_t,8> descriptors;
        NativeCombatTranslatorImage translator;
        std::array<std::uint32_t,31> tail;
    } parent{};
    static_assert(offsetof(ParentImage,scene)==116u && sizeof(ParentImage)==520u);
    static_assert(offsetof(ParentImage,translator)==380u);
    parent.prefix.fill(0x13579bdfu);parent.tail.fill(0x2468ace0u);
    const auto canaries=[&] {
        check(std::all_of(parent.prefix.begin(),parent.prefix.end(),[](auto v){return v==0x13579bdfu;}) &&
              std::all_of(parent.tail.begin(),parent.tail.end(),[](auto v){return v==0x2468ace0u;}),
              "embedded Combat world preserves parent fields");
    };
    for(unsigned cycle=0;cycle<2u;++cycle) {
        {
            NativeCombatWorldResources embedded(packages,game,parent.scene);
            NativeCombatTranslator translation(parent.translator);
            check(translation.Address()==reinterpret_cast<std::uint32_t>(&parent)+380u &&
                  parent.translator.words[0] && translation.Load(root) && translation.Find("EASI_NONE"),
                  "parent scene and translator are simultaneously constructed in-place");
            check(embedded.Address()==reinterpret_cast<std::uint32_t>(&parent)+116u,
                  "embedded Combat world aliases parent scene directly");
            embedded.Load(root,"CombatWorld","M01");
            check(parent.scene.base.words[4]==reinterpret_cast<std::uint32_t>(embedded.Asset().World()) &&
                  parent.scene.base.words[5]==reinterpret_cast<std::uint32_t>(embedded.Asset().Light()) &&
                  embedded.GeometryQueue()==reinterpret_cast<std::uint32_t>(&parent)+328u,
                  "embedded Combat SDK fields and actor queue use parent storage");
            VerifyNativeCombatDrawing(embedded,packages,cameras,root);
            check(parent.scene.tail[8]!=0u && embedded.GeometryAllocations()==2u,
                  "embedded pipeline and populated queues live in parent scene");
            std::array<std::uint8_t,sizeof(NativeCombatWorldImage)> snapshot;
            std::memcpy(snapshot.data(),&parent.scene,snapshot.size());
            bool duplicate=false;
            try {NativeCombatWorldResources conflict(packages,game,parent.scene);}
            catch(const std::logic_error&) {duplicate=true;}
            check(duplicate && std::memcmp(snapshot.data(),&parent.scene,snapshot.size())==0 &&
                  embedded.Loaded() && embedded.RenderPipelineReady(),
                  "duplicate embedded owner rejects before changing live pointers");
            canaries();
            if(cycle==0u) {
                embedded.Reset();embedded.Reset();
                check(!parent.scene.base.words[4] && !parent.scene.tail[8] &&
                      !embedded.TextAllocations(),"embedded reset clears actual parent resources");
            }
            // Second cycle intentionally destroys loaded resources in-place.
        }
        check(!parent.translator.words[0] && !parent.translator.words[1] &&
              !parent.scene.base.words[4] && !parent.scene.tail[1] && !parent.scene.tail[5] &&
              !parent.scene.tail[8] && packages.SceneArchiveCount()==archives &&
              packages.AllocationCount()==allocations,
              "embedded destruction releases queues/archive without freeing parent");
        canaries();
    }
    std::cout<<"Native Combat embedded world: parent=520 scene-offset=116 cycles=2 duplicate-safe=1\n";
    check(cameras.ActiveCameraPointer()==active && packages.SceneArchiveCount()==archives &&
        packages.AllocationCount()==allocations,"Combat preparation preserves active Scene camera and package owners");
}
}
