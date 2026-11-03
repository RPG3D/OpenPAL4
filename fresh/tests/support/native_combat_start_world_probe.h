#pragma once
#include "pal4/native_combat_start_world.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_combat_world_resources.h"
#include "pal4/native_combat_world_names.h"
#include "pal4/native_world_asset.h"
#include "pal4/native_camera_manager_owner.h"
#include <rwcore.h>
#include <rpworld.h>
#include <iostream>
#include <stdexcept>
#include <string>
namespace pal4::test {
inline void VerifyNativeCombatStartWorld(NativeCombatSystemImage& image,NativeCombatWorldResources& world,
    NativeCombatWorldNames& names,NativeCameraManagerOwner& cameras,INativeSkillSystemServices& skills,
    const std::filesystem::path& root,std::uint32_t effect){
    const auto check=[](bool v,const char* why){if(!v)throw std::runtime_error(why);};
    const std::string region(names.Region()),scene(names.Scene());
    const auto camera_data=reinterpret_cast<std::uint32_t>(&cameras.CameraDataForObject(cameras.FindByName("CombatCamera")));
    auto* camera=reinterpret_cast<RwCamera*>(*reinterpret_cast<const std::uint32_t*>(camera_data));
    const auto active=cameras.ActiveCameraPointer(),state=image.State();
    struct Restore {
        NativeCombatSystemImage& image;NativeCombatWorldResources& world;NativeCombatWorldNames& names;
        NativeCameraManagerOwner& cameras;INativeSkillSystemServices& skills;const std::filesystem::path& root;
        std::uint32_t effect;std::string region,scene;bool restored{};
        ~Restore(){if(!restored)try{LoadNativeCombatStartWorld(image,world,names,cameras,skills,root,effect,region,scene);}catch(...){}}
    } restore{image,world,names,cameras,skills,root,effect,region,scene};
    const auto invoke=[&](std::string_view next){LoadNativeCombatStartWorld(image,world,names,cameras,skills,root,effect,region,next);};
    auto* first=world.Asset().World();const auto pipeline=image.world.tail[8];
    invoke(scene);invoke(scene);
    check(world.Asset().World()==first && image.world.tail[8]==pipeline && world.RenderPipelineReady() &&
        RwCameraGetWorld(camera)==first && image.target_selection.words[14]==world.Address() &&
        image.target_selection.words[15]==camera_data && !world.GeometryAllocations(),
        "same-name53F3B0 retains actual world/pipeline and rebinds real camera/projection");
    auto* foreign=reinterpret_cast<RwCamera*>(cameras.CameraDataForObject(cameras.FindByName("MainCamera")).At(0u));
    check(foreign!=camera && RwCameraGetWorld(foreign)!=nullptr,"foreign camera is the actual live Scene camera");
    bool rejected{};try{(void)world.AttachCamera(reinterpret_cast<std::uint32_t>(foreign));}
    catch(const std::invalid_argument&){rejected=true;}
    check(rejected && RwCameraGetWorld(camera)==first && image.world.base.words[11]==reinterpret_cast<std::uint32_t>(camera),
        "foreign Scene camera cannot replace Combat membership or image field");
    const std::string alternate=scene=="M02A"?"M01":"M02A";
    invoke(alternate);
    check(world.Loaded() && names.Matches(region,alternate) && world.RenderPipelineReady() &&
        RwCameraGetWorld(camera)==world.Asset().World() && std::string(reinterpret_cast<const char*>(image.world.base.words[3]))==alternate,
        "changed original Combat scene replaces resources/cache/pipeline and camera");
    bool failed{};try{invoke("MISSING_COMBAT_SCENE_PROBE");}catch(const std::exception&){failed=true;}
    check(failed && !world.Loaded() && !world.RenderPipelineReady() && names.Region().empty() && names.Scene().empty() &&
        !RwCameraGetWorld(camera) && !world.GeometryAllocations(),
        "failed replacement retires world/camera/pipeline and invalidates names before retry");
    invoke(scene);restore.restored=true;
    check(world.Loaded() && names.Matches(region,scene) && RwCameraGetWorld(camera)==world.Asset().World() &&
        cameras.ActiveCameraPointer()==active && image.State()==state,
        "real original scene retry restores resources without activating camera or changing scheduler state");
    std::cout<<"Native53F3B0 Start world: same-name cache/rebind, real M02A replacement, missing archive cleanup/retry, actual foreign Scene camera rejection, queue/cache/projection/SDK membership and unchanged active camera/state pass\n";
}
}
