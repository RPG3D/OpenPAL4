#include "renderware_native_format_cases.h"
#include <rwcore.h>
#include <d3d9.h>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <type_traits>

static_assert(std::is_same_v<decltype(&RwD3D9RasterCreate),RwRaster* (__cdecl *)(RwUInt32,RwUInt32,RwUInt32,RwUInt32)>);
static_assert(std::is_same_v<decltype(&_rwD3D9RasterConvertToNonPalettized),void (__cdecl *)(RwRaster*)>);
namespace pal4::test {
namespace {
void Require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Drop {void operator()(RwRaster* p)const{if(p)RwRasterDestroy(p);}};
using Raster=std::unique_ptr<RwRaster,Drop>;
Raster Make(D3DFORMAT format,int flags=rwRASTERTYPETEXTURE){Raster p(RwD3D9RasterCreate(8,8,format,flags));Require(p!=nullptr,"native format create");return p;}
IDirect3DDevice9* Device(){return static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());}
D3DSURFACE_DESC Describe(RwRaster* raster){
    auto* texture=RwTextureCreate(nullptr);Require(texture,"description texture");RwTextureSetRaster(texture,raster);Require(RwD3D9SetTexture(texture,0),"bind native raster");
    IDirect3DBaseTexture9* base{};Require(SUCCEEDED(Device()->GetTexture(0,&base))&&base,"native resource");
    D3DSURFACE_DESC desc{};const auto result=static_cast<IDirect3DTexture9*>(base)->GetLevelDesc(0,&desc);base->Release();
    RwD3D9SetTexture(nullptr,0);RwTextureSetRaster(texture,nullptr);RwTextureDestroy(texture);Require(SUCCEEDED(result),"native level description");return desc;
}
RwUInt32 Pixel(RwRaster* raster){auto* p=RwRasterLock(raster,0,rwRASTERLOCKREAD);Require(p,"pixel read");auto value=*reinterpret_cast<RwUInt32*>(p);RwRasterUnlock(raster);return value;}
void Fill(RwRaster* raster,RwUInt32 value){auto* p=RwRasterLock(raster,0,rwRASTERLOCKWRITE);Require(p,"pixel write");for(int y=0;y<raster->height;++y)for(int x=0;x<raster->width;++x)*reinterpret_cast<RwUInt32*>(p+y*raster->stride+4*x)=value;RwRasterUnlock(raster);}
void Formats(){
    for(auto format:{D3DFMT_A8R8G8B8,D3DFMT_R5G6B5}){
        auto raster=Make(format,rwRASTERTYPETEXTURE|rwRASTERDONTALLOCATE|rwRASTERFORMAT8888);
        const auto desc=Describe(raster.get());Require(desc.Format==format&&desc.Pool==D3DPOOL_MANAGED&&desc.Usage==0,"native format selects real managed resource");
        Require(!(raster->cFlags&rwRASTERDONTALLOCATE),"native creator allocates despite DONTALLOCATE input");
        const bool alpha=format==D3DFMT_A8R8G8B8;Require(raster->depth==(alpha?32:16)&&(RwRasterGetFormat(raster.get())&rwRASTERFORMATPIXELFORMATMASK)==(alpha?rwRASTERFORMAT8888:rwRASTERFORMAT565),"D3D format replaces RW pixel metadata");
    }
    auto target=Make(D3DFMT_A8R8G8B8,rwRASTERTYPECAMERATEXTURE);const auto desc=Describe(target.get());Require(desc.Pool==D3DPOOL_DEFAULT&&(desc.Usage&D3DUSAGE_RENDERTARGET),"native camera texture uses default render target");
    Fill(target.get(),0xff315579);Require(Pixel(target.get())==0xff315579,"native target transfer roundtrip");
}
void MipsAndFailure(){
    auto mip=Make(D3DFMT_A8R8G8B8,rwRASTERTYPETEXTURE|rwRASTERFORMATMIPMAP);Require(RwRasterGetNumLevels(mip.get())==4,"native software mip chain8x8 has four levels");
    auto normal=Make(D3DFMT_A8R8G8B8,rwRASTERTYPENORMAL|rwRASTERFORMATMIPMAP|rwRASTERFORMATAUTOMIPMAP);
    Require(RwRasterGetNumLevels(normal.get())==1&&!(RwRasterGetFormat(normal.get())&0x9000),"normal native raster strips mip flags");
    for(int i=0;i<3;++i)Require(!RwD3D9RasterCreate(8,8,D3DFMT_UNKNOWN,rwRASTERTYPETEXTURE),"unsupported native format rejected");
    auto valid=Make(D3DFMT_A8R8G8B8);Fill(valid.get(),0xff27496b);Require(Pixel(valid.get())==0xff27496b,"valid creation works after format failures");
}
void Dxt(){
    auto compressed=Make(D3DFMT_DXT1);Require(compressed->depth==16&&(RwRasterGetFormat(compressed.get())&rwRASTERFORMATPIXELFORMATMASK)==rwRASTERFORMAT565,"DXT1 nominal RW metadata is565/16");
    Require(Describe(compressed.get()).Format==D3DFMT_DXT1,"actual resource retains compressed format");
    const unsigned char block[8]={0x00,0xf8,0,0,0,0,0,0}; // RGB565 red endpoint, every index0.
    auto* p=RwRasterLock(compressed.get(),0,rwRASTERLOCKWRITE);Require(p&&compressed->stride>=16,"compressed block lock pitch");
    for(int y=0;y<2;++y)for(int x=0;x<2;++x)std::memcpy(p+y*compressed->stride+x*8,block,8);RwRasterUnlock(compressed.get());
    auto destination=Make(D3DFMT_A8R8G8B8,rwRASTERTYPECAMERATEXTURE);Require(RwRasterPushContext(destination.get()),"DXT render context");
    Require(RwRasterRenderFast(compressed.get(),0,0)==compressed.get(),"DXT public draw");Require(RwRasterPopContext()==destination.get(),"DXT pop");
    Require((Pixel(destination.get())&0xffffff)==0xff0000,"GPU samples compressed red block correctly");
}
void Convert(bool alpha){
    auto raster=Make(alpha?D3DFMT_A8R8G8B8:D3DFMT_X8R8G8B8);auto* identity=raster.get();const RwUInt32 value=alpha?0x80406080:0xff406080;Fill(identity,value);
    auto* texture=RwTextureCreate(nullptr);Require(texture,"conversion texture");RwTextureSetRaster(texture,identity);Require(RwD3D9SetTexture(texture,0),"conversion bind");
    _rwD3D9RasterConvertToNonPalettized(identity);
    Require(texture->raster==identity&&identity->width==8&&identity->height==8,"in-place conversion retains raster wrapper");
    IDirect3DBaseTexture9* bound{};Require(SUCCEEDED(Device()->GetTexture(0,&bound))&&!bound,"conversion destroys prior resource and unbinds stage");
    const auto actual=Pixel(identity);Require((actual&0xffffff)==(value&0xffffff)&&(!alpha||actual==value),"conversion preserves color and optional alpha");
    const auto expected=alpha?rwRASTERFORMAT8888:rwRASTERFORMAT888;Require((RwRasterGetFormat(identity)&rwRASTERFORMATPIXELFORMATMASK)==expected,"conversion chooses host supported alpha/opaque format");
    Require((RwRasterGetFormat(identity)&0x9000)==0x9000,"conversion forces mip and automip flags");
    RwTextureSetRaster(texture,nullptr);RwTextureDestroy(texture);
}
void DxtReverseGpu(D3DFORMAT format){
    auto raster=Make(format);Require(Describe(raster.get()).Format==format,"GPU keeps requested compressed format");
    unsigned char block[16]={};
    if(format==D3DFMT_DXT3)std::memset(block,0xff,8);else {block[0]=255;block[1]=0;}
    // Reverse endpoints blue<=red, selector3. CPU legacy helper emits cyan;
    // native D3D samples the four-color interpolation with opaque alpha.
    block[8]=0x1f;block[9]=0;block[10]=0;block[11]=0xf8;
    std::memset(block+12,0xff,4);
    auto* bytes=RwRasterLock(raster.get(),0,rwRASTERLOCKWRITE);Require(bytes&&raster->stride>=32,"compressed alpha block pitch");
    for(int y=0;y<2;++y)for(int x=0;x<2;++x)std::memcpy(bytes+y*raster->stride+x*16,block,16);
    RwRasterUnlock(raster.get());
    auto target=Make(D3DFMT_A8R8G8B8,rwRASTERTYPECAMERATEXTURE);Fill(target.get(),0);
    Require(RwRasterPushContext(target.get()),"compressed alpha target");
    Require(RwRasterRenderFast(raster.get(),0,0)==raster.get(),"compressed alpha draw");
    Require(RwRasterPopContext()==target.get(),"compressed alpha pop");
    const auto pixel=Pixel(target.get());const auto red=(pixel>>16)&255,green=(pixel>>8)&255,blue=pixel&255;
    Require(red>=168&&red<=172&&green==0&&blue>=83&&blue<=87,"GPU reverse endpoints use four-color interpolation, unlike CPU legacy cyan");
}
}
void RunNativeFormatCase(std::string_view name){
    if(name=="native_format_create")Formats();else if(name=="native_format_mips_failure")MipsAndFailure();else if(name=="native_format_dxt1")Dxt();
    else if(name=="native_format_dxt3_reverse_gpu")DxtReverseGpu(D3DFMT_DXT3);else if(name=="native_format_dxt5_reverse_gpu")DxtReverseGpu(D3DFMT_DXT5);
    else if(name=="native_format_convert_alpha")Convert(true);else if(name=="native_format_convert_opaque")Convert(false);else throw std::runtime_error("unknown native format case");
}
}
