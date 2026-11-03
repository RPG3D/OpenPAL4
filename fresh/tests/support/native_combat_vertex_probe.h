#pragma once
#include "pal4/native_world_sector_vertex_instancer.h"
#include "pal4/matfx_world_sector_pipeline_native.h"
#include "pal4/native_renderware_pipeline_lifetime.h"
#include "pal4/native_ui_world_camera.h"
#include <d3d9.h>
#include <array>
#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace pal4::test {
namespace combat_vertex_probe {
inline void Check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
struct Context { unsigned sectors{},vertices{},meshes{}; std::exception_ptr error; };
inline thread_local Context* current{};
inline void Verify(RwResEntry* entry,void* object,RwUInt8 type,RwUInt32 flags) noexcept {
    try {
        auto& context=*current;
        auto& sector=*static_cast<RpWorldSector*>(object);
        auto& h=*reinterpret_cast<RxD3D9ResEntryHeader*>(entry+1);
        const auto& stream=h.vertexStream[0];
        Check(type==static_cast<RwUInt8>(rwSECTORATOMIC) && h.numMeshes==sector.mesh->numMeshes &&
              h.totalNumVertex==sector.numVertices && !h.useOffsets && stream.vertexBuffer,
              "M01 SDK resource entry counts and stream ownership");
        std::array<D3DVERTEXELEMENT9,MAXD3DDECLLENGTH+1> elements{};
        UINT length=static_cast<UINT>(elements.size());
        Check(SUCCEEDED(static_cast<IDirect3DVertexDeclaration9*>(h.vertexDeclaration)->GetDeclaration(elements.data(),&length)),
              "M01 GPU declaration readback");
        void* data{};
        auto* vb=static_cast<IDirect3DVertexBuffer9*>(stream.vertexBuffer);
        Check(SUCCEEDED(vb->Lock(stream.offset,stream.stride*h.totalNumVertex,&data,0)),"M01 GPU buffer readback lock");
        struct Unlock { IDirect3DVertexBuffer9* vb; ~Unlock(){vb->Unlock();} } unlock{vb};
        unsigned position{},uv0{},uv1{},color{};
        for(UINT e=0;e+1<length;++e) {
            const auto& element=elements[e];
            Check(element.Stream==0u,"M01 attributes use original single stream");
            for(RwUInt32 v=0;v<h.totalNumVertex;++v) {
                const auto* bytes=static_cast<const char*>(data)+stream.stride*v+element.Offset;
                if(element.Usage==D3DDECLUSAGE_POSITION) {
                    Check(element.Type==D3DDECLTYPE_FLOAT3 && std::memcmp(bytes,&sector.vertices[v],12u)==0,
                          "M01 every GPU position equals original BSP position");
                } else if(element.Usage==D3DDECLUSAGE_TEXCOORD) {
                    Check(element.Type==D3DDECLTYPE_FLOAT2 && element.UsageIndex<2u &&
                          std::memcmp(bytes,&sector.texCoords[element.UsageIndex][v],8u)==0,
                          "M01 every GPU UV equals original BSP UV");
                } else if(element.Usage==D3DDECLUSAGE_COLOR) {
                    DWORD value{}; std::memcpy(&value,bytes,4u);
                    const auto c=sector.preLitLum[v];
                    Check(value==D3DCOLOR_ARGB(c.alpha,c.red,c.green,c.blue),"M01 GPU prelight RGBA conversion");
                }
            }
            if(element.Usage==D3DDECLUSAGE_POSITION) ++position;
            if(element.Usage==D3DDECLUSAGE_TEXCOORD && element.UsageIndex==0u) ++uv0;
            if(element.Usage==D3DDECLUSAGE_TEXCOORD && element.UsageIndex==1u) ++uv1;
            if(element.Usage==D3DDECLUSAGE_COLOR) ++color;
        }
        Check(position==1u && uv0==((flags&0x84u)?1u:0u) &&
              uv1==(sector.texCoords[1]?1u:0u) && color==((flags&8u)?1u:0u),
              "M01 declaration preserves original position, both UV channels and prelight");
        const auto* meshes=reinterpret_cast<const RxD3D9InstanceData*>(&h+1);
        for(RwUInt32 m=0;m<h.numMeshes;++m) {
            const auto& mesh=meshes[m];
            Check(mesh.baseIndex==mesh.minVert+stream.offset/stream.stride &&
                  mesh.minVert+mesh.numVertices<=h.totalNumVertex && mesh.material && mesh.numIndex,
                  "M01 mesh baseIndex derives from minVert and managed VB offset");
            bool alpha{};
            if(flags&8u) for(RwUInt32 v=mesh.minVert;v<mesh.minVert+mesh.numVertices;++v)
                alpha=alpha || sector.preLitLum[v].alpha!=255u;
            Check(bool(mesh.vertexAlpha)==alpha,"M01 per-mesh vertex alpha matches original BSP colors");
        }
        ++context.sectors; context.vertices+=h.totalNumVertex; context.meshes+=h.numMeshes;
    } catch(...) { current->error=std::current_exception(); }
}
}
inline void VerifyNativeCombatVertices(RpWorld& world) {
    using namespace combat_vertex_probe;
    Check(!NativeWorldSectorVertexInstancer::Callback(nullptr,nullptr,FALSE) &&
          !NativeWorldSectorVertexInstancer::LastError().empty(),"native instance callback reports invalid context");
    RpWorldSector* first_sector{};
    RpWorldForAllWorldSectors(&world,[](RpWorldSector* sector,void* data)->RpWorldSector* {
        auto& first=*static_cast<RpWorldSector**>(data); if(!first) first=sector; return sector;
    },&first_sector);
    Check(first_sector!=nullptr,"M01 has a sector for unsupported tangent preflight");
    const auto original_usage=RpD3D9WorldSectorGetUsageFlags(first_sector);
    {
        struct Restore { RpWorldSector* sector; RpD3D9WorldSectorUsageFlag flags;
            ~Restore(){RpD3D9WorldSectorSetUsageFlags(sector,flags);} } restore{first_sector,original_usage};
        RpD3D9WorldSectorSetUsageFlags(first_sector,static_cast<RpD3D9WorldSectorUsageFlag>(original_usage|0x10000u));
        RxD3D9ResEntryHeader blocked{},empty{};
        bool rejected{};
        try {(void)NativeWorldSectorVertexInstancer::Instance(world,*first_sector,blocked);}
        catch(const std::runtime_error& e) {rejected=std::string(e.what()).find("tangent conversion ABI")!=std::string::npos;}
        Check(rejected && std::memcmp(&blocked,&empty,sizeof blocked)==0,
              "unsupported EXE tangent ABI fails before declaration or buffer mutation");
    }
    NativeMatFxWorldSectorPipelineRuntime runtime(NativeWorldSectorVertexInstancer::Callback,Verify);
    auto* pipeline=reinterpret_cast<RxPipeline*>(MatFxWorldSectorPipeline::Create(runtime));
    Check(pipeline!=nullptr,"M01 native game pipeline created");
    struct Bindings {
        RxPipeline* pipeline;
        std::vector<std::pair<RpWorldSector*,RxPipeline*>> previous;
        ~Bindings() {
            NativeRenderWarePipelineLifetime lifetime;
            for(const auto& [sector,old]:previous)
                (void)RenderWarePipelineLifetime::BindAndReleaseResource(
                    reinterpret_cast<std::uint32_t>(sector),reinterpret_cast<std::uint32_t>(old),lifetime);
            RxPipelineDestroy(pipeline);
        }
    } bindings{pipeline};
    RpWorldForAllWorldSectors(&world,[](RpWorldSector* sector,void* data)->RpWorldSector* {
        auto& bindings=*static_cast<Bindings*>(data);
        bindings.previous.emplace_back(sector,sector->pipeline);
        NativeRenderWarePipelineLifetime lifetime;
        (void)RenderWarePipelineLifetime::BindAndReleaseResource(reinterpret_cast<std::uint32_t>(sector),
            reinterpret_cast<std::uint32_t>(bindings.pipeline),lifetime);
        return sector;
    },&bindings);
    NativeUiWorldCamera camera;
    Check(camera.Open(&world,64u,64u),"M01 offscreen instance camera");
    Context context;
    struct ContextScope { Context* old; ~ContextScope(){current=old;} } scope{current};
    current=&context;
    Check(RwCameraBeginUpdate(camera.Camera())!=nullptr,"M01 instance camera begin");
    struct End { RwCamera* camera; ~End(){RwCameraEndUpdate(camera);} } end{camera.Camera()};
    for(const auto& [sector,old]:bindings.previous) {
        if(!RpWorldSectorRender(sector)) throw std::runtime_error(NativeWorldSectorVertexInstancer::LastError());
        if(context.error) std::rethrow_exception(context.error);
        Check(sector->repEntry!=nullptr,"M01 native pipeline owns SDK resource entry");
    }
    Check(context.sectors==5u && context.vertices==4668u && context.meshes==136u,
          "M01 native vertex conversion covers all original sectors and meshes");
    const auto first=context;
    for(const auto& [sector,old]:bindings.previous) {
        const auto* entry=sector->repEntry;
        Check(RpWorldSectorRender(sector)!=nullptr,"M01 repeat pipeline execution");
        if(context.error) std::rethrow_exception(context.error);
        Check(sector->repEntry==entry,"M01 repeat rendering reuses instanced resource entry");
    }
    Check(context.sectors==2u*first.sectors,"M01 repeat rendering reaches every callback");
    Check(NativeWorldSectorVertexInstancer::LastError().empty(),"successful instance callbacks clear prior diagnostics");
    std::cout<<"Native Combat vertex readback: sectors="<<first.sectors<<" vertices="<<first.vertices
             <<" meshes="<<first.meshes<<" repeat-reuse=1\n";
}
}
