#include "renderware_camera_cases.h"
#include "pal4/camera_resource_native.h"
#include "pal4/camera_render_geometry_native.h"
#include "pal4/camera_render_lifecycle_native.h"
#include <rpworld.h>
#include <rpusrdat.h>
#include "pal4/native_hook_clump_names.h"
#include "pal4/native_shadow_atomic_flags.h"
#include "pal4/native_pal4_camera_factory.h"
#include <rwcore.h>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace pal4::test {
namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void Near(float value, float expected, const char* message) { Require(std::fabs(value-expected) < 0.0002f, message); }
int constructed{}, destroyed{}, plugin_offset{};
void* Construct(void* camera, RwInt32 offset, RwInt32 size) {
    Require(offset == plugin_offset && size == 4, "camera plugin layout mismatch");
    *reinterpret_cast<int*>(static_cast<char*>(camera)+offset) = 0x5678; ++constructed; return camera;
}
void* Destruct(void* camera, RwInt32 offset, RwInt32) {
    Require(*reinterpret_cast<int*>(static_cast<char*>(camera)+offset) == 0x5678, "camera plugin storage corrupt");
    ++destroyed; return camera;
}
struct Camera {
    RwCamera* camera{};
    RwFrame* frame{};
    RwRaster* raster{};
    explicit Camera(bool pal4_variant = false) {
        camera = pal4_variant ? CreateNativePal4Camera() : RwCameraCreate(); Require(camera != nullptr, "camera create failed");
        frame = RwFrameCreate(); Require(frame != nullptr, "frame create failed");
        RwCameraSetFrame(camera, frame);
    }
    void RenderTarget() {
        raster = RwRasterCreate(64,64,32,rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);
        Require(raster != nullptr, "camera raster create failed"); RwCameraSetRaster(camera,raster);
    }
    ~Camera() {
        if(camera) { RwCameraSetRaster(camera,nullptr); RwCameraSetFrame(camera,nullptr); RwCameraDestroy(camera); }
        if(frame) RwFrameDestroy(frame);
        if(raster) RwRasterDestroy(raster);
    }
};
void Lifetime() {
    auto* camera = RwCameraCreate(); Require(camera != nullptr, "camera create failed");
    Require(RwCameraGetFrame(camera) == nullptr && !camera->frameBuffer && !camera->zBuffer, "new camera attachment state");
    Require(camera->projectionType == rwPERSPECTIVE && camera->beginUpdate && camera->endUpdate, "new camera dispatch/projection");
    Near(camera->nearPlane,0.05f,"default near"); Near(camera->farPlane,10.f,"default far"); Near(camera->fogPlane,5.f,"default fog");
    Near(camera->viewWindow.x,1.f,"default window"); Near(camera->recipViewWindow.y,1.f,"default reciprocal");
    auto* frame = RwFrameCreate(); auto* raster = RwRasterCreate(8,8,32,rwRASTERTYPENORMAL);
    Require(frame && raster,"borrowed resources create");
    RwCameraSetFrame(camera,frame); RwCameraSetRaster(camera,raster);
    Require(RwCameraDestroy(camera),"camera destroy result");
    // SDK camera destruction detaches, but the caller owns both resources.
    Require(RwFrameGetMatrix(frame) != nullptr && RwRasterGetWidth(raster)==8,"camera freed borrowed resource");
    Require(RwFrameDestroy(frame) && RwRasterDestroy(raster),"borrowed resource cleanup");
}
void CheckSphere(RwCamera* camera, RwV3d center, float radius, RwFrustumTestResult expected) {
    RwSphere sphere{center,radius}; Require(RwCameraFrustumTestSphere(camera,&sphere)==expected,"frustum sphere classification");
}
void Projection() {
    Camera owner; owner.RenderTarget(); auto* camera=owner.camera;
    RwCameraSetNearClipPlane(camera,1); RwCameraSetFarClipPlane(camera,10);
    const RwV2d view{1,1}; RwCameraSetViewWindow(camera,&view);
    const RwRGBA color{17,34,51,255};
    Require(RwCameraClear(camera,const_cast<RwRGBA*>(&color),rwCAMERACLEARIMAGE)==camera,"camera clear failed");
    auto* image=RwImageCreate(64,64,32);
    Require(image && RwImageAllocatePixels(image) && RwImageSetFromRaster(image,owner.raster),"clear readback failed");
    const auto pixel=*reinterpret_cast<const RwRGBA*>(RwImageGetPixels(image));
    const bool clear_matches=pixel.red==17 && pixel.green==34 && pixel.blue==51;
    RwImageDestroy(image); Require(clear_matches,"clear did not reach actual raster");
    for (auto projection : {rwPERSPECTIVE,rwPARALLEL}) {
        Require(RwCameraSetProjection(camera,projection)==camera,"valid projection rejected");
        Require(RwCameraBeginUpdate(camera)==camera,"camera begin failed");
        Require(RwCameraGetCurrentCamera()==camera,"current camera not set");
        CheckSphere(camera,{0,0,5},0.1f,rwSPHEREINSIDE);
        CheckSphere(camera,{0,0,-2},0.1f,rwSPHEREOUTSIDE);
        CheckSphere(camera,{0,0,1},0.25f,rwSPHEREBOUNDARY);
        CheckSphere(camera,{0,0,1.25f},0.25f,rwSPHEREINSIDE);
        CheckSphere(camera,{0,0,10.25f},0.25f,rwSPHEREBOUNDARY);
        for(int i=0;i<8;++i) {
            const auto p=camera->frustumCorners[i]; const float z=i<4?1.f:10.f;
            Near(p.z,z,"frustum corner depth");
            Near(std::fabs(p.x),projection==rwPERSPECTIVE?z:1.f,"frustum corner width");
            Near(std::fabs(p.y),projection==rwPERSPECTIVE?z:1.f,"frustum corner height");
        }
        Near(camera->frustumBoundBox.inf.z,1,"frustum bbox near"); Near(camera->frustumBoundBox.sup.z,10,"frustum bbox far");
        // D3D9's [0,1] depth interval is inset by 0.0001 at each endpoint.
        const float near_value=projection==rwPERSPECTIVE?1.f/camera->nearPlane:camera->nearPlane;
        const float far_value=projection==rwPERSPECTIVE?1.f/camera->farPlane:camera->farPlane;
        Near(camera->zScale*near_value+camera->zShift,0.0001f,"near depth mapping");
        Near(camera->zScale*far_value+camera->zShift,0.9999f,"far depth mapping");
        Require(RwCameraEndUpdate(camera)==camera && RwCameraGetCurrentCamera()==nullptr,"camera end/current mismatch");
    }
    const auto previous=camera->projectionType;
    Require(!RwCameraSetProjection(camera,static_cast<RwCameraProjection>(0)) && camera->projectionType==previous,"invalid projection changed camera");
    RwV2d window{2,4}; RwCameraSetViewWindow(camera,&window);
    Near(camera->recipViewWindow.x,0.5f,"reciprocal x"); Near(camera->recipViewWindow.y,0.25f,"reciprocal y");
    const RwV3d movement{3,4,5}; RwFrameTranslate(owner.frame,&movement,rwCOMBINEREPLACE);
    const RwV2d offset{0.2f,0.1f}; RwCameraSetViewOffset(camera,&offset);
    Require(RwCameraBeginUpdate(camera)==camera,"offset begin");
    Near(camera->frustumCorners[0].z,6,"frame dirty sync");
    Near(camera->frustumCorners[4].x,3+2+9*0.2f,"parallel view offset");
    Require(RwCameraEndUpdate(camera)==camera,"offset end");
}
RwCamera* received{};
RwCamera* Begin(RwCamera* camera) { received=camera; return nullptr; }
RwCamera* End(RwCamera* camera) { received=camera; return camera; }
void Dispatch() {
    Camera owner; auto* camera=owner.camera;
    camera->beginUpdate=Begin; camera->endUpdate=End;
    received=nullptr; Require(!RwCameraBeginUpdate(camera) && received==camera,"begin must pass camera argument and preserve null result");
    received=nullptr; Require(RwCameraEndUpdate(camera)==camera && received==camera,"end must dispatch +28 and pass camera");
}
unsigned failed_end_calls{};
RwBool FailDeviceEnd(void* first, void* camera, RwInt32 value) {
    Require(!first && camera==received && value==0,"PAL4 end standard receives original arguments");
    ++failed_end_calls;return FALSE;
}
void Pal4Variant() {
    Camera owner(true);owner.RenderTarget();auto* camera=owner.camera;
    const RwRGBA clear{17,34,51,255};
    Require(RwCameraClear(camera,const_cast<RwRGBA*>(&clear),rwCAMERACLEARIMAGE)==camera &&
            RwCameraBeginUpdate(camera)==camera,"PAL4 camera actual clear/begin");
    auto* globals=static_cast<RwGlobals*>(RwEngineInstance);
    const auto prior=globals->stdFunc[rwSTANDARDCAMERAENDUPDATE];
    globals->stdFunc[rwSTANDARDCAMERAENDUPDATE]=FailDeviceEnd;
    received=camera;failed_end_calls=0;
    const auto failed=RwCameraEndUpdate(camera);
    globals->stdFunc[rwSTANDARDCAMERAENDUPDATE]=prior;
    Require(!failed && failed_end_calls==1 && RwCameraGetCurrentCamera()==camera,
            "failed PAL4 device end preserves current camera for retry");
    Require(RwCameraEndUpdate(camera)==camera && RwCameraGetCurrentCamera()==nullptr,
            "PAL4 callback forwards actual SDK device end and clears current camera");
    auto* image=RwImageCreate(64,64,32);
    Require(image && RwImageAllocatePixels(image) && RwImageSetFromRaster(image,owner.raster),
            "PAL4 callback actual GPU readback");
    unsigned changed=0;
    for(int y=0;y<64;++y){
        const auto* row=reinterpret_cast<const RwRGBA*>(RwImageGetPixels(image)+y*RwImageGetStride(image));
        for(int x=0;x<64;++x)changed+=row[x].red!=17 || row[x].green!=34 || row[x].blue!=51;
    }
    RwImageDestroy(image);
    Require(changed==0,"PAL4 EndUpdate must preserve cleared raster without adding SDK logo pixels");
}
void Resources() {
    NativeCameraResourceRuntime runtime;
    const auto camera=CameraResourceHelpers::Create(16,runtime);
    Require(camera!=0,"game camera helper failed");
    auto* native=reinterpret_cast<RwCamera*>(camera);
    Require(native->zBuffer && !native->frameBuffer && native->projectionType==rwPARALLEL,"game owned depth raster/projection");
    Require(RwRasterGetType(native->zBuffer)==rwRASTERTYPEZBUFFER,"game raster type one is z-buffer");
    Require(CameraResourceHelpers::SetClipAndWindow(camera,10,runtime)==camera,"game clip return");
    Near(native->nearPlane,0.01f,"game near"); Near(native->farPlane,15,"game far"); Near(native->viewWindow.x,10,"game window");
    Camera source; const RwV3d position{2,3,4}; RwFrameTranslate(source.frame,&position,rwCOMBINEREPLACE);
    Require(CameraResourceHelpers::CopyFrame(camera,reinterpret_cast<std::uint32_t>(source.camera),runtime)==camera,"game copy return");
    Require(std::memcmp(RwFrameGetMatrix(RwCameraGetFrame(native)),RwFrameGetMatrix(source.frame),64)==0,"game copies full modelling matrix");
    const CameraRenderVector3 target{8,9,10};
    Require(CameraResourceHelpers::UpdateTarget(camera,reinterpret_cast<std::uint32_t>(&target),runtime)==camera,"game target return");
    const auto* matrix=RwFrameGetMatrix(RwCameraGetFrame(native));
    Near(matrix->pos.x,8,"target x"); Near(matrix->pos.y,9,"target y"); Near(matrix->pos.z,2.5f,"target half-depth z");
    auto* borrowed=RwRasterCreate(8,8,32,rwRASTERTYPENORMAL); Require(borrowed!=nullptr,"borrowed color create");
    RwCameraSetRaster(native,borrowed);
    Require(CameraResourceHelpers::Destroy(camera,runtime)==1,"game resource destroy");
    Require(RwRasterGetWidth(borrowed)==8 && RwRasterDestroy(borrowed),"game helper must preserve borrowed color raster");
}
void ShadowSectorFixture(RwResEntry*, void*, RwUInt8, RwUInt32) {
    throw std::runtime_error("lifecycle test must not render sectors");
}
void ShadowLifecycle() {
    bool rejected=false;
    try { NativeCameraRenderLifecycleRuntime missing(nullptr); }
    catch(const std::invalid_argument&) { rejected=true; }
    Require(rejected,"shadow lifecycle requires actual typed sector callback");
    RwBBox bounds{{100,100,100},{-100,-100,-100}};
    struct Borrowed {
        RpWorld* world{};
        RpClump* clump=RpClumpCreate();
        RpLight* light=RpLightCreate(rpLIGHTDIRECTIONAL);
        RwFrame* frame=RwFrameCreate();
        explicit Borrowed(RwBBox& bounds) : world(RpWorldCreate(&bounds)) {}
        ~Borrowed() {
            if(light) { RpLightSetFrame(light,nullptr);RpLightDestroy(light); }
            if(frame) RwFrameDestroy(frame);
            if(clump) RpClumpDestroy(clump);
            if(world) RpWorldDestroy(world);
        }
    } borrowed(bounds);
    Require(borrowed.world && borrowed.clump && borrowed.light && borrowed.frame,"shadow lifecycle borrowed SDK resources");
    RpLightSetFrame(borrowed.light,borrowed.frame);
    const RwV3d position{3,4,5};RwFrameTranslate(borrowed.frame,&position,rwCOMBINEREPLACE);
    const float dimensions[4]{0,0,0,10}, target[4]{8,9,10,10};
    const auto address=[](const void* p) { return reinterpret_cast<std::uint32_t>(p); };
    NativeCameraRenderLifecycleRuntime runtime(ShadowSectorFixture);
    CameraRenderState state;
    struct StateOwner {
        CameraRenderState& state;NativeCameraRenderLifecycleRuntime& runtime;
        ~StateOwner() { (void)CameraRenderObject::Cleanup(state,runtime); }
    } state_owner{state,runtime};
    for(const bool stereo : {true,false}) {
        Require(CameraRenderObject::Initialize(state,address(borrowed.world),address(borrowed.clump),
            address(borrowed.light),address(dimensions),address(target),{6,stereo},runtime)==1,"shadow lifecycle initialize");
        Require(state.active==1 && state.world==address(borrowed.world) && state.clump==address(borrowed.clump),"shadow state borrowed identities");
        auto* primary=reinterpret_cast<RwCamera*>(state.camera);
        auto* auxiliary=reinterpret_cast<RwCamera*>(state.auxiliary_camera);
        Require(primary && auxiliary && RwCameraGetWorld(primary)==borrowed.world &&
            !RwCameraGetWorld(auxiliary),"only primary shadow camera joins world");
        Require(RwCameraGetRaster(primary)==reinterpret_cast<RwRaster*>(state.primary_raster) &&
            !RwCameraGetRaster(auxiliary),"shadow framebuffer borrowing");
        Require(RwRasterGetWidth(primary->zBuffer)==64 &&
            RwRasterGetWidth(auxiliary->zBuffer)==(stereo?32:64),"shadow owned depth sizes");
        Require(RwRasterGetWidth(reinterpret_cast<RwRaster*>(state.primary_raster))==64 &&
            RwRasterGetWidth(reinterpret_cast<RwRaster*>(state.effects_raster))==(stereo?32:64),"shadow color sizes");
        Require(stereo ? state.stereo_raster!=0 : state.stereo_raster==0,"optional stereo raster");
        Near(primary->farPlane,30,"shadow initial far");Near(primary->nearPlane,0.02f,"shadow initial near");
        Near(primary->viewWindow.x,20,"shadow initial window");
        auto* matrix=RwFrameGetMatrix(RwCameraGetFrame(primary));
        Near(matrix->pos.x,8,"shadow initial target x");Near(matrix->pos.y,9,"shadow initial target y");
        Near(matrix->pos.z,-5,"shadow initial target depth offset");
        Require(state.second_pipeline && state.texture,"shadow projection resources");
        auto* texture=reinterpret_cast<RwTexture*>(state.texture);
        Require(!RwTextureGetRaster(texture) && RwTextureGetFilterMode(texture)==rwFILTERLINEAR &&
            RwTextureGetAddressingU(texture)==rwTEXTUREADDRESSCLAMP &&
            RwTextureGetAddressingV(texture)==rwTEXTUREADDRESSCLAMP,"shadow texture original sampling");
        auto* pipeline=reinterpret_cast<RxPipeline*>(state.second_pipeline);
        Require(pipeline->numNodes==1 && RxD3D9AllInOneGetRenderCallBack(&pipeline->nodes[0])==ShadowSectorFixture,
            "shadow typed sector callback installed");
        Require(constructed-destroyed==2,"two live shadow cameras");
        (void)CameraRenderObject::Cleanup(state,runtime);
        Require(!state.active && !state.camera && !state.auxiliary_camera && !state.primary_raster &&
            !state.stereo_raster && !state.effects_raster && !state.texture && !state.second_pipeline &&
            !state.world && !state.clump,"shadow cleanup clears handles");
        Require(constructed==destroyed,"shadow camera destruction balances plugins");
        Require(RpLightGetFrame(borrowed.light)==borrowed.frame && RpLightGetType(borrowed.light)==rpLIGHTDIRECTIONAL,
            "shadow cleanup preserves borrowed light");
        Near(RwFrameGetMatrix(borrowed.frame)->pos.x,3,"shadow cleanup preserves source transform");
        (void)CameraRenderObject::Cleanup(state,runtime);
    }
    // Cleanup also accepts partially initialized native resources.
    state.camera=runtime.CreateCamera(64);
    state.primary_raster=runtime.CreateRaster(64);
    Require(state.camera && state.primary_raster,"partial shadow native resources");
    runtime.AddToWorld(address(borrowed.world),state.camera);
    runtime.SetCameraRaster(state.camera,state.primary_raster);
    (void)CameraRenderObject::Cleanup(state,runtime);
    Require(!state.camera && !state.primary_raster && constructed==destroyed,"partial shadow cleanup");
}
void ShadowAtomicFlags() {
    Camera plugin_lifetime(true);
    NativeHookClumpNames names;
    Require(names.Load("I:/Games/original/gamedata/Common/rpHookNameTable.hnt") && names.Count()==2,"original hook table load");
    Require(names.Matches("LHANDWEAPON_01") && names.Matches("rHandWeapon") &&
        !names.Matches("xLHandWeapon") && !names.Matches("LHand") && !names.Matches(""),"hook table prefix/case semantics");
    Require(!names.Load("H:/PAL4/objs/nonexistent-hook-name-table.hnt") && names.Count()==2 &&
        names.Matches("LHandWeapon"),"failed hook table load preserves prior entries");
    Require(names.Load("I:/Games/original/gamedata/Common/rpHookNameTable.hnt") && names.Count()==4,
        "original repeated hook table load appends entries");
    struct ClumpOwner {
        RpClump* clump=RpClumpCreate();
        ~ClumpOwner() { if(clump)RpClumpDestroy(clump); }
    } owner;
    Require(owner.clump!=nullptr,"shadow flags clump");
    std::array<RpGeometry*,4> geometries{};
    for(unsigned i=0;i<geometries.size();++i) {
        auto* geometry=RpGeometryCreate(3,1,rpGEOMETRYPOSITIONS|rpGEOMETRYNORMALS|rpGEOMETRYLIGHT|rpGEOMETRYMODULATEMATERIALCOLOR);
        Require(geometry!=nullptr,"shadow flags geometry");
        auto* atomic=RpAtomicCreate();auto* frame=RwFrameCreate();
        Require(atomic && frame,"shadow flags atomic frame");
        RpAtomicSetFrame(atomic,frame);RpAtomicSetGeometry(atomic,geometry,0);
        RpClumpAddAtomic(owner.clump,atomic);RpGeometryDestroy(geometry);geometries[i]=RpAtomicGetGeometry(atomic);
        // RpClumpDestroy owns atomics; attach their frames under a clump root.
        if(!RpClumpGetFrame(owner.clump))RpClumpSetFrame(owner.clump,RwFrameCreate());
        RwFrameAddChild(RpClumpGetFrame(owner.clump),frame);
    }
    auto add=[&](unsigned i,const char* text) {
        const auto slot=RpGeometryAddUserDataArray(geometries[i],const_cast<char*>("shadow-name"),rpSTRINGUSERDATA,1);
        Require(slot>=0,"shadow string userdata add");
        RpUserDataArraySetString(RpGeometryGetUserDataArray(geometries[i],slot),0,const_cast<char*>(text));
    };
    add(1,"LhAnDWeapon_01");add(2,"xLHandWeapon");
    Require(RpGeometryAddUserDataArray(geometries[3],const_cast<char*>("ignored-int"),rpINTUSERDATA,1)==0,"shadow numeric userdata");
    const auto slot=RpGeometryAddUserDataArray(geometries[3],const_cast<char*>("second-name"),rpSTRINGUSERDATA,2);
    Require(slot==1,"shadow second userdata array index");
    auto* data=RpGeometryGetUserDataArray(geometries[3],slot);
    RpUserDataArraySetString(data,0,const_cast<char*>("NoMatch"));
    RpUserDataArraySetString(data,1,const_cast<char*>("RHANDWEAPON_02"));
    std::array<RwUInt32,4> original{};
    for(unsigned i=0;i<4;++i)original[i]=RpGeometryGetFlags(geometries[i]);
    ApplyNativeShadowAtomicFlags(owner.clump,names,false);
    for(unsigned i=0;i<4;++i)Require(RpGeometryGetFlags(geometries[i])==
        ((i==1 || i==3)?original[i]:(original[i]&~0x60u)),"shadow pre callback exception/flags");
    ApplyNativeShadowAtomicFlags(owner.clump,names,true);
    for(unsigned i=0;i<4;++i)Require(RpGeometryGetFlags(geometries[i])==original[i],"shadow post callback flags");
}
void ShadowGeometry() {
    Camera source(true), destination(true);
    auto* parent=RwFrameCreate();Require(parent!=nullptr,"shadow source parent");
    struct ParentOwner {
        RwFrame* parent; RwFrame* child;
        ~ParentOwner() { RwFrameRemoveChild(child);RwFrameDestroy(parent); }
    } parent_owner{parent,source.frame};
    Require(RwFrameAddChild(parent,source.frame)!=nullptr,"shadow parent attach");
    const RwV3d parent_position{10,20,30}, position{3,4,12};
    RwFrameTranslate(parent,&parent_position,rwCOMBINEREPLACE);
    RwFrameTranslate(source.frame, &position, rwCOMBINEREPLACE);
    const float dimensions[4]{0,0,0,26}, target[4]{10,20,30,5};
    const auto address=[](const void* p) { return reinterpret_cast<std::uint32_t>(p); };
    CameraRenderState state;
    state.source_frame=address(source.camera);
    state.camera=address(destination.camera);
    state.dimensions=address(dimensions);state.target=address(target);
    NativeCameraRenderGeometryRuntime runtime;
    Require(CameraRenderGeometry::UpdatePosition(state,true,runtime)==state.camera,"shadow position result");
    const auto* matrix=RwFrameGetMatrix(destination.frame);
    Near(matrix->pos.x,19,"shadow offset x");Near(matrix->pos.y,32,"shadow offset y");Near(matrix->pos.z,66,"shadow offset z");
    Near(destination.camera->farPlane,58.5f,"shadow far radius factors");
    Near(destination.camera->nearPlane,0.039f,"shadow near radius factors");
    Near(destination.camera->viewWindow.x,39,"shadow view radius factors");
    Near(matrix->at.z,1,"shadow source orientation");
    (void)CameraRenderGeometry::UpdateFrustum(state,false,2,runtime);
    Require(state.frustum_type==4,"shadow frustum intersection type");
    Near(state.minimum[0],5,"shadow bbox lower x");Near(state.minimum[1],15,"shadow bbox lower y");
    Near(state.minimum[2],25,"shadow bbox lower z");
    Near(state.maximum[0],15,"shadow bbox upper x");Near(state.maximum[1],25,"shadow bbox upper y");
    Near(state.maximum[2],37,"shadow bbox forward extrusion");
    // Tilt to exercise negative frustum extension as well as special mode.
    const RwV3d axis{1,0,0};RwFrameRotate(destination.frame,&axis,90,rwCOMBINEREPLACE);
    (void)CameraRenderGeometry::UpdateFrustum(state,true,2,runtime);
    Near(state.minimum[1],18,"shadow special negative extrusion");Near(state.maximum[1],20,"shadow special upper");
    Near(state.minimum[0],10,"shadow special no radius lower");Near(state.maximum[0],10,"shadow special no radius upper");
    (void)CameraRenderGeometry::UpdatePosition(state,false,runtime);
    // Original updates the LTM image directly in this branch; do not replace
    // this with modelling-space copy or force an extra SDK frame synchronization.
    const auto* ltm=&destination.frame->ltm;
    Near(ltm->pos.x,13,"shadow no-offset world x");Near(ltm->pos.y,24,"shadow no-offset world y");
    Near(ltm->pos.z,42,"shadow no-offset world z");
}
}
void RegisterShadowAtomicPlugins() { Require(RpUserDataPluginAttach()!=FALSE,"shadow userdata plugin attach"); }
void RegisterCameraPlugin() {
    plugin_offset=RwCameraRegisterPlugin(4,0x123410,Construct,Destruct,nullptr);
    Require(plugin_offset>=static_cast<int>(sizeof(RwCamera)),"camera plugin registration");
    Require(RwCameraGetPluginOffset(0x123410)==plugin_offset,"camera plugin lookup");
}
void RunCameraCase(std::string_view name) {
    if(name=="camera_shadow_atomic_flags") ShadowAtomicFlags();
    else if(name=="camera_shadow_lifecycle") ShadowLifecycle();
    else if(name=="camera_shadow_geometry") ShadowGeometry();
    else if(name=="camera_pal4_variant") Pal4Variant();
    else if(name=="camera_lifetime") Lifetime();
    else if(name=="camera_projection") Projection();
    else if(name=="camera_dispatch") Dispatch();
    else if(name=="camera_resources") Resources();
    else throw std::runtime_error("unknown camera case");
    Require(constructed>0 && constructed==destroyed,"camera plugin lifetime balance");
}
}
