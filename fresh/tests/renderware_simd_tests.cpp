#include "support/native_test_display.h"
#include <windows.h>
#include <rwcore.h>
#include <rpworld.h>
#include <d3d9.h>
#include <intrin.h>
#include <xmmintrin.h>
#include "pal4/atomic_bounds_native.h"
#include "pal4/clump_bounds_native.h"
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

// Test-only declarations of audited archive symbols. Production adapters use
// public RwV3d APIs; these probes compare all backends without changing dispatch.
extern "C" {
RwV3d* __cdecl _rwIntelSSEV3dTransformPoint(RwV3d*,const RwV3d*,const RwMatrix*);
RwV3d* __cdecl _rwIntelSSEV3dTransformVector(RwV3d*,const RwV3d*,const RwMatrix*);
RwV3d* __cdecl _rwIntelSSEP4V3dTransformPoint(RwV3d*,const RwV3d*,const RwMatrix*);
RwV3d* __cdecl _rwIntelSSEP4V3dTransformVector(RwV3d*,const RwV3d*,const RwMatrix*);
RwV3d* __cdecl _rwIntelSSEV3dTransformPoints(RwV3d*,const RwV3d*,RwInt32,const RwMatrix*);
RwV3d* __cdecl _rwIntelSSEV3dTransformVectors(RwV3d*,const RwV3d*,RwInt32,const RwMatrix*);
RwV3d* __cdecl _rwIntelSSEP4V3dTransformPoints(RwV3d*,const RwV3d*,RwInt32,const RwMatrix*);
RwV3d* __cdecl _rwIntelSSEP4V3dTransformVectors(RwV3d*,const RwV3d*,RwInt32,const RwMatrix*);
RwBool __cdecl _rwCPUHaveMMX();
RwBool __cdecl _rwCPUHaveSSE();
RwBool __cdecl _rwCPUHaveSSE2();
RwBool __cdecl _rwCPUHave3DNow();
}
static_assert(sizeof(RwV3d)==12&&sizeof(RwMatrix)==64);
namespace {
void Check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
template<class T> std::uint32_t Address(T* p){return reinterpret_cast<std::uint32_t>(p);}
void Equal(const RwV3d& a,const RwV3d& b){Check(a.x==b.x&&a.y==b.y&&a.z==b.z,"vector value");}
struct Backend{const char* name;rwVectorMultFn point,vector;rwVectorsMultFn points,vectors;};
const std::array<Backend,2> backends{{{"SSE",_rwIntelSSEV3dTransformPoint,_rwIntelSSEV3dTransformVector,_rwIntelSSEV3dTransformPoints,_rwIntelSSEV3dTransformVectors},
    {"SSE-P4",_rwIntelSSEP4V3dTransformPoint,_rwIntelSSEP4V3dTransformVector,_rwIntelSSEP4V3dTransformPoints,_rwIntelSSEP4V3dTransformVectors}}};
RwMatrix Matrix(){RwMatrix m{};m.right={2,1,0};m.up={0,3,1};m.at={1,0,4};m.pos={10,20,30};return m;}
std::unordered_set<void*> allocations;bool invalid_free{};
void* Allocate(size_t n,RwUInt32){auto* p=std::malloc(n);if(p)allocations.insert(p);return p;}
void Free(void* p){if(p&&allocations.erase(p)!=1)invalid_free=true;std::free(p);}
void* Reallocate(void* p,size_t n,RwUInt32 hint){if(!p)return Allocate(n,hint);if(!n){Free(p);return nullptr;}auto* q=std::realloc(p,n);if(q){allocations.erase(p);allocations.insert(q);}return q;}
void* Callocate(size_t n,size_t s,RwUInt32 hint){auto* p=Allocate(n*s,hint);if(p)std::memset(p,0,n*s);return p;}
struct Session{
    HWND window{};bool initialized{},opened{},started{};
    Session(){const RwMemoryFunctions memory{Allocate,Free,Reallocate,Callocate};Check(RwEngineInit(&memory,0,0),"engine init");initialized=true;Check(RpWorldPluginAttach(),"world attach");
        window=pal4::test::CreateTestWindow("STATIC","PAL4 SIMD",WS_OVERLAPPEDWINDOW,64,64,nullptr,nullptr,GetModuleHandleA(nullptr),nullptr);Check(window,"hidden window");RwEngineOpenParams params{window};Check(RwEngineOpen(&params),"engine open");opened=true;Check(RwEngineStart(),"engine start");started=true;}
    ~Session(){if(started)RwEngineStop();if(opened)RwEngineClose();if(initialized)RwEngineTerm();if(window)DestroyWindow(window);}
};
void Cpu(){int regs[4]{};__cpuid(regs,1);const auto edx=static_cast<unsigned>(regs[3]);const bool mmx=(edx&(1u<<23))!=0,sse=(edx&0x6000000u)!=0,sse2=(edx&(1u<<26))!=0;
    __cpuid(regs,static_cast<int>(0x80000000u));bool amd=false;if(static_cast<unsigned>(regs[0])>0x80000000u){__cpuid(regs,static_cast<int>(0x80000001u));amd=(static_cast<unsigned>(regs[3])>>31)!=0;}
    Check(bool(_rwCPUHaveMMX())==mmx&&bool(_rwCPUHaveSSE())==sse&&bool(_rwCPUHaveSSE2())==sse2&&bool(_rwCPUHave3DNow())==amd,"archive CPU flags equal observed CPUID predicates");
    Check(bool(_rwIntelMMXsupported())==mmx&&bool(_rwIntelSSEsupported())==sse&&bool(_rwIntelSSE2supported())==sse2&&bool(_rwAMD3DNowSupported())==amd,"engine caches detection results");
    const auto flags=__readeflags();for(unsigned bit:{0u,0x200000u}){__writeeflags((flags&~0x200000u)|bit);const auto before=__readeflags();(void)_rwCPUHaveSSE();const auto after=__readeflags();__writeeflags(flags);Check(((before^after)&0x200000u)==0,"CPU probe preserves observed ID bit");}
    std::printf("CPU MMX=%d SSEpredicate=%d SSE2=%d 3DNow=%d\n",mmx,sse,sse2,amd);
}
void Alignment(){const std::array<RwV3d,3> input{{{1,2,3},{-1,0,2},{0,0,0}}};const std::array<RwV3d,3> points{{{15,27,44},{10,19,38},{10,20,30}}},vectors{{{5,7,14},{0,-1,8},{0,0,0}}};
    for(const auto& b:backends)for(int offset:{0,4,8,12}){alignas(16) std::array<unsigned char,96> bytes{};auto matrix=Matrix();matrix.flags=matrix.pad1=matrix.pad2=matrix.pad3=0x7f800001u;std::memcpy(bytes.data()+offset,&matrix,64);const auto original=bytes;
        // Deliberate raw ABI probe: C++ never dereferences this unaligned matrix.
        auto* raw=reinterpret_cast<const RwMatrix*>(bytes.data()+offset);const auto csr=_mm_getcsr();_mm_setcsr(csr&~0x3fu);
        for(bool point:{false,true}){auto data=input;auto batch=point?b.points:b.vectors;auto single=point?b.point:b.vector;const auto& expected=point?points:vectors;
            Check(batch(data.data(),data.data(),3,raw)==data.data(),"batch return/in-place");for(int i=0;i<3;++i)Equal(data[i],expected[i]);
            for(int i=0;i<3;++i){auto value=input[i];Check(single(&value,&value,raw)==&value,"single return/in-place");Equal(value,expected[i]);}
        }
        const auto invalid=_mm_getcsr()&1u;_mm_setcsr(csr);Check(!invalid,"padding signaling NaNs must not enter arithmetic");Check(bytes==original,"matrix bytes unchanged");
    }
}
struct Page{unsigned char* memory{};size_t size{};Page(){SYSTEM_INFO info{};GetSystemInfo(&info);size=info.dwPageSize;memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,size*2,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));Check(memory,"guard allocate");DWORD old{};Check(VirtualProtect(memory+size,size,PAGE_NOACCESS,&old),"guard protect");}~Page(){if(memory)VirtualFree(memory,0,MEM_RELEASE);}unsigned char* End(){return memory+size;}};
void Guarded(){for(const auto& b:backends)for(bool point:{false,true}){Page source,destination,matrix;const RwV3d input{1,2,3};const auto m=Matrix();auto* in=reinterpret_cast<RwV3d*>(source.End()-12);auto* out=reinterpret_cast<RwV3d*>(destination.End()-12);
        std::memcpy(in,&input,12);const auto size=point?64:48;auto* raw=reinterpret_cast<RwMatrix*>(matrix.End()-size);std::memcpy(raw,&m,size);
        std::memset(destination.End()-16,0xa7,16);Check((point?b.point:b.vector)(out,in,raw)==out,"guard single");Equal(*out,point?RwV3d{15,27,44}:RwV3d{5,7,14});Check(destination.End()[-13]==0xa7,"no write before destination");
        Check((point?b.points:b.vectors)(out,in,1,raw)==out,"guard batch1");Equal(*out,point?RwV3d{15,27,44}:RwV3d{5,7,14});Equal(*in,input);
    }
}
void Rounding(){RwMatrix m{};m.right={1,1,1};m.up={1,1,1};m.at={1,1,1};m.pos={1,1,1};const RwV3d in{16777216.f,1.f,-16777216.f};
    for(const auto& b:backends){RwV3d p{},v{},ps{},vs{};b.point(&p,&in,&m);b.vector(&v,&in,&m);b.points(&ps,&in,1,&m);b.vectors(&vs,&in,1,&m);Equal(p,{1,1,1});Equal(v,{0,0,0});Equal(ps,p);Equal(vs,v);}
    // This cancellation fixture distinguishes the SIMD association from a
    // naive double-precision affine expression, which would produce point2.
}
void Denormal(){const auto saved=_mm_getcsr();Check((saved&0x8000u)!=0,"engine FINALIZESTART enables FTZ");RwMatrix m{};m.right={0.5f,0.5f,0.5f};const RwV3d in{std::numeric_limits<float>::min(),0,0};
    for(const auto& b:backends)for(unsigned ftz:{0u,0x8000u}){const auto control=(saved&~(0x8000u|0x40u|0x3fu))|ftz;_mm_setcsr(control);RwV3d p{},v{},ps{},vs{};b.point(&p,&in,&m);b.vector(&v,&in,&m);b.points(&ps,&in,1,&m);b.vectors(&vs,&in,1,&m);const auto after=_mm_getcsr();_mm_setcsr(saved);
        const float expected=ftz?0.f:std::bit_cast<float>(0x00400000u);Equal(p,{expected,expected,expected});Equal(v,p);Equal(ps,p);Equal(vs,p);Check((after&~0x3fu)==(control&~0x3fu),"SIMD leaves MXCSR control unchanged");}
}
DWORD ProbeBatch(rwVectorsMultFn fn,RwV3d* out,const RwV3d* in,int count,const RwMatrix* m){__try{fn(out,in,count,m);return 0;}__except(EXCEPTION_EXECUTE_HANDLER){return GetExceptionCode();}}
void Zero(){auto m=Matrix();const std::array<RwV3d,4> input{{{1,2,3},{1,2,3},{1,2,3},{1,2,3}}};for(const auto& b:backends)for(bool point:{false,true})for(int count:{0,-1}){Page destination;auto* out=reinterpret_cast<RwV3d*>(destination.End()-12);
        Check(ProbeBatch(point?b.points:b.vectors,out,input.data(),count,&m)==EXCEPTION_ACCESS_VIOLATION,"nonpositive SIMD count faults at guard");Equal(*out,point?RwV3d{15,27,44}:RwV3d{5,7,14});
    }
}
void Dispatch(){const auto offset=RwEngineGetPluginOffset(rwID_VECTORMODULE);Check(offset>=0,"vector module");const auto* slots=static_cast<const unsigned char*>(RwEngineInstance)+offset+8;
    std::array<std::uintptr_t,4> pointers{};std::memcpy(pointers.data(),slots,sizeof(pointers));const Backend* selected=nullptr;
    for(const auto& b:backends)if(pointers==std::array<std::uintptr_t,4>{reinterpret_cast<std::uintptr_t>(b.point),reinterpret_cast<std::uintptr_t>(b.points),reinterpret_cast<std::uintptr_t>(b.vector),reinterpret_cast<std::uintptr_t>(b.vectors)})selected=&b;
    Check(selected,"current SSE-capable host must install one audited SSE family");Check(selected==&backends[(_rwIntelSSE2supported()&&!_rwAMD3DNowSupported())?1:0],"P4 requires SSE2 and no3DNow; no vendor-name comparison");
    auto m=Matrix();std::array<RwV3d,2> input{{{1,2,3},{-1,0,2}}},out{},expected{};
    RwV3dTransformPoints(out.data(),input.data(),2,&m);selected->points(expected.data(),input.data(),2,&m);Check(std::memcmp(out.data(),expected.data(),sizeof(out))==0,"public points dispatch");
    RwV3dTransformVectors(out.data(),input.data(),2,&m);selected->vectors(expected.data(),input.data(),2,&m);Check(std::memcmp(out.data(),expected.data(),sizeof(out))==0,"public vectors dispatch");
    std::printf("public vector backend %s\n",selected->name);
}
struct Scene{RpClump* clump=RpClumpCreate();RwFrame* root=RwFrameCreate();Scene(){Check(clump&&root,"clump/root");RpClumpSetFrame(clump,root);const RwV3d parent{10,20,30};RwFrameTranslate(root,&parent,rwCOMBINEREPLACE);
    for(int i=0;i<2;++i){auto* atomic=RpAtomicCreate();auto* frame=RwFrameCreate();auto* geometry=RpGeometryCreate(2,0,rpGEOMETRYPOSITIONS);Check(atomic&&frame&&geometry,"atomic geometry");const RwV3d vertices[]{{0,0,0},{1,2,3}};std::memcpy(RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geometry,0)),vertices,sizeof(vertices));RpGeometryUnlock(geometry);RpAtomicSetGeometry(atomic,geometry,0);RpGeometryDestroy(geometry);RpAtomicSetFrame(atomic,frame);RwFrameAddChild(root,frame);RpClumpAddAtomic(clump,atomic);const RwV3d local{i?5.f:-2.f,i?-3.f:4.f,i?1.f:-1.f};RwFrameTranslate(frame,&local,rwCOMBINEREPLACE);}}
    ~Scene(){if(clump)RpClumpDestroy(clump);}};
