#include "renderware_raster_render_cases.h"
#include <d3d9.h>
#include <rwcore.h>
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace pal4::test {
namespace {
void Require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Drop {void operator()(RwRaster* p)const{if(p)RwRasterDestroy(p);}};
using Raster=std::unique_ptr<RwRaster,Drop>;
Raster Make(int w=16,int h=16,int type=rwRASTERTYPETEXTURE,int format=rwRASTERFORMAT8888){
    Raster p(RwRasterCreate(w,h,0,type|format));Require(p!=nullptr,"raster create");return p;
}
struct Context {
    RwRaster* value;RwRaster* before=RwRasterGetCurrentContext();
    explicit Context(RwRaster* r):value(r){Require(RwRasterPushContext(r)==r,"push context");}
    ~Context(){RwRasterPopContext();}
};
void Fill(RwRaster* r,RwUInt32 value){Context c(r);Require(RwRasterClear(value),"raster clear");}
RwUInt32 Pixel(RwRaster* raster,int x,int y){
    auto* p=RwRasterLock(raster,0,rwRASTERLOCKREAD);Require(p,"pixel lock");
    const auto value=*reinterpret_cast<RwUInt32*>(p+y*raster->stride+4*x);RwRasterUnlock(raster);return value;
}
void Color(RwUInt32 actual,RwUInt32 expected,int tolerance=0){
    for(int shift:{0,8,16}){int delta=int((actual>>shift)&255)-int((expected>>shift)&255);Require(delta>=-tolerance&&delta<=tolerance,"raster pixel RGB");}
}
void Set(RwRenderState state,RwUInt32 v){Require(RwRenderStateSet(state,reinterpret_cast<void*>(v)),"set RW state");}
RwUInt32 Get(RwRenderState state){RwUInt32 v{};Require(RwRenderStateGet(state,&v),"get RW state");return v;}
void ClearMemory(){
    auto raster=Make();Fill(raster.get(),0xff102030);RwRect rect{3,4,5,6};
    {Context c(raster.get());Require(RwRasterClearRect(&rect,0xffa0b0c0),"managed rect clear");}
    auto* p=RwRasterLock(raster.get(),0,rwRASTERLOCKREAD);Require(p,"clear read");
    for(int y=0;y<16;++y)for(int x=0;x<16;++x)Require(*reinterpret_cast<RwUInt32*>(p+y*raster->stride+4*x)==((x>=3&&x<8&&y>=4&&y<10)?0xffa0b0c0:0xff102030),"managed clear bounds/value");RwRasterUnlock(raster.get());
    auto sixteen=Make(8,8,rwRASTERTYPETEXTURE,rwRASTERFORMAT565);
    Require(sixteen->depth==16,"565 depth");Fill(sixteen.get(),0xf81f);
    p=RwRasterLock(sixteen.get(),0,rwRASTERLOCKREAD);Require(p,"565 read");
    for(int y=0;y<8;++y)for(int x=0;x<8;++x)Require(*reinterpret_cast<RwUInt16*>(p+y*sixteen->stride+2*x)==0xf81f,"16-bit clear uses native packed pixel");RwRasterUnlock(sixteen.get());
}
void ClearContext(){
    auto parent=Make(16,16,rwRASTERTYPECAMERATEXTURE);Fill(parent.get(),0xff102030);
    auto child=Make(0,0,rwRASTERTYPECAMERATEXTURE|rwRASTERDONTALLOCATE,rwRASTERFORMATDEFAULT);
    RwRect sub{5,6,4,3};Require(RwRasterSubRaster(child.get(),parent.get(),&sub)==child.get(),"clear child");
    auto* before=RwRasterGetCurrentContext();
    {Context p(parent.get());{Context c(child.get());Require(RwRasterClear(0xffc08040),"child full clear");}Require(RwRasterGetCurrentContext()==parent.get(),"pop restores parent context");}
    Require(RwRasterGetCurrentContext()==before,"pop restores caller context");
    Color(Pixel(parent.get(),5,6),0xffc08040);Color(Pixel(parent.get(),8,8),0xffc08040);Color(Pixel(parent.get(),4,6),0xff102030);
}
void Draw(bool scaled){
    auto source=Make(4,4);Fill(source.get(),0xffc02040);auto destination=Make(16,16,rwRASTERTYPECAMERATEXTURE);Fill(destination.get(),0xff102030);
    Context c(destination.get());
    if(scaled){RwRect rect{3,4,8,6};Require(RwRasterRenderScaled(source.get(),&rect)==source.get(),"scaled render");}
    else Require(RwRasterRender(source.get(),3,4)==source.get(),"unscaled render");
    Color(Pixel(destination.get(),4,5),0xffc02040);Color(Pixel(destination.get(),2,3),0xff102030);
    Color(Pixel(destination.get(),scaled?9:5,scaled?8:6),0xffc02040);
}
void AlphaAndState(){
    auto source=Make(4,4);Fill(source.get(),0x80ff0000);auto destination=Make(16,16,rwRASTERTYPECAMERATEXTURE);Fill(destination.get(),0xff0000ff);
    auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    IDirect3DSurface9* previous_target{};Require(SUCCEEDED(device->GetRenderTarget(0,&previous_target)),"initial target");
    D3DSURFACE_DESC default_desc{};Require(SUCCEEDED(previous_target->GetDesc(&default_desc)),"default target size");
    D3DVIEWPORT9 custom_viewport{2,3,10,11,0,1};Require(SUCCEEDED(device->SetViewport(&custom_viewport)),"custom viewport");
    Set(rwRENDERSTATESRCBLEND,rwBLENDONE);Set(rwRENDERSTATEDESTBLEND,rwBLENDZERO);
    Set(rwRENDERSTATEVERTEXALPHAENABLE,0);Set(rwRENDERSTATEFOGENABLE,1);
    Set(rwRENDERSTATETEXTUREFILTER,rwFILTERLINEAR);Set(rwRENDERSTATETEXTUREADDRESSU,rwTEXTUREADDRESSWRAP);Set(rwRENDERSTATETEXTUREADDRESSV,rwTEXTUREADDRESSMIRROR);
    const std::array<RwRenderState,7> states{rwRENDERSTATESRCBLEND,rwRENDERSTATEDESTBLEND,rwRENDERSTATEVERTEXALPHAENABLE,rwRENDERSTATEFOGENABLE,rwRENDERSTATETEXTUREFILTER,rwRENDERSTATETEXTUREADDRESSU,rwRENDERSTATETEXTUREADDRESSV};
    std::array<RwUInt32,7> values{};for(size_t i=0;i<states.size();++i)values[i]=Get(states[i]);
    RwD3D9SetRenderState(D3DRS_ZENABLE,1);RwD3D9SetRenderState(D3DRS_CULLMODE,D3DCULL_CCW);
    {Context c(destination.get());Require(RwRasterRender(source.get(),1,1)==source.get(),"alpha render");}
    Color(Pixel(destination.get(),2,2),0xff80007f,1);
    for(size_t i=0;i<states.size();++i)Require(Get(states[i])==values[i],"RW preferences restored");
    RwUInt32 z{},cull{};RwD3D9GetRenderState(D3DRS_ZENABLE,&z);RwD3D9GetRenderState(D3DRS_CULLMODE,&cull);Require(z==1&&cull==D3DCULL_CCW,"raw desired depth/cull restored");
    DWORD actual_z{},actual_cull{};device->GetRenderState(D3DRS_ZENABLE,&actual_z);device->GetRenderState(D3DRS_CULLMODE,&actual_cull);
    Require(actual_z==0 && actual_cull==D3DCULL_NONE,"device retains draw state until deferred restoration flush");
    IDirect3DSurface9* current_target{};Require(SUCCEEDED(device->GetRenderTarget(0,&current_target)),"restored default target");
    Require(current_target==previous_target,"render restores default target on supported camera texture path");current_target->Release();previous_target->Release();
    D3DVIEWPORT9 viewport{};Require(SUCCEEDED(device->GetViewport(&viewport))&&viewport.X==0&&viewport.Y==0&&viewport.Width==default_desc.Width&&viewport.Height==default_desc.Height,"default target bind resets viewport instead of restoring caller viewport");
    {Context c(destination.get());Require(RwRasterRenderFast(source.get(),8,8)==source.get(),"fast render");}
    Color(Pixel(destination.get(),9,9),0xffff0000);
    // Fast retains caller factors. Raster alpha can still enable blending.
    Set(rwRENDERSTATESRCBLEND,rwBLENDSRCALPHA);Set(rwRENDERSTATEDESTBLEND,rwBLENDINVSRCALPHA);
    {Context c(destination.get());Require(RwRasterRenderFast(source.get(),8,1)==source.get(),"fast inherited alpha factors");}
    Color(Pixel(destination.get(),9,2),0xff80007f,1);
}
void CopyFailure(){
    auto source=Make(4,4);Fill(source.get(),0xffa03010);auto destination=Make(16,16);Fill(destination.get(),0xff102030);
    {Context c(destination.get());RwRect rect{5,6,4,4};
        Require(!RWSRCGLOBAL(stdFunc[rwSTANDARDRASTERRENDERFAST])(source.get(),&rect,0),"device standard rejects managed copy");
        Require(RwRasterRenderFast(source.get(),5,6)==source.get(),"public wrapper returns input even after failure");}
    Color(Pixel(destination.get(),0,0),0xff102030);Color(Pixel(destination.get(),5,6),0xff102030);
    auto probe=Make(16,16,rwRASTERTYPECAMERATEXTURE);Fill(probe.get(),0xff000000);
    {Context c(probe.get());Require(RwRasterRenderFast(destination.get(),0,0)==destination.get(),"sample copied texture");}
    Color(Pixel(probe.get(),0,0),0xff102030);Color(Pixel(probe.get(),5,6),0xff102030);
}
}
void RunRasterRenderCase(std::string_view name){
    if(name=="raster_render_clear_memory")ClearMemory();else if(name=="raster_render_clear_context")ClearContext();
    else if(name=="raster_render_unscaled")Draw(false);else if(name=="raster_render_scaled")Draw(true);
    else if(name=="raster_render_alpha_state")AlphaAndState();else if(name=="raster_render_copy_failure")CopyFailure();
    else throw std::runtime_error("unknown raster render case");
}
}
