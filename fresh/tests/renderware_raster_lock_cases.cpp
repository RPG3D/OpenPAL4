#include "renderware_raster_lock_cases.h"
#include <rwcore.h>
#include <d3d9.h>
#include <cstring>
#include <memory>
#include <stdexcept>

namespace pal4::test {
namespace {
void Require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Drop {void operator()(RwRaster* p)const{if(p)RwRasterDestroy(p);}};
using Raster=std::unique_ptr<RwRaster,Drop>;
Raster Make(int type=rwRASTERTYPETEXTURE,int format=rwRASTERFORMAT8888){
    Raster p(RwRasterCreate(16,8,32,type|format));Require(p!=nullptr,"raster create");return p;
}
void Fill(RwRaster* r,unsigned char value){
    auto* p=RwRasterLock(r,0,rwRASTERLOCKWRITE|rwRASTERLOCKNOFETCH);Require(p,"write lock");
    for(int y=0;y<r->height;++y)std::memset(p+y*r->stride,value,4*r->width);
    RwRasterUnlock(r);
}
void SubRaster(){
    auto parent=Make();Fill(parent.get(),0x11);
    Raster child(RwRasterCreate(0,0,0,rwRASTERTYPETEXTURE|rwRASTERDONTALLOCATE));Require(child!=nullptr,"child create");
    RwRect rectangle{3,2,5,3};Require(RwRasterSubRaster(child.get(),parent.get(),&rectangle)==child.get(),"subraster bind");
    auto* p=RwRasterLock(child.get(),0,rwRASTERLOCKREAD|rwRASTERLOCKWRITE);Require(p,"subraster lock");
    Require(child->width==5 && child->height==3 && child->stride>=16*4,"subraster uses parent pitch");
    for(int y=0;y<3;++y){for(int x=0;x<20;++x)Require(p[y*child->stride+x]==0x11,"subraster origin read");std::memset(p+y*child->stride,0x77,20);}
    RwRasterUnlock(child.get());Require(!child->cpPixels && child->stride==0 && child->width==5,"unlock restores child metadata");
    p=RwRasterLock(parent.get(),0,rwRASTERLOCKREAD);Require(p,"parent read");
    for(int y=0;y<8;++y)for(int x=0;x<16;++x)for(int c=0;c<4;++c)
        Require(p[y*parent->stride+4*x+c]==((x>=3&&x<8&&y>=2&&y<5)?0x77:0x11),"subraster write must preserve outside rectangle");
    RwRasterUnlock(parent.get());
}
void CameraTexture(){
    auto raster=Make(rwRASTERTYPECAMERATEXTURE);Fill(raster.get(),0x35);
    auto* p=RwRasterLock(raster.get(),0,rwRASTERLOCKREAD);Require(p,"camera texture GetRenderTargetData read");
    for(int y=0;y<8;++y)for(int x=0;x<64;++x)Require(p[y*raster->stride+x]==0x35,"camera texture UpdateSurface round trip");
    RwRasterUnlock(raster.get());
    Require(!raster->cpPixels && raster->stride==0 && !(raster->privateFlags&6),"camera texture releases staging metadata");
    // A read/write child updates only its rectangle on the destination surface.
    Raster child(RwRasterCreate(0,0,0,rwRASTERTYPECAMERATEXTURE|rwRASTERDONTALLOCATE));Require(child!=nullptr,"camera child");
    RwRect rect{2,1,4,2};Require(RwRasterSubRaster(child.get(),raster.get(),&rect)==child.get(),"camera child bind");
    p=RwRasterLock(child.get(),0,rwRASTERLOCKREAD|rwRASTERLOCKWRITE);Require(p,"camera child read/write");
    for(int y=0;y<2;++y)std::memset(p+y*child->stride,0x62,16);RwRasterUnlock(child.get());
    p=RwRasterLock(raster.get(),0,rwRASTERLOCKREAD);Require(p,"camera read after child update");
    for(int y=0;y<8;++y)for(int x=0;x<16;++x)Require(p[y*raster->stride+4*x]==((x>=2&&x<6&&y>=1&&y<3)?0x62:0x35),"camera child region update");
    RwRasterUnlock(raster.get());
}
void LockErrors(){
    auto raster=Make();auto* p=RwRasterLock(raster.get(),0,rwRASTERLOCKWRITE);Require(p,"initial lock");
    const auto pitch=raster->stride;Require(!RwRasterLock(raster.get(),0,rwRASTERLOCKREAD),"double lock rejected");
    Require(raster->cpPixels==p && raster->stride==pitch,"double lock preserves first lock");RwRasterUnlock(raster.get());
    auto callback=RWSRCGLOBAL(stdFunc[rwSTANDARDRASTERUNLOCK]);
    Require(!callback(nullptr,raster.get(),0),"device standard rejects unlock without lock");
    Require(RwRasterUnlock(raster.get())==raster.get(),"public unlock ignores device-standard failure");
    Require(!RwRasterLock(raster.get(),250,rwRASTERLOCKREAD) && !raster->cpPixels && raster->width==16,"invalid mip GetSurfaceLevel fails without lock metadata");
    Fill(raster.get(),0x28);
}
void PaletteRead(){
    auto raster=Make();Require(!raster->palette,"RGBA has no palette");
    // The SDK accepts this type without requiring PAL4/PAL8. Read-only avoids
    // the write-unlock path's unchecked palette-index dereference.
    Require(!RwRasterLockPalette(raster.get(),rwRASTERLOCKREAD),"RGBA palette returns null");
    Require((raster->privateFlags&8)!=0,"null palette still sets read lock flag");
    Require(RwRasterUnlockPalette(raster.get())==raster.get() && !(raster->privateFlags&24),"palette read unlock clears flags");
    auto depth=Make(rwRASTERTYPEZBUFFER,rwRASTERFORMATDEFAULT);
    Require(!RwRasterLockPalette(depth.get(),rwRASTERLOCKREAD) && !(depth->privateFlags&24),"depth palette type rejected");
}
int generated{};RwTextureCallBackMipmapGeneration original_generator{};
RwRaster* Generate(RwRaster* raster,RwImage* image){++generated;return original_generator(raster,image);}
void AutoMips(){
    auto raster=Make(rwRASTERTYPETEXTURE,rwRASTERFORMAT8888|rwRASTERFORMATMIPMAP|rwRASTERFORMATAUTOMIPMAP);
    original_generator=RwTextureGetMipmapGenerationCallBack();RwTextureSetMipmapGenerationCallBack(Generate);generated=0;
    Fill(raster.get(),0x44);const auto after_write=generated;
    auto* p=RwRasterLock(raster.get(),0,rwRASTERLOCKREAD);Require(p,"auto mip read");RwRasterUnlock(raster.get());
    Require(generated==after_write,"read unlock does not generate mips");
    if(RwRasterGetNumLevels(raster.get())>1){
        Require(after_write==1,"software generation runs once for base write");
        p=RwRasterLock(raster.get(),1,rwRASTERLOCKREAD);Require(p,"software generated mip");
        for(int y=0;y<raster->height;++y)for(int x=0;x<raster->width*4;++x)Require(p[y*raster->stride+x]==0x44,"software mip content");
        RwRasterUnlock(raster.get());
    }else{
        // D3D autogen exposes only level0; the generated chain is not CPU-lockable.
        Require(after_write==0 && !RwRasterLock(raster.get(),1,rwRASTERLOCKREAD),"hardware autogen bypasses software callback and hides mip levels");
    }
    RwTextureSetMipmapGenerationCallBack(original_generator);
}
}
void RunRasterLockCase(std::string_view name){
    if(name=="raster_lock_subraster")SubRaster();else if(name=="raster_lock_camera_texture")CameraTexture();
    else if(name=="raster_lock_errors")LockErrors();else if(name=="raster_lock_palette_read")PaletteRead();
    else if(name=="raster_lock_auto_mips")AutoMips();else throw std::runtime_error("unknown raster lock case");
}
}
