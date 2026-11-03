#pragma once
#include "pal4/native_scene_gob_assets.h"
#include "pal4/native_ui_world_camera.h"
#include "pal4/native_world_asset.h"
#include "pal4/native_game_arena_manager_owner.h"
#include "pal4/native_game_world_scene_pass.h"
#include "pal4/native_gob_closest_selection.h"
#include "pal4/native_actor_user_data.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include <rwcore.h>
#include <rpworld.h>
#include <d3d9.h>
#include <algorithm>
#include <bit>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeMissingSceneGob(const std::filesystem::path& root,
    IGameResourceManagerRuntime& transforms, NativeBaseObjectMemory& memory,
    IRwDictSchemaRuntime& schema, NativePkFileManagerRuntime& packages,
    INativeGobEffectService& effects, NativePalGameIVTextureStorage& game) {
    const auto before = memory.OutstandingAllocations();
    {
        NativeSceneGobAssets objects;
        if (!objects.Load(root,"Q04","CN05",transforms,memory,schema,packages,effects) ||
            objects.Size()!=0u || objects.SceneMemberMissing() || !objects.NameTreeAddress())
            throw std::runtime_error("original CN05 creates a normal empty GOB manager");
        const auto manager=objects.NameTreeAddress();
        if (!objects.Load(root,"Q04","CN05C",transforms,memory,schema,packages,effects) ||
            objects.Size()!=0u || !objects.SceneMemberMissing() || objects.NameTreeAddress()!=manager ||
            memory.ReadDword(manager+12u)!=0u)
            throw std::runtime_error("missing CN05C GOB retains the empty creating manager without fallback");
        if (objects.Load(root,"../Q04","CN05C",transforms,memory,schema,packages,effects) ||
            objects.NameTreeAddress()!=manager)
            throw std::runtime_error("invalid GOB load must fail and preserve the published manager");
        objects.CleanupScene(game);
    }
    if (memory.OutstandingAllocations()!=before)
        throw std::runtime_error("empty/missing GOB managers release through their creating owner");
    std::cout << "Native optional GOB: CN05 empty, CN05C missing, invalid rejected and normal cleanup pass\n";
}

