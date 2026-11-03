#pragma once
#include "pal4/native_actor_animation_binding.h"
#include "pal4/native_actor_instance_owner.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_gob_clump_asset.h"
#include "pal4/native_actor_user_data.h"
#include <rwcore.h>
#include <rpworld.h>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeActorAnimationBinding(const std::filesystem::path& root) {
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    NativeAnimatedActorAsset actor;check(actor.Load(root,"103"),"binding original actor103");
    NativeGobClumpAsset weapon;check(weapon.Load(root,"gamedata\\PALWeapon\\WTA\\","WTA"),"binding original WTA");
    auto* hook=NativeActorUserData::FindAtomic(weapon.Clump(),"hook_hand");
    check(hook && RpAtomicGetClump(hook)==weapon.Clump(),"original WTA hook_hand");
    auto* source=RpAtomicGetFrame(hook);auto* frame=RpClumpGetFrame(weapon.Clump());
    std::array<std::uint32_t,128> game{};NativeActorInstanceOwner image_owner;
    const auto a=image_owner.Construct(address(game.data()));
    auto view=std::span<std::uint32_t,116>(reinterpret_cast<std::uint32_t*>(a+624u),116u);
    view[5]=address(weapon.Clump());
    struct Cleanup {
        RpClump* actor;RwFrame* frame;std::uint32_t* clump_field;const char* hook="hook_LHWeapon";
        void Detach(){NativeActorUserData::RemoveFrame(actor,hook,frame);}
        ~Cleanup(){Detach();if(clump_field)*clump_field=0u;}
    } cleanup{actor.Main(),frame,&view[5]};
    unsigned passes{};
    for(const auto* name:{"hook_LHWeapon","hook_RHWeapon"})for(unsigned mode=0;mode<2u;++mode) {
        cleanup.hook=name;
        auto* target=NativeActorUserData::FindAtomic(actor.Main(),name);
        check(target && RpAtomicGetFrame(target),"original actor weapon attachment hook");
        // A nonzero translation distinguishes inversion from copying. Include
        // an independent hook rotation so the inverse must cover the full matrix.
        const RwV3d shift{3,5,7},axis{0,1,0};
        check(RwFrameRotate(source,&axis,27.0f,rwCOMBINEREPLACE) &&
              RwFrameTranslate(source,&shift,rwCOMBINEPOSTCONCAT),"controlled nonidentity weapon hook transform");
        check(NativeActorAnimationBinding::Update(view,*weapon.Clump(),*actor.Main(),name,mode)==address(weapon.Clump()),
              "shared update returns actual hidden-hook clump");
        check(RwFrameGetParent(frame)==RpAtomicGetFrame(target),"weapon frame bound to actual parent hook");
        RwMatrix inverse_check=*RwFrameGetMatrix(frame);
        const bool left=std::string_view(name)=="hook_LHWeapon";
        const RwV3d x{1,0,0},z{0,0,1};
        if(!left)check(RwMatrixRotate(&inverse_check,&z,-180.0f,rwCOMBINEPOSTCONCAT)!=nullptr,"undo secondary orientation");
        check(RwMatrixRotate(&inverse_check,&x,left && mode==1u?90.0f:-90.0f,rwCOMBINEPOSTCONCAT)!=nullptr,"undo primary orientation");
        RwMatrix product;
        check(RwMatrixMultiply(&product,&inverse_check,RwFrameGetMatrix(source))!=nullptr,"inverse/forward matrix product");
        const auto nearly_equal=[](float x,float y){return std::abs(x-y)<0.0002f;};
        check(nearly_equal(product.right.x,1) && nearly_equal(product.right.y,0) && nearly_equal(product.right.z,0) &&
              nearly_equal(product.up.x,0) && nearly_equal(product.up.y,1) && nearly_equal(product.up.z,0) &&
              nearly_equal(product.at.x,0) && nearly_equal(product.at.y,0) && nearly_equal(product.at.z,1) &&
              nearly_equal(product.pos.x,0) && nearly_equal(product.pos.y,0) && nearly_equal(product.pos.z,0),
              "weapon bind uses matrix inverse, not a copied forward transform");
        check(NativeActorUserData::HiddenHookCount(weapon.Clump())>0u,"original weapon helper atomics hidden");
        cleanup.Detach();check(!RwFrameGetParent(frame),"detach clears actual parent before next binding");++passes;
    }
    const auto prior=*RwFrameGetMatrix(frame);view[5]=0u;
    bool rejected=false;try{(void)NativeActorAnimationBinding::Update(view,*weapon.Clump(),*actor.Main(),"hook_LHWeapon",1u);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected && !std::memcmp(&prior,RwFrameGetMatrix(frame),sizeof(prior)),"foreign component rejects before SDK mutation");
    image_owner.Destroy();cleanup.clump_field=nullptr;
    check(!image_owner.OwnedAllocations() && !image_owner.OwnedTexts() && passes==4u,"binding fixture explicit ownership retirement");
    std::cout<<"Native52A340 animation binding: original103/WTA four hand/mode branches, nonidentity inverse invariant, real SDK orientation/parent hook/hide/detach and foreign receiver rejection pass\n";
}
}
