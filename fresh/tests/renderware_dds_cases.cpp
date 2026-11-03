#include "renderware_dds_cases.h"
#include "pal4/rw_texture_read_hook_native.h"
#include "pal4/native_compressed_raster_alpha.h"
#include "support/renderware_format_probe.h"
#include <windows.h>
#include <rwcore.h>
#include <rtfsyst.h>
#include <rtpng.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

static_assert(std::is_same_v<decltype(&RwD3D9RasterStreamReadDDS),RwRaster* (__cdecl *)(RwStream*)>);
static_assert(std::is_same_v<decltype(&RwD3D9DDSTextureRead),RwTexture* (__cdecl *)(const char*,const char*)>);
static_assert(std::is_same_v<decltype(&RwD3D9RasterIsCompressed),RwBool (__cdecl *)(const RwRaster*)>);
namespace pal4::test {
namespace {
void Require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
std::set<RwRaster*> live;
std::set<RwRaster*> baseline;
void* RasterBorn(void* p,RwInt32,RwInt32){live.insert(static_cast<RwRaster*>(p));return p;}
void* RasterDied(void* p,RwInt32,RwInt32){live.erase(static_cast<RwRaster*>(p));return p;}
struct DropRaster {void operator()(RwRaster* p)const{if(p)RwRasterDestroy(p);}};
struct DropTexture {void operator()(RwTexture* p)const{if(p)RwTextureDestroy(p);}};
struct DropImage {void operator()(RwImage* p)const{if(p)RwImageDestroy(p);}};
using Raster=std::unique_ptr<RwRaster,DropRaster>;
using Texture=std::unique_ptr<RwTexture,DropTexture>;
using Image=std::unique_ptr<RwImage,DropImage>;
using Bytes=std::vector<unsigned char>;
void Word(Bytes& data,size_t offset,RwUInt32 value){std::memcpy(data.data()+offset,&value,4);}
Bytes Dds(D3DFORMAT format,int levels=1,bool reverse=false){
    Bytes data(128);Word(data,0,0x20534444);Word(data,4,124);Word(data,8,0x81007|(levels>1?0x20000:0));
    Word(data,12,8);Word(data,16,8);Word(data,20,format==D3DFMT_DXT1?32:64);Word(data,28,levels);
    Word(data,76,32);Word(data,80,4);Word(data,84,format);Word(data,108,0x1000|(levels>1?0x400008:0));
    const int blockSize=format==D3DFMT_DXT1?8:16;
    for(int level=0;level<levels;++level){
        const int dimension=std::max(1,8>>level),blocks=std::max(1,dimension/4);
        for(int n=0;n<blocks*blocks;++n){
            std::array<unsigned char,16> block{};int color=0;
            if(format!=D3DFMT_DXT1){color=8;if(format==D3DFMT_DXT3||format==D3DFMT_DXT2)std::memset(block.data(),255,8);else block[0]=255;}
            if(reverse){block[color]=0x1f;block[color+2]=0;block[color+3]=0xf8;std::memset(block.data()+color+4,0xff,4);}
            else {block[color]=0;block[color+1]=0xf8;}
            data.insert(data.end(),block.begin(),block.begin()+blockSize);
        }
    }
    return data;
}
struct Loaded {Raster raster;RwUInt32 consumed;};
Loaded Load(Bytes& data){
    const RwMemory memory{data.data(),static_cast<RwUInt32>(data.size())};
    auto* stream=RwStreamOpen(rwSTREAMMEMORY,rwSTREAMREAD,&memory);Require(stream,"DDS memory stream");
    Raster raster(RwD3D9RasterStreamReadDDS(stream));const auto consumed=stream->Type.memory.position;
    Require(RwStreamClose(stream,nullptr),"DDS reader leaves stream for caller");return {std::move(raster),consumed};
}
RwRGBA Pixel(RwRaster* raster){
    Image image(RwImageCreate(raster->width,raster->height,32));
    Require(image&&RwImageAllocatePixels(image.get())&&RwImageSetFromRaster(image.get(),raster),"uncompressed raster image readback");
    return *reinterpret_cast<RwRGBA*>(image->cpPixels);
}
void Native(){
    for(auto format:{D3DFMT_DXT1,D3DFMT_DXT3,D3DFMT_DXT5}){
        auto data=Dds(format);auto loaded=Load(data);auto* r=loaded.raster.get();
        Require(r&&RwD3D9RasterIsCompressed(r)&&r->width==8&&r->height==8,"native DDS compressed wrapper");
        Require(loaded.consumed==data.size()&&RwRasterGetNumLevels(r)==1,"native DDS consumes one complete level");
        auto* p=RwRasterLock(r,0,rwRASTERLOCKREAD);Require(p,"native DDS payload lock");const int row=format==D3DFMT_DXT1?16:32;
        for(int y=0;y<2;++y)Require(!std::memcmp(p+y*r->stride,data.data()+128+y*row,row),"native DDS block payload retained");
        RwRasterUnlock(r);
        Require((RwRasterGetFormat(r)&rwRASTERFORMATPIXELFORMATMASK)==(format==D3DFMT_DXT1?rwRASTERFORMAT565:rwRASTERFORMAT4444),"compressed DDS RW alpha metadata");
    }
    for(auto format:{D3DFMT_DXT1,D3DFMT_DXT3,D3DFMT_DXT5}){
        auto opaque=Dds(format);
        auto inspect=[](Bytes& bytes){
            auto loaded=Load(bytes);
            Require(loaded.raster != nullptr,"compressed alpha fixture loads");
            Texture texture(RwTextureCreate(loaded.raster.release()));
            Require(texture != nullptr,"compressed alpha fixture texture");
            return NativeCompressedRasterHasAlpha(texture.get());
        };
        Require(inspect(opaque)==false,"opaque compressed fixture has no Alpha");
        auto transparent=opaque;
        if(format==D3DFMT_DXT1){
            transparent[128]=0;transparent[129]=0;
            transparent[130]=0;transparent[131]=0xf8;
            transparent[132]=3;
        }else if(format==D3DFMT_DXT3){
            transparent[128]=0xf0;
        }else{
            transparent[130]=1;
        }
        Require(inspect(transparent)==true,"transparent compressed fixture has Alpha");
    }
}
void Mips(){
    auto data=Dds(D3DFMT_DXT1,4); // Put actual transparency in the 1x1 mip.
    data[data.size()-8]=0;data[data.size()-7]=0;std::fill(data.end()-4,data.end(),255);
    auto loaded=Load(data);auto* r=loaded.raster.get();
    Require(r&&loaded.consumed==data.size()&&RwRasterGetNumLevels(r)==4,"DDS native full mip chain");
    Require((RwRasterGetFormat(r)&rwRASTERFORMATPIXELFORMATMASK)==rwRASTERFORMAT1555,"DXT1 alpha scan includes smallest mip");
    for(int i=0;i<4;++i){Require(RwRasterLock(r,i,rwRASTERLOCKREAD),"native DDS mip lock");Require(r->width==std::max(1,8>>i),"mip dimensions");RwRasterUnlock(r);}
}
void Rgb(){
    auto data=Dds(D3DFMT_DXT1);data.resize(128+8*8*4);
    Word(data,8,0x100f);Word(data,20,32);Word(data,80,65);Word(data,84,0);Word(data,88,32);
    Word(data,92,0xff0000);Word(data,96,0xff00);Word(data,100,0xff);Word(data,104,0xff000000);
    for(size_t at=128;at<data.size();at+=4)Word(data,at,0x80406080);
    auto loaded=Load(data);Require(loaded.raster&&!RwD3D9RasterIsCompressed(loaded.raster.get())&&loaded.consumed==data.size(),"uncompressed DDS RGBA load");
    const auto c=Pixel(loaded.raster.get());Require(c.red==64&&c.green==96&&c.blue==128&&c.alpha==128,"DDS native BGRA channel layout");
}
void Cube(){
    auto face=Dds(D3DFMT_DXT5);auto data=face;data.resize(128);Word(data,108,0x1008);Word(data,112,0xfe00);
    for(int i=0;i<6;++i){face[136]=static_cast<unsigned char>(i+1);data.insert(data.end(),face.begin()+128,face.end());}
    {
        auto loaded=Load(data);auto* r=loaded.raster.get();Require(r&&RwD3D9RasterIsCompressed(r)&&loaded.consumed==data.size(),"DDS six-face compressed cube load");
        Texture texture(RwTextureCreate(loaded.raster.release()));Require(texture&&RwD3D9SetTexture(texture.get(),0),"bind DDS cube resource");
        auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());IDirect3DBaseTexture9* base{};
        Require(SUCCEEDED(device->GetTexture(0,&base))&&base&&base->GetType()==D3DRTYPE_CUBETEXTURE,"DDS produced actual D3D cube");
        auto* cube=static_cast<IDirect3DCubeTexture9*>(base);
        for(int i=0;i<6;++i){D3DLOCKED_RECT lock{};Require(SUCCEEDED(cube->LockRect(static_cast<D3DCUBEMAP_FACES>(i),0,&lock,nullptr,D3DLOCK_READONLY)),"DDS cube face read");
            Require(static_cast<unsigned char*>(lock.pBits)[8]==i+1,"DDS face stream order maps +X,-X,+Y,-Y,+Z,-Z");cube->UnlockRect(static_cast<D3DCUBEMAP_FACES>(i),0);}
        base->Release();RwD3D9SetTexture(nullptr,0);
    }
    DxtUnsupported proxy;Require(!Load(data).raster&&proxy.denied>0,"unsupported cube DDS rejects without software fallback");
}
void Fallback(){
    DxtUnsupported proxy;
    for(auto format:{D3DFMT_DXT1,D3DFMT_DXT3,D3DFMT_DXT5}){
        for(bool reverse:{false,true}){
            auto data=Dds(format,1,reverse);auto loaded=Load(data);auto* r=loaded.raster.get();
            Require(r&&!RwD3D9RasterIsCompressed(r)&&loaded.consumed==data.size(),"denied DXT selects public DDS software fallback");
            const auto c=Pixel(r);
            std::printf("DDS fallback format=%u reverse=%d RWformat=%x pixel=%u,%u,%u,%u\n",unsigned(format),int(reverse),RwRasterGetFormat(r),c.red,c.green,c.blue,c.alpha);
            const bool rgba4444=(RwRasterGetFormat(r)&rwRASTERFORMATPIXELFORMATMASK)==rwRASTERFORMAT4444;
            const unsigned maxChannel=rgba4444?240:248;
            if(!reverse)Require(c.red==maxChannel&&c.green==0&&c.blue==0&&c.alpha==255,"software fallback endpoint reflects selected 565/1555/4444 quantization");
            else if(format==D3DFMT_DXT1)Require(c.red==0&&c.green==0&&c.blue==0&&c.alpha==0,"software fallback DXT1 transparent pixels");
            else Require(c.red==0&&c.green==maxChannel&&c.blue==maxChannel&&c.alpha==255,"public DDS fallback retains legacy reverse-endpoint cyan after raster quantization");
        }
    }
    auto mip=Dds(D3DFMT_DXT1,4);auto loaded=Load(mip);
    Require(loaded.raster&&loaded.consumed==160&&loaded.consumed<mip.size(),"software fallback reads base level only");
    Require((RwRasterGetFormat(loaded.raster.get())&0x9000)==0x9000,"fallback generates mipmaps from decoded base");
    Require(proxy.denied>=7,"fallback probes reached intercepted real SDK capability boundary");
    std::printf("DDS software fallback executed: %u rejected DXT capability probes\n",proxy.denied);
}
void Headers(){
    for(int size:{0,3,4,127}){Bytes data(size);if(size>=4)Word(data,0,0x20534444);Require(!Load(data).raster,"truncated DDS header rejected");}
    auto data=Dds(D3DFMT_DXT1);Word(data,0,0);Require(!Load(data).raster,"bad DDS magic rejected");
    data=Dds(D3DFMT_DXT1);Word(data,8,0);Require(!Load(data).raster,"zero DDS flags rejected");
    data=Dds(D3DFMT_DXT1);Word(data,4,0);Word(data,8,1);
    Require(Load(data).raster!=nullptr,"legacy DDS header accepts any required flag and ignores header-size field");
    Require(live==baseline,"header cases leave no raster");
}
void Truncated(){
    Require(live==baseline,"clean failure baseline");auto data=Dds(D3DFMT_DXT1);data.pop_back();
    auto loaded=Load(data);Require(!loaded.raster&&live.size()==baseline.size()+1,"short native payload returns null and retains raster");
    auto* leaked=*std::find_if(live.begin(),live.end(),[](RwRaster* p){return !baseline.contains(p);});Require(RwRasterUnlock(leaked),"diagnostic unlock of leaked payload raster");
    Require(RwRasterDestroy(leaked)&&live==baseline,"explicit diagnostic cleanup of leaked raster");
    std::puts("DDS failure diagnostic: SDK returned null with one locked raster; fixture explicitly released it");
}
class Files {
    std::filesystem::path directory_;
public:
    Files(){
        directory_=std::filesystem::current_path()/("rw_dds_fixture_"+std::to_string(GetCurrentProcessId())+"_"+std::to_string(GetTickCount64()));
        Require(std::filesystem::create_directory(directory_),"DDS fixture directory");
        Require(RtFSManagerOpen(1),"DDS filesystem open");auto prefix=directory_.string()+"\\";char name[]="dds";
        auto* fs=RtWinFSystemInit(8,prefix.data(),name);Require(fs&&RtFSManagerRegister(fs),"DDS filesystem registration");RtFSManagerSetDefaultFileSystem(fs);
        Require(RwImageSetPath("")&&RwImageSetGamma(1.f),"DDS path setup");
    }
    ~Files(){RwImageSetPath("");RtFSManagerClose();std::error_code error;std::filesystem::remove_all(directory_,error);}
    void Write(const char* name,const Bytes& data){const auto path=directory_/name;std::filesystem::create_directories(path.parent_path());std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(data.data()),data.size());Require(bool(file),"DDS fixture write");}
    void Png(const char* name){
        Require(RwImageRegisterImageFormat("png",RtPNGImageRead,RtPNGImageWrite),"DDS fallback PNG registration");
        Image image(RwImageCreate(8,8,32));Require(image&&RwImageAllocatePixels(image.get()),"PNG source");
        for(int y=0;y<8;++y)for(int x=0;x<8;++x)reinterpret_cast<RwRGBA*>(image->cpPixels+y*image->stride)[x]={40,80,120,255};
        Require(RwImageWrite(image.get(),name)==image.get(),"native PNG fixture write");
    }
};
void Paths(){
    Files files;files.Write("tile.dds",Dds(D3DFMT_DXT1));
    Texture direct(RwD3D9DDSTextureRead("tile","missing-mask"));Require(direct&&std::string(direct->name)=="tile"&&direct->mask[0]==0&&RwTextureGetFilterMode(direct.get())==rwFILTERLINEAR,"DDS ignores mask and sets linear/name");
    files.Write("second\\mipped.dds",Dds(D3DFMT_DXT1,4));Require(RwImageSetPath("first\\;second\\"),"DDS search list");
    Texture found(RwD3D9DDSTextureRead("mipped",nullptr));Require(found&&std::string(found->name)=="mipped"&&RwTextureGetFilterMode(found.get())==rwFILTERLINEARMIPLINEAR,"semicolon path finds later DDS with mip filter");
    files.Write("first\\mipped.dds",Bytes{0,0,0,0});
    Require(!RwD3D9DDSTextureRead("mipped",nullptr),"existing corrupt first match stops search");
}
void Game(){
    Files files;files.Write("stone.dds",Dds(D3DFMT_DXT1));files.Png("fallback.png");files.Png("masked.png");files.Png("mask.png");
    RwTextureSetMipmapping(FALSE);RwTextureSetAutoMipmapping(FALSE);
    auto previous=RwTextureGetReadCallBack();RwTextureReadHookState state;
    {
        NativeRwTextureReadHookRuntime runtime(state);Require(RwTextureReadHook::Install(state,runtime),"native game callback install");
        Require(state.previous_callback==reinterpret_cast<std::uint32_t>(previous),"game retains actual SDK callback");
        bool rejected=false;try{(void)RwTextureReadHook::Install(state,runtime);}catch(const std::logic_error&){rejected=true;}
        Require(rejected&&state.previous_callback==reinterpret_cast<std::uint32_t>(previous),"adapter rejects invalid repeated installation without damaging previous callback");
        Texture dds(RwTextureRead("stone.png","absent-mask"));Require(dds&&RwD3D9RasterIsCompressed(dds->raster)&&std::string(dds->name)=="stone","game extension stripping reaches real DDS and ignores mask there");
        Texture png(RwTextureRead("fallback.png",nullptr));Require(png&&!RwD3D9RasterIsCompressed(png->raster),"game DDS miss invokes actual PNG reader");
        const auto c=Pixel(png->raster);Require(c.red==40&&c.green==80&&c.blue==120,"game fallback pixels");
        Texture masked(RwTextureRead("masked.png","mask.png"));Require(masked&&Pixel(masked->raster).alpha==120,"game preserves mask argument for saved SDK callback");
        Require(!RwTextureRead("missing.png",nullptr),"game both formats missing returns null");
    }
    Require(RwTextureGetReadCallBack()==previous,"native adapter restores borrowed callback on release");
}
}
void RegisterDdsPlugin(){Require(RwRasterRegisterPlugin(0,0x1234d5,RasterBorn,RasterDied,nullptr)>=0,"DDS raster tracking plugin");}
void RunDdsCase(std::string_view name){
    baseline=live; // Engine-owned default rasters exist before the case starts.
    if(name=="dds_native")Native();else if(name=="dds_mips")Mips();else if(name=="dds_rgb")Rgb();else if(name=="dds_cube")Cube();else if(name=="dds_fallback")Fallback();else if(name=="dds_headers")Headers();else if(name=="dds_truncated")Truncated();else if(name=="dds_paths")Paths();else if(name=="dds_game")Game();else throw std::runtime_error("unknown DDS case");
    Require(live==baseline,"DDS cases release all observed rasters");
}
}
