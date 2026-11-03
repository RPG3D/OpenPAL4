#pragma once
#include "native_scene_archive_probe.h"
#include "pal4/native_scene_movement_asset.h"
#include <iostream>
#include <rwcore.h>
#include <rpworld.h>

namespace pal4::test {
inline void VerifyNativeSceneIntri(const std::filesystem::path& root,std::uint32_t game) {
    // Original PALWorld packages contain no indexed *_intri.dff in the Scene
    // catalog. Exercise the same source-taking loader with a real SDK floor
    // Clump; this proves ownership/registration, not original intri data parity.
    NativePackageArchive package;
    CheckSceneArchive(package.Open((root/"gamedata"/"scenedata.cpk").string().c_str()));
    constexpr auto member="Q01\\N01\\N01_floor.dff";
    NativeSceneMovementAsset movement;
    for(unsigned pass=0;pass<2u;++pass) {
        CheckSceneArchive(movement.LoadIntri(package,member,game));
        const auto intri=movement.Intri();
        const auto list=movement.State().words[5u];
        const auto count=movement.IntriAtomics();
        CheckSceneArchive(intri && count && movement.RegisteredIntriAtomics()==count);
        CheckSceneArchive(!movement.LoadIntri(package,member,game));
        CheckSceneArchive(movement.Intri()==intri);
        CheckSceneArchive(movement.Load(root,"Q01","N01"));
        CheckSceneArchive(movement.Intri()==intri && movement.State().words[5u]==list &&
            movement.IntriAtomics()==count && movement.RegisteredIntriAtomics()==count &&
            movement.RegisteredFloorAtomics()==movement.FloorAtomics() &&
            movement.RegisteredWallAtomics()==movement.WallAtomics());
        auto node=*reinterpret_cast<const std::uint32_t*>(list);
        for(std::uint32_t i=0;i<count;++i) {
            CheckSceneArchive(node!=list);
            const auto record=reinterpret_cast<const std::uint32_t*>(node);
            CheckSceneArchive(record[2u] && record[3u]==1u && !record[4u] &&
                record[5u]==1u && !record[6u] && !record[7u]);
            node=record[0u];
        }
        CheckSceneArchive(node==list);
        struct BorrowedClumps {
            RpClump* object{};
            RpClump* event{};
            ~BorrowedClumps() {
                if(object) (void)RpClumpDestroy(object);
                if(event) (void)RpClumpDestroy(event);
            }
        } borrowers{RpClumpClone(movement.Floor()),RpClumpClone(movement.Floor())};
        CheckSceneArchive(borrowers.object && borrowers.event);
        CheckSceneArchive(movement.RegisterObjectClump(borrowers.object,0u) &&
            movement.RegisterEventClump(borrowers.event,0x1234u,true));
        const auto primary=movement.State().words[2u];
        const auto secondary=movement.MutableSecondaryState().words[2u];
        movement.ReleaseComponentResources(movement.PrimaryAddress());
        movement.ReleaseComponentResources(movement.SecondaryAddress());
        movement.ReleaseComponentResources(movement.PrimaryAddress());
        movement.ReleaseComponentResources(movement.SecondaryAddress());
        CheckSceneArchive(!movement.Floor() && !movement.Wall() && !movement.Intri() &&
            !movement.RegisteredFloorAtomics() && !movement.RegisteredWallAtomics() &&
            !movement.RegisteredIntriAtomics() && !movement.RegisteredObjectAtomics() &&
            !movement.RegisteredSecondaryObjectAtomics() && movement.SceneBorrowerCount()==2u &&
            movement.OutstandingAllocations()==8u && movement.State().words[2u]==primary &&
            movement.MutableSecondaryState().words[2u]==secondary);
        CheckSceneArchive(!movement.Load(root,"Q01","N01"));
        CheckSceneArchive(movement.IsObjectClumpRegistered(borrowers.object) &&
            movement.IsEventResourceRegistered(borrowers.event,0x1234u));
        CheckSceneArchive(movement.UnregisterObjectClump(borrowers.object) &&
            movement.UnregisterEventClump(borrowers.event,true) && !movement.SceneBorrowerCount());
        movement.Unload(); movement.Unload();
        CheckSceneArchive(!movement.Intri() && !movement.RegisteredIntriAtomics() &&
            !movement.OutstandingAllocations());
    }
    // Rollback between intri and floor/wall must also retire the owned Clump.
    CheckSceneArchive(movement.LoadIntri(package,member,game));
    CheckSceneArchive(movement.Intri()!=nullptr);
    movement.Unload();
    CheckSceneArchive(!movement.Intri() && !movement.OutstandingAllocations());
    CheckSceneArchive(movement.LoadIntri(package,"N01\\missing_intri.dff",game));
    CheckSceneArchive(!movement.Intri() && !movement.OutstandingAllocations());
    movement.Unload();
    std::cout<<"Native intri loader: original floor fixture, preserved lists, repeat release verified\n";
}
}
