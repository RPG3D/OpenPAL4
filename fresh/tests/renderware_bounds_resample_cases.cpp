#include "renderware_bounds_resample_cases.h"
#include "pal4/atomic_bounds_native.h"
#include <rwcore.h>
#include <rpworld.h>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>

namespace pal4::test {
namespace {
void Require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
template<class T> std::uint32_t Address(T* p){return reinterpret_cast<std::uint32_t>(p);}
void Equal(const RwV3d& a,const RwV3d& b){Require(std::fabs(a.x-b.x)<0.0001f&&std::fabs(a.y-b.y)<0.0001f&&std::fabs(a.z-b.z)<0.0001f,"bounds vector mismatch");}
RwBBox Empty(){const auto f=std::numeric_limits<float>::max();return {{-f,-f,-f},{f,f,f}};}
void Bounds(){
    const RwV3d points[]{{-3,7,1},{4,-2,9},{0,5,-6}};RwBBox box{};
    Require(RwBBoxCalculate(&box,points,3)==&box,"box calculate return");Equal(box.inf,{-3,-2,-6});Equal(box.sup,{4,7,9});
    const RwV3d extra{10,1,-8};Require(RwBBoxAddPoint(&box,&extra)==&box,"box add return");Equal(box.inf,{-3,-2,-8});Equal(box.sup,{10,7,9});
    RwV3d zero{0.0f,-0.0f,0.0f};RwBBoxCalculate(&box,&zero,1);const RwV3d other{-0.0f,0.0f,-0.0f};RwBBoxAddPoint(&box,&other);
    Require(std::bit_cast<std::uint32_t>(box.inf.x)==0&&std::bit_cast<std::uint32_t>(box.sup.y)==0x80000000u,"equal signed zeros retain first point");
}
struct Scene {
    RpClump* clump{};RpAtomic* atomic{};
    Scene(){
        clump=RpClumpCreate();atomic=RpAtomicCreate();auto* parent=RwFrameCreate();auto* child=RwFrameCreate();auto* geometry=RpGeometryCreate(3,0,rpGEOMETRYPOSITIONS);
        Require(clump&&atomic&&parent&&child&&geometry,"bounds scene create");
        const RwV3d vertices[]{{-1,2,0},{3,-2,4},{0,1,-3}};std::memcpy(RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geometry,0)),vertices,sizeof(vertices));RpGeometryUnlock(geometry);
        RpAtomicSetGeometry(atomic,geometry,0);RpGeometryDestroy(geometry);RpClumpSetFrame(clump,parent);RpAtomicSetFrame(atomic,child);RwFrameAddChild(parent,child);RpClumpAddAtomic(clump,atomic);
        const RwV3d translation{10,20,30},scale{2,3,1},local{5,7,9};RwFrameTranslate(parent,&translation,rwCOMBINEREPLACE);RwFrameScale(child,&scale,rwCOMBINEREPLACE);RwFrameTranslate(child,&local,rwCOMBINEPOSTCONCAT);
    }
    ~Scene(){if(clump)RpClumpDestroy(clump);}
};
void GameVertices(){
    Scene scene;std::uint32_t direct=1,secondary=0;NativeAtomicBoundsRuntime runtime(direct,secondary);auto box=Empty();
    Require(AtomicBoundingBoxBuilder::AddTransformedVertices(Address(scene.atomic),Address(&box),runtime)==Address(scene.atomic),"game vertex return");Equal(box.inf,{13,21,36});Equal(box.sup,{21,33,43});
    box=Empty();Require(AtomicBoundingBoxBuilder::AddTransformedVerticesSecondary(Address(scene.atomic),Address(&box),runtime)==Address(scene.atomic),"game relative return");Equal(box.inf,{3,1,6});Equal(box.sup,{11,13,13});
    direct=0;box=Empty();(void)AtomicBoundingBoxBuilder::AddTransformedVertices(Address(scene.atomic),Address(&box),runtime);Equal(box.inf,{3,1,6});
    box=Empty();(void)AtomicBoundingBoxBuilder::ProcessBoundingBox(Address(scene.atomic),1,Address(&box),runtime);Equal(box.sup,{21,33,43});
    box=Empty();(void)AtomicBoundingBoxBuilder::ProcessBoundingBox(Address(scene.atomic),0,Address(&box),runtime);Equal(box.sup,{11,13,13});
}
void GameCorners(){
    Scene scene;std::uint32_t direct=1,secondary=0;NativeAtomicBoundsRuntime runtime(direct,secondary);
    RwBBox boxes[2]{{{3,2,4},{-1,-2,-3}},{{0,0,0},{0,0,0}}};
    Require(AtomicBoundingBoxBuilder::UpdateTransformedBox(Address(boxes),Address(scene.atomic),runtime)==Address(&boxes[1]),"game transformed box pointer");Equal(boxes[1].inf,{13,21,36});Equal(boxes[1].sup,{21,33,43});Equal(boxes[0].inf,{-1,-2,-3});
    Require(!AtomicBoundingBoxBuilder::UpdateTransformedBox(Address(boxes),0,runtime),"game null atomic gate");
}
struct Drop {void operator()(RwImage* p)const{if(p)RwImageDestroy(p);}};
using Image=std::unique_ptr<RwImage,Drop>;
Image Make(int w,int h,int depth=32){Image image(RwImageCreate(w,h,depth));Require(image&&RwImageAllocatePixels(image.get()),"image create");return image;}
RwRGBA& At(RwImage* i,int x,int y){return reinterpret_cast<RwRGBA*>(i->cpPixels+y*i->stride)[x];}
void Near(RwRGBA c,int r,int g,int b,int a){Require(std::abs(int(c.red)-r)<=1&&std::abs(int(c.green)-g)<=1&&std::abs(int(c.blue)-b)<=1&&std::abs(int(c.alpha)-a)<=1,"resampled pixel mismatch");}
void ResampleAreas(){
    auto source=Make(3,1);At(source.get(),0,0)={0,0,0,0};At(source.get(),1,0)={90,60,30,90};At(source.get(),2,0)={180,120,60,180};source->flags|=rwIMAGEGAMMACORRECTED;
    auto dest=Make(2,1);Require(RwImageResample(dest.get(),source.get())==dest.get(),"resample destination return");Near(At(dest.get(),0,0),30,20,10,30);Near(At(dest.get(),1,0),150,100,50,150);Require(dest->flags&rwIMAGEGAMMACORRECTED,"gamma flag propagated");
    auto pair=Make(2,1);At(pair.get(),0,0)={255,0,0,0};At(pair.get(),1,0)={0,0,255,255};Image average(RwImageCreateResample(pair.get(),1,1));Require(average!=nullptr,"average create");Near(At(average.get(),0,0),128,0,128,128); // Independent RGBA average, not premultiplied alpha.
    Image enlarged(RwImageCreateResample(pair.get(),4,2));Require(enlarged!=nullptr,"enlarged image");for(int y=0;y<2;++y)for(int x=0;x<4;++x)Near(At(enlarged.get(),x,y),x<2?255:0,0,x<2?0:255,x<2?0:255);
    auto square=Make(4,4);for(int y=0;y<4;++y)for(int x=0;x<4;++x)At(square.get(),x,y)={static_cast<RwUInt8>(40*x),static_cast<RwUInt8>(40*y),80,160};Image reduced(RwImageCreateResample(square.get(),2,2));Require(reduced!=nullptr,"two-axis reduction");for(int y=0;y<2;++y)for(int x=0;x<2;++x)Near(At(reduced.get(),x,y),20+80*x,20+80*y,80,160);
    source->flags&=~rwIMAGEGAMMACORRECTED;RwImageResample(dest.get(),source.get());Require(dest->flags&rwIMAGEGAMMACORRECTED,"destination gamma flag is not cleared by unmarked source");
}
void ResamplePalette(){
    for(int depth:{4,8}){auto source=Make(2,1,depth);source->palette[1]={20,40,60,80};source->palette[2]={100,120,140,160};source->cpPixels[0]=1;source->cpPixels[1]=2;
        Image output(RwImageCreateResample(source.get(),1,1));Require(output&&output->depth==32&&!output->palette,"indexed resample expands to32bit");Near(At(output.get(),0,0),60,80,100,120);Require(source->cpPixels[0]==1&&source->cpPixels[1]==2&&source->palette[1].alpha==80,"source palette ownership preserved");}
}
void ResampleFailure(bool& fail_next){
    auto source=Make(2,2);for(int y=0;y<2;++y)for(int x=0;x<2;++x)At(source.get(),x,y)={10,20,30,40};fail_next=true;Image failed(RwImageCreateResample(source.get(),1,1));Require(!failed&&!fail_next,"resample allocation failure propagated");
    Image retry(RwImageCreateResample(source.get(),1,1));Require(retry!=nullptr,"resample allocation retry");Near(At(retry.get(),0,0),10,20,30,40);
}
}
void RegisterBoundsPlugin(){Require(RpWorldPluginAttach(),"world plugin for bounds geometry");}
void RunBoundsResampleCase(std::string_view name,bool& fail_next){
    if(name=="bounds_values")Bounds();else if(name=="bounds_game_vertices")GameVertices();else if(name=="bounds_game_corners")GameCorners();
    else if(name=="resample_areas")ResampleAreas();else if(name=="resample_palette")ResamplePalette();else if(name=="resample_failure")ResampleFailure(fail_next);else throw std::runtime_error("unknown bounds/resample case");
}
}
