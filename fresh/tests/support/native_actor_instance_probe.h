#pragma once

#include "pal4/native_actor_instance_owner.h"
#include "pal4/native_actor_instance_pool.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/camera_resource_native.h"
#include "pal4/camera_render_object.h"
#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <bit>
#include <cmath>
#include <rphanim.h>
#include <rpworld.h>
#include <rtanim.h>

namespace pal4::test {
inline void VerifyNativeActorInstance(const std::filesystem::path& root) {
    // Standalone PALGameIV image provides a real, stable context address.
    std::array<std::uint32_t,128u> game{};
    const auto game_address=reinterpret_cast<std::uint32_t>(game.data());
    auto check=[](bool ok,const char* m) { if (!ok) throw std::runtime_error(m); };
    auto word=[](std::uint32_t a) { return *reinterpret_cast<std::uint32_t*>(a); };
    {
        NativeActorInstanceOwner owner;
        const auto a=owner.Construct(game_address);
        owner.Load(root,"B09");
        // Actual pair99 original Money God model cached radius, independent of
        // the later pose-dependent sphere that InitializeFrame refreshes.
        check(word(a+620u)==0x429435d3u,"B09 cached load radius matches original74.1051254");
        for(unsigned i=0;i<4u;++i)
            check(word(a+608u+4u*i)==word(a+208u+4u*i),"load caches the four actual initial sphere words");
        (void)owner.InitializeFrame();
        check(word(a+620u)==0x429435d3u,"default pose refresh retains cached combat scale radius");
        owner.Destroy();
    }
    for (const auto model : {"B01","B02"}) {
        for (unsigned repeat=0;repeat<3u;++repeat) {
            NativeActorInstanceOwner owner;
            const auto a=owner.Construct(game_address);
            check(a && word(a)==0x844960u && word(a+368u)==0x844954u,
                  "native child uses original ActorInstance vtables");
            check(word(a+624u)==0x84499cu && word(a+1088u)==0x84499cu &&
                  word(a+1032u)==0u && word(a+1496u)==0u &&
                  word(a+1036u+4u)==0u && word(a+1532u+4u)==0u,
                  "combat attachments and all child text fields remain empty");
            check(!owner.OwnedAllocations() && !owner.OwnedTexts(),
                  "child construction needs no simulation allocations");
            owner.Load(root,model);
            const auto* asset=owner.Asset();
            check(asset && asset->Main() && asset->Hierarchy() && asset->ActionCount(),
                  "original cave monster model and all clips loaded");
            check(word(a+20u)==reinterpret_cast<std::uint32_t>(asset->Main()) &&
                  word(a+48u)==reinterpret_cast<std::uint32_t>(asset->Shadow()) &&
                  word(a+372u)==word(a+20u), "native child borrows exact owning asset pointers");
            const auto records=word(a+376u);
            check(records && word(records-4u)==asset->ActionCount(),"all original clips have child records");
            for (std::size_t i=0;i<asset->ActionCount();++i)
                check(word(records+static_cast<std::uint32_t>(i)*68u+32u)==
                      reinterpret_cast<std::uint32_t>(asset->AnimationAt(i)),
                      "child animation records borrow original SDK clips");
            check(owner.OwnedTexts()==1u && owner.OwnedAllocations()>asset->ActionCount(),
                  "child name and animation storage have actual owners");
            // Disturb the SDK frame/materials and stored scales.52AA20 must
            // restore the real clump and clip, not merely update image words.
            const RwV3d displacement{31.0f,41.0f,59.0f};
            (void)RwFrameTranslate(RpClumpGetFrame(asset->Main()),&displacement,rwCOMBINEREPLACE);
            for (const auto offset : {184u,188u,192u})
                *reinterpret_cast<std::uint32_t*>(a+offset)=std::bit_cast<std::uint32_t>(7.0f);
            *reinterpret_cast<std::uint32_t*>(a+172u)=std::bit_cast<std::uint32_t>(2.0f);
            *reinterpret_cast<std::uint32_t*>(a+176u)=std::bit_cast<std::uint32_t>(3.0f);
            *reinterpret_cast<std::uint32_t*>(a+180u)=std::bit_cast<std::uint32_t>(4.0f);
            (void)RpClumpForAllAtomics(asset->Main(),[](RpAtomic* atomic,void*) -> RpAtomic* {
                auto* geometry=RpAtomicGetGeometry(atomic);
                const RwRGBA color{17u,34u,51u,68u};
                for(int i=0;i<RpGeometryGetNumMaterials(geometry);++i)
                    (void)RpMaterialSetColor(RpGeometryGetMaterial(geometry,i),&color);
                return atomic;
            },nullptr);
            check(owner.InitializeFrame()!=0u,"shared52AA20 native initialization succeeds");
            check(_stricmp(asset->CurrentAction().c_str(),"z01")==0,
                  "native child selects original576CF0 default animation");
            const auto selected=word(a+380u);
            check(selected && word(selected+32u)==reinterpret_cast<std::uint32_t>(
                      asset->Hierarchy()->currentAnim->pCurrentAnim),
                  "shared animation record and actual SDK interpolator agree");
            bool colors_ok=true;
            (void)RpClumpForAllAtomics(asset->Main(),[](RpAtomic* atomic,void* data) -> RpAtomic* {
                auto& ok=*static_cast<bool*>(data);
                auto* geometry=RpAtomicGetGeometry(atomic);
                for(int i=0;i<RpGeometryGetNumMaterials(geometry);++i) {
                    const auto* c=RpMaterialGetColor(RpGeometryGetMaterial(geometry,i));
                    ok=ok && c->red==255u && c->green==255u && c->blue==255u && c->alpha==255u;
                }
                return atomic;
            },&colors_ok);
            const auto* transform=RwFrameGetLTM(RpClumpGetFrame(asset->Main()));
            check(colors_ok && std::fabs(transform->pos.x-2.0f)<0.001f &&
                  std::fabs(transform->pos.y-3.0f)<0.001f &&
                  std::fabs(transform->pos.z-4.0f)<0.001f &&
                  word(a+184u)==0x3f800000u && word(a+188u)==0x3f800000u &&
                  word(a+192u)==0x3f800000u && std::bit_cast<float>(word(a+220u))>0.0f,
                  "GetFrame restores native material RGBA, transform, scale and sphere");
            auto& camera=*reinterpret_cast<CameraRenderState*>(a+56u);
            NativeCameraResourceRuntime cameras;
            camera.camera=CameraResourceHelpers::Create(16u,cameras);
            check(camera.camera!=0u,"child has a real SDK camera to retire");
            camera.active=1u;
            auto* raster=RwRasterCreate(16,16,32,rwRASTERTYPECAMERATEXTURE|rwRASTERFORMAT8888);
            check(raster!=nullptr,"child camera owns a color raster");
            camera.primary_raster=reinterpret_cast<std::uint32_t>(raster);
            RwCameraSetRaster(reinterpret_cast<RwCamera*>(camera.camera),raster);
            auto* observer=RwTextureCreate(raster);
            check(observer!=nullptr,"child camera texture");
            RwTextureAddRef(observer);
            camera.texture=reinterpret_cast<std::uint32_t>(observer);
            const auto shadow_before=word(a+48u),light_before=word(a+52u);
            if (!shadow_before || !light_before) {
                owner.DisableCamera();
                check(camera.camera && camera.texture,
                      "6664A0 disabled camera guard preserves resources without shadow/light");
            }
            *reinterpret_cast<std::uint8_t*>(a+624u+460u)=1u;
            *reinterpret_cast<std::uint8_t*>(a+1088u+460u)=1u;
            // Check preflight across both hands before retiring real resources.
            *reinterpret_cast<std::uint32_t*>(a+624u+408u)=game_address;
            bool effects_rejected=false;
            try { owner.Deactivate(); } catch(const std::logic_error&) { effects_rejected=true; }
            check(effects_rejected && camera.camera && camera.texture &&
                  *reinterpret_cast<std::uint8_t*>(a+1088u+460u)==1u,
                  "unbound left effects reject before camera/right-hand mutation");
            *reinterpret_cast<std::uint32_t*>(a+624u+408u)=0u;
            auto* shadow=RpClumpCreate();
            auto* light=RpLightCreate(rpLIGHTAMBIENT);
            check(shadow && light,"real shadow/light handles for guarded camera disable");
            *reinterpret_cast<std::uint32_t*>(a+48u)=reinterpret_cast<std::uint32_t>(shadow);
            *reinterpret_cast<std::uint32_t*>(a+52u)=reinterpret_cast<std::uint32_t>(light);
            owner.DisableCamera();
            check(!camera.camera && !camera.texture && !camera.primary_raster &&
                  !RwTextureGetRaster(observer) && observer->refCount==1,
                  "6664A0 enabled0 calls actual SDK camera cleanup when both handles exist");
            *reinterpret_cast<std::uint32_t*>(a+48u)=shadow_before;
            *reinterpret_cast<std::uint32_t*>(a+52u)=light_before;
            RpLightDestroy(light); RpClumpDestroy(shadow);
            *reinterpret_cast<std::uint32_t*>(a+492u)=game_address;
            *reinterpret_cast<std::uint32_t*>(a+516u)=game_address;
            owner.Deactivate(); owner.Deactivate();
            check(!camera.camera && !camera.texture && !camera.primary_raster && !camera.active &&
                  !RwTextureGetRaster(observer) && observer->refCount==1 &&
                  !*reinterpret_cast<std::uint8_t*>(a+624u+460u) &&
                  !*reinterpret_cast<std::uint8_t*>(a+1088u+460u) &&
                  word(a+492u)==game_address && word(a+516u)==game_address &&
                  owner.Asset()->Main() && word(a+380u)==selected,
                  "52AE70 retires camera/hands, preserves parent links and loaded model/clip");
            RwTextureDestroy(observer);
            *reinterpret_cast<std::uint32_t*>(a+492u)=0u;
            *reinterpret_cast<std::uint32_t*>(a+516u)=0u;
            // Unsupported owner linkage rejects before shared destruction mutates.
            *reinterpret_cast<std::uint32_t*>(a+492u)=game_address;
            bool rejected=false;
            try { owner.Destroy(); } catch(const std::logic_error&) { rejected=true; }
            check(rejected && owner.Asset()->Main() && word(a)==0x844960u,
                  "attached child rejected without altering its SDK graph");
            *reinterpret_cast<std::uint32_t*>(a+492u)=0u;
            std::cout << "Combat child " << model << " actions=" << asset->ActionCount()
                      << " shadow=" << bool(asset->Shadow()) << " repeat=" << repeat
                      << " frame_action=" << asset->CurrentAction() << '\n';
            owner.Destroy(); owner.Destroy();
            check(!owner.Address() && !owner.Asset() && !owner.OwnedAllocations() && !owner.OwnedTexts(),
                  "shared child destruction retires graph and every native allocation");
        }
    }
    NativeActorInstanceOwner failed;
    failed.Construct(game_address);
    bool rejected=false;
    try { failed.Load(root,"missing_combat_actor"); }
    catch(const std::runtime_error&) { rejected=true; }
    check(rejected && !failed.Asset()->Main(),"missing original model fails without partial SDK graph");
    failed.Destroy();

    bool inactive=true;
    unsigned creations=0u;
    NativeActorInstancePool pool([&] { return inactive; },
        [&](std::string_view name) {
            ++creations;
            auto owner=std::make_unique<NativeActorInstanceOwner>();
            owner->Construct(game_address);
            owner->Load(root,name);
            return owner;
        },2u);
    check(!pool.TakeOrCreate("missing_combat_actor") && creations==0u,
          "inactive Combat returns before cache access or model loading");
    inactive=false;
    auto first=pool.TakeOrCreate("B01");
    auto second=pool.TakeOrCreate("B02");
    auto third=pool.TakeOrCreate("B02");
    check(first && second && third && second->Address()!=third->Address() && creations==3u,
          "simultaneously checked-out identical models have independent owners");
    const auto second_address=second->Address();
    const auto third_address=third->Address();
    check(second->BindParent(game_address)==game_address &&
          word(second_address+492u)==game_address && word(second_address+516u)==game_address,
          "shared52A9C0 binds both child parent fields");
    bool active_destroy_rejected=false;
    try { second->Destroy(); } catch(const std::logic_error&) { active_destroy_rejected=true; }
    check(active_destroy_rejected && second->Asset()->Main(),
          "active parent association blocks destruction before SDK mutation");
    pool.Return(first); pool.Return(second);
    check(!first && !second && pool.CachedCount()==2u,"return transfers ownership to cache");
    const auto effect_owner=third->Address()+624u+408u;
    *reinterpret_cast<std::uint32_t*>(effect_owner)=game_address;
    bool rejected_return=false;
    try { pool.Return(third); } catch(const std::logic_error&) { rejected_return=true; }
    check(rejected_return && third && pool.CachedCount()==2u,
          "failed deactivation retains incoming ownership and avoids eviction");
    *reinterpret_cast<std::uint32_t*>(effect_owner)=0u;
    pool.Return(third); // Full cache evicts B01, retaining the two B02 entries.
    inactive=true;
    check(!pool.TakeOrCreate("B02") && pool.CachedCount()==2u && creations==3u,
          "inactive guard also preserves populated cache");
    inactive=false;
    second=pool.TakeOrCreate("B02"); third=pool.TakeOrCreate("B02");
    check(second->Address()==second_address && third->Address()==third_address &&
          creations==3u && pool.CachedCount()==0u,
          "first matching cached owner extracted in order without SDK reload");
    check(word(second_address+492u)==game_address && word(second_address+516u)==game_address &&
          second->InitializeFrame()!=0u,
          "returned model retains old parent fields but permits frame initialization");
    const auto next_parent=game_address+4u;
    check(second->BindParent(next_parent)==next_parent &&
          word(second_address+492u)==next_parent && word(second_address+516u)==next_parent,
          "cached child can be explicitly reassociated with a new parent");
    first=pool.TakeOrCreate("B01");
    check(first && creations==4u,"evicted oldest model requires a new original-resource load");
    first->Destroy(); first.reset();
    pool.Return(second); pool.Return(third); pool.Return(first);
    check(pool.CachedCount()==2u,"null return has no effect");
    const auto before_failure=creations;
    bool load_failed=false;
    try { (void)pool.TakeOrCreate("missing_combat_actor"); }
    catch(const std::runtime_error&) { load_failed=true; }
    check(load_failed && creations==before_failure+1u && pool.CachedCount()==2u,
          "failed factory load preserves every cached owner");
    pool.Clear(); pool.Clear();
    check(pool.CachedCount()==0u,"explicit pool clear destroys all SDK owners and is repeatable");
    std::cout << "Combat model pool inactive/reuse/independence/eviction/rollback verified\n";
}
}
