#pragma once

#include "pal4/native_package_archive.h"
#include "pal4/native_world_asset.h"
#include "pal4/native_combat_world_resources.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include <rwcore.h>
#include <rpworld.h>
#include <rpmatfx.h>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <cstring>

namespace pal4::test {
inline void VerifyNativeWorldTextureIsolation(const std::filesystem::path& root) {
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    struct RestoreCurrent {
        RwTexDictionary* prior=RwTexDictionaryGetCurrent();
        ~RestoreCurrent(){RwTexDictionarySetCurrent(prior);}
    } restore;
    struct Dictionary {
        RwTexDictionary* value=RwTexDictionaryCreate();
        ~Dictionary(){if(value){if(RwTexDictionaryGetCurrent()==value)RwTexDictionarySetCurrent(nullptr);RwTexDictionaryDestroy(value);}}
    } foreign;
    check(foreign.value!=nullptr,"world collision fixture dictionary");
    const auto dictionary_count=[](){
        unsigned count{};
        RwTexDictionaryForAllTexDictionaries([](RwTexDictionary* dictionary,void* data)->RwTexDictionary*{
            ++*static_cast<unsigned*>(data);return dictionary;
        },&count);
        return count;
    };
    const auto dictionaries_before=dictionary_count();
    NativePackageArchive archive;
    check(archive.Open((root/"gamedata/PALWorld/CombatWorld/M07B.cpk").string().c_str()),"original M07B archive");
    const auto lightmaps=[](RpWorld* world){
        std::map<std::string,std::pair<int,int>> result;
        for(int i=0;i<RpWorldGetNumMaterials(world);++i){
            auto* material=RpWorldGetMaterial(world,i);
            const auto effect=RpMatFXMaterialGetEffects(material);
            if(effect!=rpMATFXEFFECTDUAL && effect!=rpMATFXEFFECTDUALUVTRANSFORM)continue;
            auto* texture=RpMatFXMaterialGetDualTexture(material);
            if(!texture)continue;
            auto* raster=RwTextureGetRaster(texture);
            result.emplace(RwTextureGetName(texture),std::pair{RwRasterGetWidth(raster),RwRasterGetHeight(raster)});
        }
        return result;
    };
    RwTexDictionarySetCurrent(nullptr);
    NativeWorldAsset baseline;
    check(baseline.LoadArenaSection(archive,"M07B"),"original isolated M07B geometry");
    const auto expected=lightmaps(baseline.World());
    check(!expected.empty(),"M07B uses actual dual lightmap textures");
    for(const auto& [name,size]:expected){
        check(size==std::pair{512,512},"original M07B lightmap dimensions512");
        auto* raster=RwRasterCreate(1024,1024,32,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888);
        check(raster!=nullptr,"foreign1024 lightmap raster");
        auto* texture=RwTextureCreate(raster);
        check(texture && RwTextureSetName(texture,name.c_str()) && RwTexDictionaryAddTexture(foreign.value,texture),"same-name foreign lightmap publication");
    }
    baseline.Unload();
    check(dictionary_count()==dictionaries_before,"baseline world retires its resource dictionaries");
    for(unsigned cycle=0;cycle<2;++cycle){
        RwTexDictionarySetCurrent(foreign.value);
        NativeWorldAsset loaded;
        check(loaded.LoadArenaSection(archive,"M07B"),"M07B loads while another dictionary is current");
        check(RwTexDictionaryGetCurrent()==foreign.value,"world load restores borrowed current dictionary");
        const auto actual=lightmaps(loaded.World());
        for(const auto& [name,size]:actual)std::cout<<"M07B lightmap "<<name<<" "<<size.first<<'x'<<size.second<<'\n';
        check(actual==expected,"M07B must use its own512 lightmaps, not foreign same-name1024 textures");
        loaded.Unload();loaded.Unload();
        check(dictionary_count()==dictionaries_before,"repeated world retirement has no retained texture dictionaries");
        check(RwTexDictionaryGetCurrent()==foreign.value,"world retirement preserves foreign dictionary");
        check(!loaded.LoadArenaSection(archive,"missing") && !loaded.World() && RwTexDictionaryGetCurrent()==foreign.value,"failed world load unwinds dictionaries and current selection");
        check(dictionary_count()==dictionaries_before,"failed BSP load releases its pending dictionary");
    }
    std::cout<<"Native world texture isolation: actual M07B dual maps, same-name1024 collision, reload/failure and borrowed current restoration pass\n";
}

inline void VerifyNativeCombatWorldMaterials(const std::filesystem::path& root) {
    const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    NativePkFileManagerRuntime packages;
    const auto manager=packages.InitializePackFileManager();
    std::array<std::uint32_t,20> filesystem{};filesystem[10]=0x1010u;
    packages.ProcessPackFiles(manager,reinterpret_cast<std::uint32_t>(filesystem.data()));
    NativePalGameIVTextureStorage game([](std::uint32_t dictionary){RwTexDictionaryDestroy(reinterpret_cast<RwTexDictionary*>(dictionary));});
    check(game.Construct()!=0u,"Combat material fixture game storage");
    NativeCombatWorldResources world(packages,game);
    for(const auto* name:{"M01","M07B"})for(unsigned cycle=0;cycle<2;++cycle){
        NativePackageArchive reference;
        check(reference.Open((root/"gamedata/PALWorld/CombatWorld"/(std::string(name)+".cpk")).string().c_str()),"Combat material reference archive");
        std::array<std::uint32_t,6> expected{1,1,1,1,1,1};
        if(const auto bytes=reference.ReadMember((std::string(name)+"\\world.flr").c_str())){
            check(bytes->size()>=24u,"original world.flr has six material modes");
            std::memcpy(expected.data(),bytes->data(),24u);
        }
        world.Load(root,"CombatWorld",name);
        const auto* state=reinterpret_cast<const NativeCombatWorldImage*>(world.Address());
        constexpr std::array<unsigned,6> offsets{10,17,22,27,32,41};
        for(unsigned i=0;i<6;++i)check(state->base.words[offsets[i]]==expected[i],"Combat world loads six original world.flr modes");
        auto* geometry=world.Asset().World();
        unsigned checked{};
        for(int i=0;i<RpWorldGetNumMaterials(geometry);++i){
            auto* texture=RpMaterialGetTexture(RpWorldGetMaterial(geometry,i));
            if(!texture || expected[0]>5u)continue;
            std::cout<<"Combat "<<name<<" diffuse "<<RwTextureGetName(texture)<<" filter="<<int(RwTextureGetFilterMode(texture))<<" expected="<<expected[0]+1u<<'\n';
            check(unsigned(RwTextureGetFilterMode(texture))==expected[0]+1u,"Combat BSP material must apply original world.flr filter mode");
            ++checked;
        }
        check(checked && (geometry->flags&0x45u)==0x45u,"Combat world has initialized geometry flags and textured materials");
        world.Reset();
        check(!world.Loaded() && packages.SceneArchiveCount()==0u,"Combat material reload retires mounted resources");
    }
    std::cout<<"Native Combat world materials: real M01/M07B FLR modes, SDK diffuse filters and reload pass\n";
}
}
