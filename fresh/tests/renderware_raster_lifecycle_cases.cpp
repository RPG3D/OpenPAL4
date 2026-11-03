#include "support/native_test_display.h"
#include "renderware_raster_lifecycle_cases.h"
#include "pal4/camera_resource_native.h"
#include <rwcore.h>
#include <d3d9.h>
#include <memory>
#include <stdexcept>

namespace pal4::test {
namespace {
void Require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
struct Drop {void operator()(RwRaster* p)const{if(p)RwRasterDestroy(p);}};
using Raster=std::unique_ptr<RwRaster,Drop>;
Raster Make(int w,int h,int flags,int depth=0){Raster p(RwRasterCreate(w,h,depth,flags));Require(p!=nullptr,"raster create");return p;}
void Fill(RwRaster* r,RwUInt32 value){auto* p=RwRasterLock(r,0,rwRASTERLOCKWRITE);Require(p,"fill lock");for(int y=0;y<r->height;++y)for(int x=0;x<r->width;++x)*reinterpret_cast<RwUInt32*>(p+y*r->stride+4*x)=value;RwRasterUnlock(r);}
RwUInt32 Pixel(RwRaster* r){auto* p=RwRasterLock(r,0,rwRASTERLOCKREAD);Require(p,"readback lock");auto v=*reinterpret_cast<RwUInt32*>(p);RwRasterUnlock(r);return v;}
void Formats(){
    auto a=Make(8,8,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888,16);Require(a->depth==32,"format determines depth over caller hint");
    auto b=Make(8,8,rwRASTERTYPETEXTURE|rwRASTERFORMAT565,32);Require(b->depth==16,"565 overrides32-bit hint");
    auto c=Make(8,8,rwRASTERTYPETEXTURE);Require(c->depth==32 && (RwRasterGetFormat(c.get())&rwRASTERFORMATPIXELFORMATMASK)==rwRASTERFORMAT8888,"default texture selects supported alpha format on this host");
    Require(!RwRasterCreate(8,8,4,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888|rwRASTERFORMATPAL4),"actual create rejects explicit PAL4");
    Require(!RwRasterCreate(8,8,0,3),"unsupported raster type rejected");
}
void ImageFormat(){
    auto* image=RwImageCreate(13,7,32);Require(image&&RwImageAllocatePixels(image),"format image");
    for(int y=0;y<7;++y)for(int x=0;x<13;++x)*reinterpret_cast<RwRGBA*>(image->cpPixels+y*image->stride+4*x)={20,50,90,128};
    RwInt32 w{},h{},depth{},flags{};
    Require(RwImageFindRasterFormat(image,rwRASTERTYPETEXTURE,&w,&h,&depth,&flags)==image,"image format probe");
    Require(depth==32 && (flags&rwRASTERFORMATPIXELFORMATMASK)==rwRASTERFORMAT8888,"image alpha retains8888");
    auto raster=Make(w,h,flags,depth);Require(RwRasterSetFromImage(raster.get(),image)==raster.get(),"probed format creates and imports");
    Require(Pixel(raster.get())==0x8014325a,"image-format import channel order");RwImageDestroy(image);
}
void NoAllocate(){
    const int flags=rwRASTERTYPETEXTURE|rwRASTERFORMAT8888|rwRASTERFORMATMIPMAP|rwRASTERDONTALLOCATE;
    auto root=Make(13,7,flags);Require(root->width==13&&root->height==7&&!root->cpPixels&&!root->stride,"noallocate preserves metadata without size adjustment");
    Require(RwRasterGetNumLevels(root.get())==4,"no-resource mip count uses root max dimension");
    auto child=Make(0,0,rwRASTERTYPETEXTURE|rwRASTERDONTALLOCATE);RwRect rect{2,1,5,3};
    Require(RwRasterSubRaster(child.get(),root.get(),&rect)==child.get(),"noallocate child");
    Require(child->depth==root->depth&&RwRasterGetFormat(child.get())==RwRasterGetFormat(root.get())&&RwRasterGetNumLevels(child.get())==4,"child uses root mip count and copies format");
    auto zero=Make(0,16,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888|rwRASTERFORMATMIPMAP);
    Require((zero->cFlags&rwRASTERDONTALLOCATE)&&!zero->stride&&RwRasterGetNumLevels(zero.get())==5,"zero area becomes noallocate and counts maximum dimension");
}
void AssignmentDestroy(){
    auto first=Make(8,8,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888);Fill(first.get(),0xff254769);
    auto second=Make(8,8,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888);auto* texture=RwTextureCreate(nullptr);Require(texture,"empty texture");
    Require(RwTextureSetRaster(texture,first.get())==texture && RwTextureSetRaster(texture,second.get())==texture,"texture raster replacement");
    Require(Pixel(first.get())==0xff254769,"raster setter does not destroy replaced raster");
    Require(RwD3D9SetTexture(texture,0)&&RwD3D9SetTexture(texture,7),"bind raster on multiple stages");
    auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    RwTextureSetRaster(texture,nullptr);second.reset();
    for(DWORD stage:{0u,7u}){IDirect3DBaseTexture9* t{};Require(SUCCEEDED(device->GetTexture(stage,&t))&&!t,"raster destruction clears all matching stage bindings");}
    RwRaster* current{};Require(RwRenderStateGet(rwRENDERSTATETEXTURERASTER,&current)&&!current,"destroy updates raster cache");RwTextureDestroy(texture);
    auto child=Make(0,0,rwRASTERTYPETEXTURE|rwRASTERDONTALLOCATE);RwRect rect{1,1,3,3};RwRasterSubRaster(child.get(),first.get(),&rect);child.reset();
    Require(Pixel(first.get())==0xff254769,"child destruction preserves parent resource");
}
struct Camera {
    RwCamera* value=RwCameraCreate();RwFrame* frame=RwFrameCreate();
    Camera(int w,int h,int type){Require(value&&frame,"camera create");RwCameraSetFrame(value,frame);auto color=Make(w,h,type);auto depth=Make(w,h,rwRASTERTYPEZBUFFER);RwCameraSetRaster(value,color.release());RwCameraSetZRaster(value,depth.release());}
    ~Camera(){auto* color=value->frameBuffer;auto* depth=value->zBuffer;RwCameraSetRaster(value,nullptr);RwCameraSetZRaster(value,nullptr);RwCameraSetFrame(value,nullptr);RwCameraDestroy(value);RwFrameDestroy(frame);RwRasterDestroy(color);RwRasterDestroy(depth);}
    void Clear(RwRGBA color){Require(RwCameraClear(value,&color,rwCAMERACLEARIMAGE|rwCAMERACLEARZ)==value,"camera clear");}
};
void Resize(HWND window,Camera& main){Require(SetWindowPos(window,nullptr,0,0,260,190,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE),"resize window");main.Clear({31,47,63,255});}
void ResetGame(HWND window){
    RECT size{};GetClientRect(window,&size);Camera main(size.right,size.bottom,rwRASTERTYPECAMERA);
    auto managed=Make(16,8,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888);Fill(managed.get(),0xff214365);
    NativeCameraResourceRuntime runtime;const auto handle=CameraResourceHelpers::Create(16,runtime);Require(handle,"game camera create");
    auto* game=reinterpret_cast<RwCamera*>(handle);auto* owned_depth=game->zBuffer;
    auto color=Make(16,16,rwRASTERTYPECAMERATEXTURE|rwRASTERFORMAT8888);RwCameraSetRaster(game,color.get());
    Require(CameraResourceHelpers::SetClipAndWindow(handle,10,runtime)==handle,"game clip setup");RwRGBA before{21,43,65,255};Require(RwCameraClear(game,&before,rwCAMERACLEARIMAGE|rwCAMERACLEARZ)==game,"game clear before reset");
    Resize(window,main);
    Require(Pixel(managed.get())==0xff214365,"managed content survives reset");
    Require(game->zBuffer==owned_depth && owned_depth->width==16 && owned_depth->height==16,"game owned-depth wrapper identity and size survive reset");
    RwRGBA after{91,123,155,255};Require(RwCameraClear(game,&after,rwCAMERACLEARIMAGE|rwCAMERACLEARZ)==game,"game resources usable after reset");
    Require((Pixel(color.get())&0xffffff)==0x5b7b9b,"restored camera texture accepts new content");
    Require(CameraResourceHelpers::Destroy(handle,runtime)==1,"game destroy after reset");
    Require((Pixel(color.get())&0xffffff)==0x5b7b9b,"game cleanup preserves borrowed color");
}
struct Window {HWND value=pal4::test::CreateTestWindow("STATIC","secondary raster reset",WS_OVERLAPPEDWINDOW,180,140,nullptr,nullptr,GetModuleHandleA(nullptr),nullptr);~Window(){if(value)DestroyWindow(value);}};
void SecondaryReset(HWND window){
    RECT size{};GetClientRect(window,&size);Camera main(size.right,size.bottom,rwRASTERTYPECAMERA);Window second;Require(second.value,"secondary window");
    Camera attached(16,16,rwRASTERTYPECAMERATEXTURE|rwRASTERFORMAT8888);Require(RwD3D9CameraAttachWindow(attached.value,second.value),"attach secondary chain");attached.Clear({24,48,72,255});
    Resize(window,main);attached.Clear({96,128,160,255});
    Require((Pixel(attached.value->frameBuffer)&0xffffff)==0x6080a0,"secondary chain restored across main reset");
}
}
void RunRasterLifecycleCase(std::string_view name,HWND window){
    if(name=="raster_lifecycle_formats")Formats();else if(name=="raster_lifecycle_image_format")ImageFormat();else if(name=="raster_lifecycle_noallocate")NoAllocate();
    else if(name=="raster_lifecycle_ownership")AssignmentDestroy();else if(name=="raster_lifecycle_game_reset")ResetGame(window);else if(name=="raster_lifecycle_secondary_reset")SecondaryReset(window);
    else throw std::runtime_error("unknown raster lifecycle case");
}
}