RwBBox EmptyBox(){const auto m=std::numeric_limits<float>::max();return {{-m,-m,-m},{m,m,m}};}
void Game(){Scene scene;std::uint32_t direct=0,secondary=99;pal4::NativeAtomicBoundsRuntime atomics(direct,secondary);pal4::NativeClumpBoundingBoxRuntime runtime(secondary,atomics);
    for(unsigned mode:{0u,7u}){auto box=EmptyBox();Check(pal4::ClumpBoundingBoxBuilder::Build(Address(scene.clump),mode,Address(&box),runtime)==Address(scene.clump),"real clump traversal return");Check(secondary==mode,"shared mode retained after traversal");Equal(box.inf,mode?RwV3d{8,17,29}:RwV3d{-2,-3,-1});Equal(box.sup,mode?RwV3d{16,26,34}:RwV3d{6,6,4});}
    auto* empty=RpClumpCreate();Check(empty,"empty clump");auto box=EmptyBox(),before=box;Check(pal4::ClumpBoundingBoxBuilder::Build(Address(empty),3,Address(&box),runtime)==Address(empty)&&secondary==3&&std::memcmp(&box,&before,sizeof(box))==0,"empty clump has no callbacks, mode persists");RpClumpDestroy(empty);
}
void Import(){static_assert(D3D_SDK_VERSION==32);Check(Direct3DCreate9(0)==nullptr,"unsupported SDK version rejected");auto* d3d=Direct3DCreate9(D3D_SDK_VERSION);Check(d3d&&d3d->GetAdapterCount()>0,"public D3D9 import creates real object");D3DADAPTER_IDENTIFIER9 id{};Check(SUCCEEDED(d3d->GetAdapterIdentifier(D3DADAPTER_DEFAULT,0,&id)),"adapter query");
    auto* older=Direct3DCreate9(31);Check(older&&older->GetAdapterCount()>0,"available RW archive requests31, original requests32; both accepted by this runtime");older->Release();
    auto module=GetModuleHandleA("d3d9.dll");Check(module,"loaded D3D9 module");auto exported=GetProcAddress(module,"Direct3DCreate9");auto linked=reinterpret_cast<FARPROC>(&Direct3DCreate9);
    if(linked!=exported){const auto* code=reinterpret_cast<const unsigned char*>(linked);Check(code[0]==0xff&&code[1]==0x25,"x86 linker import jump");std::uint32_t slot{};std::memcpy(&slot,code+2,4);std::memcpy(&linked,reinterpret_cast<const void*>(slot),4);}
    Check(exported&&linked==exported,"public import IAT resolves to loaded DLL export");char path[MAX_PATH]{};Check(GetModuleFileNameA(module,path,MAX_PATH)>0,"D3D9 runtime path");std::printf("D3D9 SDK=%u adapter=%s runtime=%s\n",D3D_SDK_VERSION,id.Description,path);d3d->Release();}
}
int main(int argc,char** argv){try{Check(argc==2,"one case required");const std::string_view c=argv[1];Check(IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE),"SSE-required diagnostic host");
    if(c=="import")Import();else{Session session;if(c=="cpu")Cpu();else if(c=="alignment")Alignment();else if(c=="guarded")Guarded();else if(c=="rounding")Rounding();else if(c=="denormal")Denormal();else if(c=="zero")Zero();else if(c=="dispatch")Dispatch();else if(c=="game")Game();else throw std::runtime_error("unknown case");}
    Check(allocations.empty()&&!invalid_free,"all engine allocations released once");std::printf("simd %s passed\n",argv[1]);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"simd failed: %s\n",e.what());return 1;}}
