#pragma once
#include "pal4/native_actor_user_data.h"
#include "pal4/native_arena_npc_scene_loader.h"
#include "pal4/native_scene_gob_assets.h"
#include <iostream>
#include <stdexcept>
#include <rwcore.h>
#include <rpworld.h>

namespace pal4::test {
// Observes the original M01 calls without changing objects or script results.
class NativeNpcHandStoryProbe final {
public:
    void Observe(NativeArenaNpcSceneLoader* npcs, NativeSceneGobAssets& objects, RpWorld* world) {
        if (!npcs) return;
        const auto npc=npcs->Manager().FindObject("MN005");
        auto* gob=objects.FindMutableObject("MO005");
        if (!npc || !gob) return;
        const auto* actor=reinterpret_cast<const BaseObjectState*>(npc);
        auto* clump=reinterpret_cast<RpClump*>(*reinterpret_cast<const std::uint32_t*>(gob->Address()+0x14u));
        if (!clump) throw std::runtime_error("M01 hand fixture missing retained MO005 clump");
        const auto hand=actor->Read(0x51cu);
        if (hand) {
            auto* hook=NativeActorUserData::FindAtomic(reinterpret_cast<RpClump*>(actor->Read(0x14u)),"hook_LHWeapon");
            if (hand!=reinterpret_cast<std::uint32_t>(clump) || !hook ||
                RwFrameGetParent(RpClumpGetFrame(clump))!=RpAtomicGetFrame(hook) ||
                RpClumpGetWorld(clump)!=world ||
                *reinterpret_cast<const std::uint32_t*>(gob->Address()+0x184u)!=0u ||
                !actor->Read(0x52cu))
                throw std::runtime_error("M01 MO005 left hand/frame/world/effect ownership mismatch");
            if (!attached_) std::cout<<"Native M01 NPC hand: actual MO005 retained clump -> MN005 LHWeapon, scene GOB hidden, original H_083 effect bound\n";
            attached_=true;
        } else if (attached_ && !detached_) {
            if (RwFrameGetParent(RpClumpGetFrame(clump)) || RpClumpGetWorld(clump) || actor->Read(0x52cu))
                throw std::runtime_error("M01 original detach retained frame/world/effect borrower");
            detached_=true;
            std::cout<<"Native M01 NPC hand: original giGOBDetachFromNPC detaches live borrowed clump, world and hand effect; GOB resource survives\n";
        }
    }
    void RequireCycle() const {
        if (!attached_ || !detached_) throw std::runtime_error("M01 original hand attach/detach cycle not observed");
    }
private:
    bool attached_{},detached_{};
};
}
