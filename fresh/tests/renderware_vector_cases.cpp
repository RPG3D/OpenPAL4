#include "renderware_vector_cases.h"
#include "pal4/renderware_image_native.h"
#include <windows.h>
#include <rwcore.h>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <type_traits>

namespace pal4::test {
namespace {
static_assert(std::is_same_v<decltype(&RwV3dNormalize),RwReal (__cdecl *)(RwV3d*,const RwV3d*)>);
static_assert(std::is_same_v<decltype(&RwV3dTransformPoints),RwV3d* (__cdecl *)(RwV3d*,const RwV3d*,RwInt32,const RwMatrix*)>);
static_assert(offsetof(RwGlobals,stdFunc)+rwSTANDARDIMAGEGETRASTER*sizeof(RwStandardFunc)==96);
static_assert(offsetof(RwGlobals,stdFunc)+rwSTANDARDRASTERSETIMAGE*sizeof(RwStandardFunc)==100);
static_assert(offsetof(RwGlobals,stdFunc)+rwSTANDARDIMAGEFINDRASTERFORMAT*sizeof(RwStandardFunc)==108);
void Require(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
void Near(float a,float b,const char* message) { Require(std::fabs(a-b)<0.0002f,message); }
void Vector(const RwV3d& value,float x,float y,float z) { Near(value.x,x,"vector x"); Near(value.y,y,"vector y"); Near(value.z,z,"vector z"); }
void Transform() {
    RwMatrix matrix{}; RwMatrixSetIdentity(&matrix);
    matrix.right={2,1,0}; matrix.up={0,3,1}; matrix.at={1,0,4}; matrix.pos={10,20,30}; RwMatrixUpdate(&matrix);
    const RwV3d input{1,2,3}; RwV3d point{},vector{};
    Require(RwV3dTransformPoint(&point,&input,&matrix)==&point,"point return"); Vector(point,15,27,44);
    Require(RwV3dTransformVector(&vector,&input,&matrix)==&vector,"vector return"); Vector(vector,5,7,14);
    point=input; RwV3dTransformPoint(&point,&point,&matrix); Vector(point,15,27,44);
    vector=input; RwV3dTransformVector(&vector,&vector,&matrix); Vector(vector,5,7,14);
    std::array<RwV3d,3> points{{{1,2,3},{-1,0,2},{0,0,0}}},vectors=points;
    Require(RwV3dTransformPoints(points.data(),points.data(),3,&matrix)==points.data(),"points in-place return");
    Vector(points[0],15,27,44); Vector(points[1],10,19,38); Vector(points[2],10,20,30);
    RwV3dTransformVectors(vectors.data(),vectors.data(),3,&matrix);
    Vector(vectors[0],5,7,14); Vector(vectors[1],0,-1,8); Vector(vectors[2],0,0,0);
    // Partial overlapping arrays are not promised; exact in-place is covered.
}
DWORD ProbeZeroBatch(RwV3d* output,const RwV3d* input,const RwMatrix* matrix) {
    // Diagnostic isolation only. The production adapter does not intercept
    // exceptions or change SDK count semantics. Guarded pages prevent corruption
    // of other state when the selected SIMD loop executes despite count zero.
    __try { RwV3dTransformPoints(output,input,0,matrix); return 0; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
}
void ZeroBatch() {
    SYSTEM_INFO info{}; GetSystemInfo(&info); const auto page=info.dwPageSize;
    auto* memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,2*page,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    Require(memory!=nullptr,"diagnostic pages allocate");
    DWORD old{};
    if(!VirtualProtect(memory+page,page,PAGE_NOACCESS,&old)) { VirtualFree(memory,0,MEM_RELEASE); Require(false,"diagnostic guard setup"); }
    auto* output=reinterpret_cast<RwV3d*>(memory+page-24);
    const std::array<RwV3d,8> input{}; RwMatrix matrix{}; RwMatrixSetIdentity(&matrix);
    matrix.pos={1,2,3}; RwMatrixUpdate(&matrix);
    const auto fault=ProbeZeroBatch(output,input.data(),&matrix);
    const bool wrote=output->x==1 && output->y==2 && output->z==3;
    const auto module=RwEngineGetPluginOffset(rwID_VECTORMODULE);
    Require(module>=0,"vector module offset");
    const auto callback=*reinterpret_cast<const std::uintptr_t*>(static_cast<const char*>(RwEngineInstance)+module+12);
    const auto rva=callback-reinterpret_cast<std::uintptr_t>(GetModuleHandleA(nullptr));
    VirtualFree(memory,0,MEM_RELEASE);
    std::printf("zero-batch callback RVA=%zx fault=%08lX wrote_first=%d\n",static_cast<size_t>(rva),fault,wrote);
    Require((fault==EXCEPTION_ACCESS_VIOLATION && wrote) || (fault==0 && !wrote),"unrecognized zero-batch behavior");
    // Different CPU dispatch may choose the safe scalar loop. Current result
    // is captured in the evidence report; neither behavior becomes a new shim.
}
void Normalize() {
    RwV3d input{3,4,12},out{}; Near(RwV3dLength(&input),13,"3D length");
    Near(RwV3dNormalize(&out,&input),13,"public normalize returns length");
    Vector(out,3.f/13,4.f/13,12.f/13);
    // Header-exposed SPI returns inverse length, unlike the public function.
    Near(_rwV3dNormalize(&out,&input),1.f/13,"SPI inverse length");
    input={3,4,0}; Near(RwV3dNormalize(&input,&input),5,"3D alias length"); Vector(input,.6f,.8f,0);
    RwV2d two{3,4}; Near(RwV2dLength(&two),5,"2D length");
    Near(RwV2dNormalize(&two,&two),5,"2D alias length"); Near(two.x,.6f,"2D unit x"); Near(two.y,.8f,"2D unit y");
    RwError error{}; RwErrorGet(&error);
    input={0,0,0}; out={9,9,9};
    Require(RwV3dNormalize(&out,&input)==0,"zero length return"); Vector(out,0,0,0);
    RwErrorGet(&error); Require(error.pluginID==1 && error.errorCode!=0,"zero normalize must report core error");
    two={0,0}; RwV2d zero_out{9,9};
    Require(RwV2dNormalize(&zero_out,&two)==0 && zero_out.x==0 && zero_out.y==0,"2D zero output");
    RwErrorGet(&error); Require(error.pluginID==1 && error.errorCode!=0,"2D zero error");
}
void Approximation() {
    bool differs=false;
    for(float value : {1e-12f,.01f,.1f,.7f,1.f,1.0002f,2.f,3.f,10.f,123.456f,1e12f}) {
        const float exact=std::sqrt(value),root=_rwSqrt(value),inverse=_rwInvSqrt(value);
        Require(std::fabs(root/exact-1)<.0002f && std::fabs(inverse*exact-1)<.0002f,"table approximation error");
        differs |= std::bit_cast<unsigned>(root)!=std::bit_cast<unsigned>(exact);
    }
    Require(differs,"SDK table path unexpectedly identical to sqrt for all probes");
    Require(_rwSqrt(0)==0 && _rwInvSqrt(0)==0,"zero table special case");
    // Invalid/denormal/nonfinite input is not granted std::sqrt semantics.
}
RwBool Reject(void*,void*,RwInt32) { return FALSE; }
struct StandardOverride {
    RwStandardFunc& slot; RwStandardFunc saved;
    explicit StandardOverride(int index):slot(static_cast<RwGlobals*>(RwEngineInstance)->stdFunc[index]),saved(slot) { slot=Reject; }
    ~StandardOverride() { slot=saved; }
};
void RasterBridge() {
    auto* image=RwImageCreate(2,2,32); Require(image && RwImageAllocatePixels(image),"bridge image create");
    const RwRGBA color{17,51,102,255};
    for(int y=0;y<2;++y) for(int x=0;x<2;++x)
        reinterpret_cast<RwRGBA*>(image->cpPixels+y*image->stride)[x]=color;
    int width=-1,height=-1,depth=-1,format=-1;
    Require(RwImageFindRasterFormat(image,rwRASTERTYPETEXTURE,&width,&height,&depth,&format)==image,"format selection failed");
    Require(width==2 && height==2 && depth>0 && (format&rwRASTERTYPEMASK)==rwRASTERTYPETEXTURE,"format output fields");
    auto* raster=RwRasterCreate(2,2,32,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888);
    auto* output=RwImageCreate(2,2,32); Require(raster && output && RwImageAllocatePixels(output),"bridge objects create");
    image->flags|=rwIMAGEGAMMACORRECTED;
    Require(RwRasterSetFromImage(raster,image)==raster && (raster->privateFlags&rwRASTERGAMMACORRECTED),"upload gamma propagation");
    NativeRenderWareImageApi api;
    Require(api.SetFromRaster(reinterpret_cast<unsigned>(output),reinterpret_cast<unsigned>(raster))==reinterpret_cast<unsigned>(output),"native image adapter readback");
    Require((output->flags&rwIMAGEGAMMACORRECTED) && (output->flags&rwIMAGEALLOCATED),"readback preserves allocation and copies gamma flag");
    auto pixel=*reinterpret_cast<RwRGBA*>(output->cpPixels);
    Require(std::memcmp(&pixel,&color,sizeof(color))==0,"real raster RGBA readback");
    image->flags &= ~rwIMAGEGAMMACORRECTED;
    Require(RwRasterSetFromImage(raster,image) && (raster->privateFlags&rwRASTERGAMMACORRECTED),"upload must not clear prior gamma flag");
    raster->privateFlags &= ~rwRASTERGAMMACORRECTED;
    Require(RwImageSetFromRaster(output,raster) && (output->flags&rwIMAGEGAMMACORRECTED),"readback must not clear prior gamma flag");
    { StandardOverride reject(rwSTANDARDIMAGEGETRASTER); Require(!RwImageSetFromRaster(output,raster),"image failure propagation"); }
    { StandardOverride reject(rwSTANDARDRASTERSETIMAGE); Require(!RwRasterSetFromImage(raster,image),"raster failure propagation"); }
    { StandardOverride reject(rwSTANDARDIMAGEFINDRASTERFORMAT);
      width=height=depth=format=-7;
      Require(!RwImageFindRasterFormat(image,rwRASTERTYPETEXTURE,&width,&height,&depth,&format),"format failure propagation");
      Require(width==-7 && height==-7 && depth==-7 && format==-7,"failed format must leave outputs untouched"); }
    RwImageDestroy(output); RwRasterDestroy(raster); RwImageDestroy(image);
}
}
void RunVectorCase(std::string_view name) {
    if(name=="vector_transform") Transform();
    else if(name=="vector_zero_batch") ZeroBatch();
    else if(name=="vector_normalize") Normalize();
    else if(name=="vector_approximation") Approximation();
    else if(name=="vector_raster_bridge") RasterBridge();
    else if(name=="vector_world_map") RunWorldMapVectorCase();
    else Require(false,"unknown vector case");
}
}