inline void VerifyNativeCaveSwordHooks(NativeSceneGobAssets& objects) {
    for (const auto name : {"MO001", "MO002"}) {
        const auto* object = objects.FindObject(name);
        const auto* record = objects.FindRecord(name);
        if (!object || !record || record->model != "JQ02")
            throw std::runtime_error("original cave owns both configured JQ02 sword objects");
        const auto clump = *reinterpret_cast<const std::uint32_t*>(object->Address()+20u);
        if (NativeActorUserData::HiddenHookCount(reinterpret_cast<RpClump*>(clump)) != 4u)
            throw std::runtime_error("original JQ02 sword hides all four auxiliary hook atomics");
    }
    std::cout << "Cave sword hooks: both actual JQ02 models retain four hidden helpers\n";
}
inline void VerifyNativeGobInteractionRange(NativeSceneGobAssets& objects,
                                           NativeBaseObjectMemory& memory) {
    const auto* bow = objects.FindObject("MO002");
    const auto* record = objects.FindRecord("MO002");
    if (!bow || !record || record->script_entry != "func2002")
        throw std::runtime_error("return-for-bow owns configured original MO002/func2002");
    const auto object = bow->Address();
    const auto read = [&](unsigned offset) {
        return std::bit_cast<float>(memory.ReadDword(object + offset));
    };
    const float range = read(0x19cu);
    if (range != 60.0f || memory.ReadDword(object + 0x1a0u) != 1u)
        throw std::runtime_error("original N01 bow range/height fixture");
    GameObjectPoint position{read(0xacu) + 30.0f, read(0xb0u), read(0xb4u) + 40.0f};
    NativeGobClosestSelection selection(objects, memory);
    const auto address = reinterpret_cast<std::uint32_t>(&position);
    if (!selection.Enabled(object))
        throw std::runtime_error("return-for-bow script enables original research object");
    if (!selection.InRange(object, address, 40.0f))
        throw std::runtime_error("original bow accepts planar distance50 inside range60");
    if (!selection.FindPoint(position, 40.0f, false))
        throw std::runtime_error("coordinate research selection finds an actual nearby bow GOB");
    position.x = read(0xacu) + 70.0f;
    position.z = read(0xb4u);
    if (selection.InRange(object, address, 40.0f))
        throw std::runtime_error("original bow rejects distance70 outside range60");
    position.x = read(0xacu);
    position.y = read(0xb0u) - 40.0f;
    if (selection.InRange(object, address, 40.0f))
        throw std::runtime_error("GOB upper height endpoint remains strict");
    std::cout << "Original bow interaction: distance50 accepted,70 rejected, strict height endpoint\n";
}
inline void VerifyNativeLoadedGobEffects(NativeSceneGobAssets& objects,
                                        INativeGobEffectService& effects) {
    unsigned visible_effects{};
    for (const auto address : objects.ObjectAddresses()) {
        const auto* object = objects.FindMutableObjectByAddress(address);
        if (!object || object->Type() != 8u) continue;
        const auto word = [address](unsigned offset) {
            return *reinterpret_cast<const std::uint32_t*>(address + offset);
        };
        const auto* record = objects.FindRecord(
            reinterpret_cast<const char*>(word(4u)));
        if (!record || word(0x184u) != record->flags[0])
            throw std::runtime_error("loaded effect retains original record visibility");
        if (word(0x184u)) {
            if (!effects.IsActive(word(0x1dcu), word(0x1e0u)))
                throw std::runtime_error("visible loaded GOB creates its actual effect instance");
            ++visible_effects;
        } else if(word(0x1dcu)!=0xffffffffu || word(0x1e0u)!=0u) {
            throw std::runtime_error("hidden loaded GOB retains the original empty effect handle");
        }
    }
    std::cout << "Loaded GOB visibility: initial live effect instances="
              << visible_effects << '\n';
}
inline void VerifyNativeSceneGobRender(NativeSceneGobAssets& objects,NativeGameArenaManagerOwner& arena,
    NativeWorldAsset& world,const std::function<void()>& restore_states){
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    const auto word=[](std::uint32_t a)->std::uint32_t&{return *reinterpret_cast<std::uint32_t*>(a);};
    const auto addresses=objects.ObjectAddresses();check(addresses.size()>=2u,"original scene GOB render requires populated manager");
    NativeGobObjectImage* target{};
    for(auto a:addresses){auto* object=objects.FindMutableObjectByAddress(a);if(object && object->Type()!=3u && object->Type()!=8u && word(a+20u)){
        if(!target || object->Type()==0u)target=object;if(object->Type()==0u)break;}}
    check(target,"original scene has a renderable non-effect GOB");const auto selected=target->Address();
    struct Flags{std::uint32_t a,visible,marker,drawn;};std::vector<Flags> flags;
    for(auto a:addresses){flags.push_back({a,word(a+0x184u),word(a+0x1c4u),word(a+28u)});word(a+0x184u)=0u;word(a+0x1c4u)=0u;}
    struct Restore{std::vector<Flags>& flags;~Restore(){for(auto f:flags){*reinterpret_cast<std::uint32_t*>(f.a+0x184u)=f.visible;*reinterpret_cast<std::uint32_t*>(f.a+0x1c4u)=f.marker;*reinterpret_cast<std::uint32_t*>(f.a+28u)=f.drawn;}}} restore{flags};
    word(selected+0x184u)=1u;
    const auto original_mode=word(selected+24u);
    struct Mode{std::uint32_t& value;std::uint32_t before;~Mode(){value=before;}} mode_restore{word(selected+24u),original_mode};
    check(original_mode==0u && word(selected+36u)==1u,"original mode0 GOB binding completes actual resource finalization");

    RwSphere sphere{};bool found{};
    struct Sphere{RwSphere& value;bool& found;} sphere_context{sphere,found};
    RpClumpForAllAtomics(reinterpret_cast<RpClump*>(word(selected+20u)),[](RpAtomic* atomic,void* raw)->RpAtomic*{
        auto& c=*static_cast<Sphere*>(raw);if(!c.found){c.value=*RpAtomicGetWorldBoundingSphere(atomic);c.found=true;}return atomic;
    },&sphere_context);check(found,"original GOB has actual SDK atomic bounds");
    NativeUiWorldCamera camera;check(camera.Open(world.World(),320u,240u),"original GOB offscreen camera");
    const float radius=std::max(1.f,sphere.radius);const RwV3d position{sphere.center.x,sphere.center.y,sphere.center.z-4.f*radius};
    RwMatrixSetIdentity(RwFrameGetMatrix(RwCameraGetFrame(camera.Camera())));
    check(RwFrameTranslate(RwCameraGetFrame(camera.Camera()),&position,rwCOMBINEREPLACE),"GOB probe actual camera pose");
    RwCameraSetNearClipPlane(camera.Camera(),radius*0.01f);RwCameraSetFarClipPlane(camera.Camera(),radius*30.f);
    const auto context=reinterpret_cast<std::uint32_t>(camera.Camera());
    NativeBaseObjectMemory memory;
    {NativePalGameObjectManagerOwner manager(memory);const auto second=addresses.front()==selected?addresses[1]:addresses.front();
        word(second+0x184u)=1u;check(manager.InsertObject("z",selected) && manager.InsertObject("a",second),"borrowed original objects in real ordered manager");
        unsigned mutations{},rejections{};bool failed{};
        try{(void)manager.RenderEnabled(context,[&](auto,auto){
            for(unsigned which=0;which<3u;++which){bool rejected{};try{
                if(which==0u)(void)manager.InsertObject("foreign",selected);
                if(which==1u)manager.CleanupScene([&](auto){++mutations;},[&](auto,auto){++mutations;});
                if(which==2u)(void)manager.RenderEnabled(context,[](auto,auto){});
            }catch(const std::logic_error&){rejected=true;}check(rejected,"manager mutation/reentry rejects during GOB render");++rejections;}
            throw std::runtime_error("intentional GOB render callback failure");});
        }catch(const std::runtime_error& e){failed=std::string_view(e.what())=="intentional GOB render callback failure";}
        std::vector<std::uint32_t> visited;
        check(failed && !mutations && rejections==3u && manager.RenderEnabled(context,[&](auto a,auto c){check(c==context,"GOB camera forwarded");visited.push_back(a);})==2u && visited==std::vector<std::uint32_t>{second,selected},"actual GOB ordered traversal retries after exception");
        word(second+0x184u)=0u;
    }check(!memory.OutstandingAllocations(),"borrowed GOB manager fixture retires only its nodes");
    {auto& clump=word(selected+20u);const auto original=clump;struct Clump{std::uint32_t& c;std::uint32_t old;~Clump(){c=old;}} reset{clump,original};clump^=4u;
        bool rejected{};try{(void)objects.RenderFrame(context);}catch(const std::logic_error&){rejected=true;}check(rejected,"foreign GOB clump rejects before SDK draw");}
    const auto capture=[&](unsigned mode){
        RwRGBA black{0,0,0,255};check(RwCameraClear(camera.Camera(),&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ) && RwCameraBeginUpdate(camera.Camera()),"GOB camera begin");
        struct End{RwCamera* camera;~End(){RwCameraEndUpdate(camera);}} end{camera.Camera()};restore_states();
        if(mode==1u)check(objects.RenderFrame(context)==(word(selected+0x184u)?1u:0u),"original manager184 gate selects exact GOB count");
        if(mode==2u)target->Render(context);
        NativeGameWorldScenePassRuntime pass([&](auto,auto){arena.RenderSequence(context);});(void)GameWorldScenePass::Process(arena.Address(),context,pass);
        auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());IDirect3DSurface9* target_surface{};IDirect3DSurface9* staging{};
        struct Surface{IDirect3DSurface9*& p;~Surface(){if(p)p->Release();}} target_owner{target_surface},staging_owner{staging};
        check(device && SUCCEEDED(device->GetRenderTarget(0,&target_surface)),"GOB GPU target");D3DSURFACE_DESC desc{};check(SUCCEEDED(target_surface->GetDesc(&desc)),"GOB GPU description");
        check(SUCCEEDED(device->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&staging,nullptr)) && SUCCEEDED(device->GetRenderTargetData(target_surface,staging)),"GOB GPU readback");
        D3DLOCKED_RECT lock{};check(SUCCEEDED(staging->LockRect(&lock,nullptr,D3DLOCK_READONLY)),"GOB GPU lock");std::vector<std::uint32_t> pixels(desc.Width*desc.Height);
        for(unsigned y=0;y<desc.Height;++y)std::memcpy(pixels.data()+y*desc.Width,static_cast<const char*>(lock.pBits)+y*lock.Pitch,desc.Width*4u);staging->UnlockRect();return pixels;
    };
    const auto baseline=capture(0u);
    check(capture(1u)==baseline,"original resource mode0 does not actively submit Clump");
    word(selected+24u)=1u; // Controlled direct-submission branch; production remains mode0.
    const auto rendered=capture(1u);unsigned differences{};for(std::size_t i=0;i<baseline.size();++i)differences+=(baseline[i]&0xffffffu)!=(rendered[i]&0xffffffu);
    std::cout<<"GOB GPU operands: type="<<target->Type()<<" loaded="<<word(selected+36u)<<" mode="<<word(selected+24u)<<" cull="<<word(selected+28u)<<" radius="<<sphere.radius<<" differences="<<differences<<"\n";
    check(differences>20u,"original typed GOB changes actual scene GPU output");
    word(selected+0x184u)=0u;word(selected+0x1c4u)=1u;
    check(capture(1u)==baseline,"manager disabled gate precedes object marker fallback");
    check(capture(2u)==rendered,"direct original object marker fallback renders same clump");
    word(selected+0x1c4u)=0u;check(capture(2u)==baseline,"hidden object and marker gates suppress actual drawing");
    std::cout<<"Native scene GOB render: objects="<<addresses.size()<<" selected_type="<<target->Type()<<" GPU_changed="<<differences<<" manager184/object184-or1C4 gates, actual ordered tree, owner failure/retry and node cleanup pass\n";
}
}
