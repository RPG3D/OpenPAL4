#include "renderware_texture_loader_cases.h"
#include <windows.h>
#include <rwcore.h>
#include <rtpng.h>
#include <rtbmp.h>
#include <rtfsyst.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <cstring>

namespace pal4::test {
namespace {
void Require(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
struct DropImage { void operator()(RwImage* p) const { RwImageDestroy(p); } };
struct DropTexture { void operator()(RwTexture* p) const { RwTextureDestroy(p); } };
struct DropRaster { void operator()(RwRaster* p) const { RwRasterDestroy(p); } };
using Image=std::unique_ptr<RwImage,DropImage>;
using Texture=std::unique_ptr<RwTexture,DropTexture>;
using Raster=std::unique_ptr<RwRaster,DropRaster>;
Image Solid(int size,RwRGBA color) {
    Image image(RwImageCreate(size,size,32)); Require(image && RwImageAllocatePixels(image.get()),"allocate source image");
    for(int y=0;y<size;++y) for(int x=0;x<size;++x)
        reinterpret_cast<RwRGBA*>(RwImageGetPixels(image.get())+y*RwImageGetStride(image.get()))[x]=color;
    return image;
}
class Files {
    bool created_{},opened_{};
    std::filesystem::path directory_;
public:
    Files() {
        directory_=std::filesystem::current_path()/("rw_texture_fixture_"+std::to_string(GetCurrentProcessId())+"_"+std::to_string(GetTickCount64()));
        Require(std::filesystem::create_directory(directory_),"unique fixture directory");created_=true;
        Require(RtFSManagerOpen(1),"filesystem manager open");opened_=true;
        auto prefix=directory_.string()+"\\";char name[]="tex";
        static_assert(sizeof(name)<=RTFS_MAX_NAME_LENGTH);
        auto* fs=RtWinFSystemInit(8,prefix.data(),name); Require(fs && RtFSManagerRegister(fs),"native filesystem register");
        RtFSManagerSetDefaultFileSystem(fs);
        Require(RwImageRegisterImageFormat("png",RtPNGImageRead,RtPNGImageWrite),"PNG registration");
        Require(RwImageRegisterImageFormat("bmp",RtBMPImageRead,RtBMPImageWrite),"BMP registration");
        Require(RwImageSetPath("") && RwImageSetGamma(1.f),"loader image configuration");
    }
    ~Files() {
        if(opened_) RtFSManagerClose();
        if(created_) {std::error_code error;std::filesystem::remove_all(directory_,error);}
    }
    void Write(const char* filename,int size,RwRGBA color) {
        auto image=Solid(size,color);Require(RwImageWrite(image.get(),filename)==image.get(),"real image codec write");
        Require(std::filesystem::file_size(directory_/filename)>32,"codec produced empty fixture");
    }
    void Corrupt() {std::ofstream file(directory_/"invalid.png",std::ios::binary);file<<"not a PNG";}
};
RwRGBA ReadLevel(RwRaster* raster,int level) {
    Require(RwRasterLock(raster,level,rwRASTERLOCKREAD)!=nullptr,"mipmap read lock");
    Image image(RwImageCreate(RwRasterGetWidth(raster),RwRasterGetHeight(raster),32));
    const bool ok=image && RwImageAllocatePixels(image.get()) && RwImageSetFromRaster(image.get(),raster);
    RwRasterUnlock(raster);Require(ok,"mipmap image readback");
    return *reinterpret_cast<RwRGBA*>(RwImageGetPixels(image.get()));
}
void Color(RwRGBA actual,RwRGBA expected,const char* message,bool alpha=true) {
    Require(actual.red==expected.red && actual.green==expected.green && actual.blue==expected.blue &&
        (!alpha || actual.alpha==expected.alpha),message);
}
void Names() {
    for(unsigned level : {0u,1u,9u,10u,15u,16u,255u,256u}) {
        char name[64]="tile",mask[64]="mask";
        Require(RwTextureGenerateMipmapName(name,mask,level,rwRASTERFORMATMIPMAP),"default mip name result");
        const char* suffix=level==1?"m1":level==9?"m9":level==10?"ma":level==15?"mf":"";
        Require(std::string(name)==std::string("tile")+suffix && std::string(mask)==std::string("mask")+suffix,"default mip suffix");
    }
    char name[64]="tile",empty[64]="";
    Require(RwTextureGenerateMipmapName(name,empty,1,0) && !*empty,"empty mask must stay empty");
}
void Plain() {
    Files files; files.Write("color.png",4,{64,128,192,255});files.Write("mask.bmp",4,{32,96,16,255});files.Corrupt();
    RwTextureSetMipmapping(FALSE);RwTextureSetAutoMipmapping(FALSE);
    Texture texture(RwTextureRead("color","mask"));Require(texture!=nullptr,"default extension discovery/masked load");
    Require(std::string(texture->name)=="color" && std::string(texture->mask)=="mask","texture retains input names");
    Color(ReadLevel(texture->raster,0),{64,128,192,96},"PNG + BMP alpha mask pipeline");
    texture.reset();
    Require(RwImageSetGamma(2.f),"gamma two");
    texture.reset(RwTextureRead("color.png",nullptr));Require(texture!=nullptr,"explicit extension default read");
    Color(ReadLevel(texture->raster,0),{128,181,221,255},"default read gamma correction");
    texture.reset();RwImageSetGamma(1.f);
    Require(!RwTextureRead("missing",nullptr),"missing texture should fail");
    Require(!RwTextureRead("color","missingmask"),"missing mask should fail");
    Require(!RwTextureRead("invalid.png",nullptr),"corrupt PNG should fail");
}
void ManualMips() {
    Files files;files.Write("tile.png",4,{10,20,30,255});files.Write("tilem1.png",2,{40,50,60,255});files.Write("tilem2.png",1,{70,80,90,255});
    RwTextureSetMipmapping(TRUE);RwTextureSetAutoMipmapping(FALSE);
    Texture texture(RwTextureRead("tile",nullptr));Require(texture!=nullptr,"manual mip chain read");
    Require(RwRasterGetNumLevels(texture->raster)==3,"manual mip level count");
    Color(ReadLevel(texture->raster,0),{10,20,30,255},"base mip");
    Color(ReadLevel(texture->raster,1),{40,50,60,255},"external mip one");
    Color(ReadLevel(texture->raster,2),{70,80,90,255},"external mip two");
    texture.reset();files.Write("incomplete.png",4,{10,20,30,255});
    Require(!RwTextureRead("incomplete",nullptr),"missing external mip must fail");
}
void GeneratedMips() {
    Files files;auto source=Solid(4,{64,128,192,200});
    Raster raster(RwRasterCreate(4,4,32,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888|rwRASTERFORMATMIPMAP));
    Require(raster && RwRasterGetNumLevels(raster.get())==3,"generated mip raster");
    Require(RwTextureRasterGenerateMipmaps(raster.get(),source.get()),"default mip generation");
    for(int i=0;i<3;++i) Color(ReadLevel(raster.get(),i),{64,128,192,200},"generated constant mip");
    Require(RwImageGetWidth(source.get())==4,"generator freed borrowed source");
    Require(RwTextureRasterGenerateMipmaps(raster.get(),nullptr),"generate from raster readback");
    for(int i=0;i<3;++i) Color(ReadLevel(raster.get(),i),{64,128,192,200},"readback generated mip");
    Require(RwImageSetGamma(2.f),"generated mip gamma");
    Require(RwTextureRasterGenerateMipmaps(raster.get(),source.get()),"gamma mip generation");
    Color(*reinterpret_cast<RwRGBA*>(RwImageGetPixels(source.get())),{128,181,221,200},"generator gamma mutates borrowed source RGB");
    for(int i=0;i<3;++i) Color(ReadLevel(raster.get(),i),{128,181,221,200},"gamma-correct generated mip");
    Image indexed(RwImageCreate(4,4,8));Require(indexed && RwImageAllocatePixels(indexed.get()),"indexed mip source");
    for(int i=0;i<256;++i) RwImageGetPalette(indexed.get())[i]={64,128,192,200};
    std::memset(RwImageGetPixels(indexed.get()),1,RwImageGetStride(indexed.get())*4);
    Require(RwTextureRasterGenerateMipmaps(raster.get(),indexed.get()),"indexed source promotion for mip generation");
    Color(RwImageGetPalette(indexed.get())[1],{64,128,192,200},"indexed borrowed source should be preserved");
    for(int i=0;i<3;++i) Color(ReadLevel(raster.get(),i),{128,181,221,200},"indexed-to-RGBA generated mip");
    files.Write("auto.png",4,{24,48,72,255});
    RwTextureSetMipmapping(TRUE);RwTextureSetAutoMipmapping(TRUE);
    Texture texture(RwTextureRead("auto",nullptr));Require(texture!=nullptr,"automatic mip texture read");
    Require(RwRasterGetNumLevels(texture->raster)>=1,"automatic mip levels");
    Require((RwRasterGetFormat(texture->raster)&rwRASTERFORMATAUTOMIPMAP)!=0,"driver did not accept automatic mip format");
    Color(ReadLevel(texture->raster,0),{24,48,72,255},"automatic mip base image");
}
void Find() {
    auto* saved=RwTexDictionaryGetCurrent();auto* a=RwTexDictionaryCreate();auto* b=RwTexDictionaryCreate();Require(a&&b,"find dictionaries");
    Texture texture(RwTextureCreate(nullptr));Require(texture!=nullptr,"find texture");RwTextureSetName(texture.get(),"Across");RwTexDictionaryAddTexture(a,texture.get());
    const auto find=RwTextureGetFindCallBack();RwTexDictionarySetCurrent(b);
    Require(!find("across"),"current dictionary must limit search");
    RwTexDictionarySetCurrent(nullptr);Require(find("ACROSS")==texture.get() && texture->refCount==1,"null current must search all dictionaries without addref");
    RwTexDictionaryRemoveTexture(texture.get());texture.reset();RwTexDictionaryDestroy(a);RwTexDictionaryDestroy(b);RwTexDictionarySetCurrent(saved);
}
}
void RunTextureLoaderCase(std::string_view name) {
    if(name=="loader_names") Names();else if(name=="loader_plain") Plain();
    else if(name=="loader_manual_mips") ManualMips();else if(name=="loader_generated_mips") GeneratedMips();
    else if(name=="loader_find") Find();else throw std::runtime_error("unknown loader case");
}
}
