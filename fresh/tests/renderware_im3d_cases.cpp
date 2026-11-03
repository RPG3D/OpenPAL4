#include "renderware_im3d_cases.h"
#include "pal4/clump_bounds_render_native.h"
#include <d3d9.h>
#include <windows.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace pal4::test {
namespace {
void Require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static_assert(sizeof(rwIm3DPool)==60 && offsetof(RwIm3DVertex,color)==24);
struct ProbeState {
    bool transform_ok=true,render_ok=true;
    int transforms{},renders{};
    rwIm3DPool pool{};
    _rwIm3DPoolStash stash{};
    std::array<RwIm3DVertex,8> vertices{};
    std::vector<RwImVertexIndex> indices;
} probe;
RwBool TransformProbe(RxPipelineNode*,const RxPipelineNodeParam* param) {
    ++probe.transforms;probe.pool=*static_cast<rwIm3DPool*>(param->dataParam);
    if(probe.pool.numElements==8)std::memcpy(probe.vertices.data(),probe.pool.elements,sizeof(probe.vertices));
    return probe.transform_ok;
}
RwBool RenderProbe(RxPipelineNode*,const RxPipelineNodeParam* param) {
    ++probe.renders;probe.stash=*static_cast<_rwIm3DPoolStash*>(param->dataParam);
    probe.indices.clear();if(probe.stash.indices)probe.indices.assign(probe.stash.indices,probe.stash.indices+probe.stash.numIndices);
    return probe.render_ok;
}
struct Pipeline {
    RxNodeDefinition definition{};RxPipeline* value{};
    explicit Pipeline(RxNodeBodyFn body) {
        definition.name=const_cast<char*>("pal4-im3d-contract");definition.nodeMethods.nodeBody=body;definition.editable=rxNODEDEFCONST;
        value=RxPipelineCreate();Require(value!=nullptr,"probe pipeline create");
        auto* locked=RxPipelineLock(value);Require(locked && RxLockedPipeAddFragment(locked,nullptr,&definition,static_cast<RxNodeDefinition*>(nullptr)),"probe pipeline node");
        Require(RxLockedPipeUnlock(locked)==value,"probe pipeline unlock");
    }
    ~Pipeline(){if(value)RxPipelineDestroy(value);}
};
struct Pipelines {
    Pipeline transform{TransformProbe},render{RenderProbe};
    RxPipeline* old_transform{};std::array<RxPipeline*,6> old_render{};
    Pipelines() {
        probe={};old_transform=RwIm3DGetTransformPipeline();
        Require(RwIm3DSetTransformPipeline(transform.value)==transform.value,"transform setter returns installed pipeline");
        for(int i=1;i<=6;++i){const auto type=static_cast<RwPrimitiveType>(i);old_render[i-1]=RwIm3DGetRenderPipeline(type);Require(RwIm3DSetRenderPipeline(render.value,type)==render.value,"render setter supports six slots");}
    }
    ~Pipelines(){RwIm3DEnd();RwIm3DSetTransformPipeline(old_transform);for(int i=1;i<=6;++i)RwIm3DSetRenderPipeline(old_render[i-1],static_cast<RwPrimitiveType>(i));}
};
void MatrixLifetime(bool& fail_next) {
    fail_next=true;Require(!RwMatrixCreate() && !fail_next,"matrix allocation failure propagates");
    auto* m=RwMatrixCreate();Require(m && m->flags==0x20003,"matrix creation flags");
    Require(m->right.x==1 && m->up.y==1 && m->at.z==1 && m->right.y==0 && m->right.z==0 && m->up.x==0 && m->up.z==0 && m->at.x==0 && m->at.y==0 && m->pos.x==0 && m->pos.y==0 && m->pos.z==0,"matrix create initializes affine identity");
    Require(RwMatrixDestroy(m),"matrix destruction");
}
void Session() {
    Pipelines pipes;std::array<RwIm3DVertex,8> vertices{};RwMatrix matrix{};RwMatrixSetIdentity(&matrix);
    Require(!RwIm3DEnd() && !RwIm3DRenderPrimitive(rwPRIMTYPETRILIST),"idle end/render failure");
    Require(RwIm3DTransform(vertices.data(),8,&matrix,2)==vertices.data(),"transform returns original input");
    Require(probe.pool.numElements==8 && probe.pool.elements==vertices.data() && probe.pool.stride==36 && probe.pool.stash.flags==26 && probe.pool.stash.ltm==&matrix,"pool pointer/layout/mandatory flags");
    auto* pool=_rwIm3DGetPool();const auto before=*pool;const auto calls=probe.transforms;
    Require(!RwIm3DTransform(vertices.data(),65537,nullptr,0) && probe.transforms==calls && std::memcmp(pool,&before,60)==0,"oversized request rejects without replacing prior session");
    Require(RwIm3DEnd(),"end existing session");const rwIm3DPool empty{};Require(std::memcmp(pool,&empty,60)==0 && !RwIm3DEnd(),"end clears all60 bytes and repeated end fails");
    // A probe node consumes no vertex memory: safely expose original16-bit wrap.
    Require(RwIm3DTransform(vertices.data(),65536,nullptr,0)==vertices.data() && probe.pool.numElements==0,"65536 accepted but truncates to zero");Require(RwIm3DEnd(),"wrapped pool cleanup");
    probe.transform_ok=false;Require(!RwIm3DTransform(vertices.data(),8,nullptr,0),"pipeline failure propagated");
    Require(pool->elements==vertices.data() && RwIm3DEnd(),"failed transform leaves active pool requiring End");
    probe.transform_ok=true;
    Require(RwIm3DSetTransformPipeline(nullptr)==pipes.old_transform && RwIm3DGetTransformPipeline()==pipes.old_transform,"null transform setter selects platform default");
}
void Dispatch() {
    Pipelines pipes;std::array<RwIm3DVertex,8> vertices{};RwImVertexIndex indices[]{0,1,2,3,4,5,6,7};
    Require(RwIm3DTransform(vertices.data(),7,nullptr,0),"dispatch transform");
    for(int i=1;i<=5;++i){const auto type=static_cast<RwPrimitiveType>(i);
        Require(RwIm3DRenderIndexedPrimitive(type,indices,7),"indexed pipeline dispatch");
        const auto expected=i==1?6u:i==3?6u:7u;
        Require(probe.stash.primType==type && probe.stash.indices==indices && probe.stash.numIndices==expected && probe.stash.pipeline==pipes.render.value,"indexed list truncation and chosen slot");
        Require(RwIm3DRenderPrimitive(type) && !probe.stash.indices && probe.stash.numIndices==7,"nonindexed uses raw16-bit vertex count without list truncation");
    }
    probe.render_ok=false;Require(!RwIm3DRenderIndexedPrimitive(rwPRIMTYPELINELIST,indices,2),"render pipeline false propagated");
    Require(RwIm3DEnd(),"render failure retains pool for End");
    Require(!RwIm3DSetRenderPipeline(pipes.render.value,static_cast<RwPrimitiveType>(99)),"invalid setter fails");
    for(int i=1;i<=6;++i)Require(RwIm3DGetRenderPipeline(static_cast<RwPrimitiveType>(i))==pipes.render.value,"invalid setter leaves valid slots intact");
    for(int i=1;i<=6;++i)Require(RwIm3DSetRenderPipeline(nullptr,static_cast<RwPrimitiveType>(i))==pipes.old_render[i-1],"null render setter returns platform default");
}
void GameContract() {
    Pipelines pipes;NativeClumpBoundsRenderRuntime runtime;ClumpBoundsState box{};box.minimum={-1,-2,-3};box.maximum={4,5,6};
    RwMatrix matrix{};RwMatrixSetIdentity(&matrix);
    Require(ClumpSpatialBounds::RenderBoundingBox(box,0x11223344,reinterpret_cast<std::uint32_t>(&matrix),runtime)!=0,"game final shade mode setter result");
    Require(probe.transforms==1 && probe.renders==1 && probe.pool.stash.flags==26,"game immediate sequence");
    for(unsigned i=0;i<8;++i){const auto& v=probe.vertices[i];Require(v.objVertex.x==((i&1)?4.f:-1.f) && v.objVertex.y==((i&2)?5.f:-2.f) && v.objVertex.z==((i&4)?6.f:-3.f) && v.color==0x11443322,"game corner order and packed color");}
    const std::vector<RwImVertexIndex> expected{0,1,1,3,3,2,2,0,4,5,5,7,7,6,6,4,0,4,1,5,2,6,3,7};
    Require(probe.indices==expected && probe.stash.primType==rwPRIMTYPELINELIST,"original twelve-edge index table");
    RwUInt32 shade{};Require(RwRenderStateGet(rwRENDERSTATESHADEMODE,&shade) && shade==rwSHADEMODEGOURAUD,"game sets fixed GOURAUD afterward");
    Require(!_rwIm3DGetPool()->elements,"successful game path ends session");
    probe.transform_ok=false;const auto draws=probe.renders;
    Require(ClumpSpatialBounds::RenderBoundingBox(box,0,0,runtime)!=0 && probe.renders==draws && _rwIm3DGetPool()->elements,"game failure skips End and leaves pool live");
    Require(RwIm3DEnd(),"explicit diagnostic cleanup after game failure");
}
struct Camera {
    RwCamera* value=RwCameraCreate();RwFrame* frame=RwFrameCreate();RwRaster *color{},*depth{};
    explicit Camera(HWND window) {
        Require(value&&frame,"im3d camera/frame");RwCameraSetFrame(value,frame);
        color=RwRasterCreate(32,32,32,rwRASTERTYPECAMERATEXTURE|rwRASTERFORMAT8888);depth=RwRasterCreate(32,32,0,rwRASTERTYPEZBUFFER);Require(color&&depth,"im3d targets");RwCameraSetRaster(value,color);RwCameraSetZRaster(value,depth);
        RwCameraSetNearClipPlane(value,1);RwCameraSetFarClipPlane(value,10);const RwV2d view{1,1};RwCameraSetViewWindow(value,&view);
        Require(RwCameraBeginUpdate(value)&&RwCameraEndUpdate(value)&&RwCameraShowRaster(value,window,0),"im3d initial present policy");
    }
    ~Camera(){RwCameraSetFrame(value,nullptr);RwCameraSetRaster(value,nullptr);RwCameraSetZRaster(value,nullptr);RwCameraDestroy(value);RwFrameDestroy(frame);RwRasterDestroy(color);RwRasterDestroy(depth);}
    void Begin(){RwRGBA black{0,0,0,255};Require(RwCameraClear(value,&black,rwCAMERACLEARIMAGE|rwCAMERACLEARZ)&&RwCameraBeginUpdate(value),"im3d clear/begin");
        RwD3D9SetVertexShader(nullptr);RwD3D9SetPixelShader(nullptr);RwRenderStateSet(rwRENDERSTATETEXTURERASTER,nullptr);RwRenderStateSet(rwRENDERSTATEZTESTENABLE,reinterpret_cast<void*>(FALSE));RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,reinterpret_cast<void*>(FALSE));RwRenderStateSet(rwRENDERSTATECULLMODE,reinterpret_cast<void*>(rwCULLMODECULLNONE));RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,reinterpret_cast<void*>(FALSE));RwRenderStateSet(rwRENDERSTATEFOGENABLE,reinterpret_cast<void*>(FALSE));
        RwD3D9SetRenderState(D3DRS_LIGHTING,FALSE);RwD3D9SetRenderState(D3DRS_STENCILENABLE,FALSE);RwD3D9SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);
    }
    std::vector<RwRGBA> Read(HWND window){Require(RwCameraEndUpdate(value)&&RwCameraShowRaster(value,window,0),"im3d end/present");auto* image=RwImageCreate(32,32,32);Require(image&&RwImageAllocatePixels(image)&&RwImageSetFromRaster(image,color),"im3d image readback");std::vector<RwRGBA> out(1024);for(int y=0;y<32;++y)std::memcpy(out.data()+32*y,RwImageGetPixels(image)+y*RwImageGetStride(image),128);RwImageDestroy(image);return out;}
};
void Pixels(HWND window,bool game) {
    Camera camera(window);NativeClumpBoundsRenderRuntime runtime;
    for(bool indexed:{false,true}){
        if(game&&indexed)break;camera.Begin();
        if(game){ClumpBoundsState box{};box.minimum={-.5f,-.5f,2};box.maximum={.5f,.5f,3};Require(ClumpSpatialBounds::RenderBoundingBox(box,0xff2349db,0,runtime)!=0,"game box real draw");}
        else {std::array<RwIm3DVertex,3> vertices{};vertices[0].objVertex={-1,-1,2};vertices[1].objVertex={1,-1,2};vertices[2].objVertex={0,1,2};for(auto& v:vertices)v.color=0xffdb4923;
            Require(RwIm3DTransform(vertices.data(),3,nullptr,rwIM3D_ALLOPAQUE)!=nullptr,"real immediate transform");RwImVertexIndex order[]{0,1,2};Require(indexed?RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST,order,3):RwIm3DRenderPrimitive(rwPRIMTYPETRILIST),"real immediate render");Require(RwIm3DEnd(),"real immediate End");}
        const auto pixels=camera.Read(window);int colored=0;for(const auto& p:pixels){if(p.red==219&&p.green==73&&p.blue==35)++colored;else Require(p.red==0&&p.green==0&&p.blue==0,"unexpected immediate pixel color");}
        std::printf("im3d game=%d indexed=%d colored=%d\n",game,indexed,colored);
        Require(colored>(game?20:60)&&colored<400,"immediate coverage bounds");
        if(!game){const auto p=pixels[16*32+16];Require(p.red==219&&p.green==73&&p.blue==35,"triangle center coverage");}
        const auto outside=pixels[0];Require(outside.red==0&&outside.green==0&&outside.blue==0,"outside remains clear");
    }
}
}
void RunIm3DCase(std::string_view name,void* window,bool& fail_next){
    if(name=="im3d_matrix_lifetime")MatrixLifetime(fail_next);else if(name=="im3d_session")Session();
    else if(name=="im3d_dispatch")Dispatch();else if(name=="im3d_game_contract")GameContract();
    else if(name=="im3d_pixels")Pixels(static_cast<HWND>(window),false);else if(name=="im3d_game_pixels")Pixels(static_cast<HWND>(window),true);
    else throw std::runtime_error("unknown im3d case");
}
}
