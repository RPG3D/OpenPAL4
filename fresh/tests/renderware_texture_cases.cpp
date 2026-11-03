#include "renderware_texture_cases.h"
#include "pal4/texture_dictionary_native.h"
#include <rwcore.h>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace pal4::test {
namespace {
void Require(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
int created{},destroyed{},plugin_offset{};
std::vector<int> destructor_references;
void* Construct(void* object,RwInt32 offset,RwInt32 size) {
    Require(offset==plugin_offset && size==4,"texture plugin layout");
    *reinterpret_cast<int*>(static_cast<char*>(object)+offset)=0x6789; ++created; return object;
}
void* Destruct(void* object,RwInt32 offset,RwInt32) {
    Require(*reinterpret_cast<int*>(static_cast<char*>(object)+offset)==0x6789,"texture plugin corrupt");
    destructor_references.push_back(static_cast<RwTexture*>(object)->refCount); ++destroyed; return object;
}
RwTexture* Make(const char* name="test") {
    auto* raster=RwRasterCreate(4,4,32,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888);
    Require(raster!=nullptr,"texture raster creation");
    auto* texture=RwTextureCreate(raster);
    if(!texture) RwRasterDestroy(raster);
    Require(texture!=nullptr,"texture creation");
    RwTextureSetName(texture,name); return texture;
}
void Lifetime() {
    auto* texture=Make();
    Require(texture->refCount==1 && !texture->dict && texture->mask[0]==0,"texture defaults");
    Require(RwTextureGetFilterMode(texture)==rwFILTERNEAREST && RwTextureGetAddressingU(texture)==rwTEXTUREADDRESSWRAP &&
            RwTextureGetAddressingV(texture)==rwTEXTUREADDRESSWRAP,"filter/address defaults");
    auto* original=texture->raster;
    Require(RwTextureSetRaster(texture,nullptr)==texture && !texture->raster,"detach raster");
    Require(RwRasterGetWidth(original)==4,"detaching freed raster");
    auto* replacement=RwRasterCreate(8,8,32,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888);
    Require(replacement && RwTextureSetRaster(texture,replacement)==texture,"attach replacement raster");
    Require(RwRasterDestroy(original),"detached raster caller cleanup");
    RwTextureAddRef(texture); Require(texture->refCount==2,"addref");
    Require(RwTextureDestroy(texture) && texture->refCount==1 && destroyed==0,"non-final release destroyed texture");
    Require(RwTextureDestroy(texture) && destroyed==1 && destructor_references.back()==1,"final destructor refcount contract");
}
RwTexture* CountTexture(RwTexture* texture,void* context) { ++*static_cast<int*>(context); return texture; }
RwTexDictionary* CountDictionary(RwTexDictionary* dict,void* context) { ++*static_cast<int*>(context); return dict; }
void Dictionary() {
    auto* a=RwTexDictionaryCreate(); auto* b=RwTexDictionaryCreate(); Require(a&&b,"dictionary create");
    auto* first=Make("MiXeD"); auto* second=Make("Second"); auto* third=Make("Third");
    Require(RwTexDictionaryAddTexture(a,first)==first && first->refCount==1,"add must not addref");
    RwTexDictionaryAddTexture(a,second);
    RwTexDictionaryAddTexture(a,third);
    Require(RwTexDictionaryFindNamedTexture(a,"mixed")==first && first->refCount==1,"ASCII casefold lookup must not addref");
    Require(!RwTexDictionaryFindNamedTexture(a,"mix"),"prefix must not match");
    const std::string long_name(50,'q'); RwTextureSetName(second,long_name.c_str()); RwTextureSetMaskName(second,long_name.c_str());
    Require(std::strlen(second->name)==31 && std::strlen(second->mask)==31,"name/mask truncation");
    RwTextureSetName(second,"b"); Require(second->name[1]==0 && second->name[31]==0,"short name termination/padding");
    RwTexDictionaryAddTexture(b,first); Require(first->dict==b && first->refCount==1,"transfer must preserve refcount");
    int count=0; RwTexDictionaryForAllTextures(a,CountTexture,&count); Require(count==2,"transfer left old membership");
    NativeTextureDictionaryRuntime api;
    int visits=0;
    api.ForEachTexture(reinterpret_cast<std::uint32_t>(a),[&](auto){++visits;return 0u;}); Require(visits==1,"adapter null callback stop");
    try { api.ForEachTexture(reinterpret_cast<std::uint32_t>(a),[](auto)->std::uint32_t{throw std::runtime_error("visitor");});
          throw std::logic_error("missing visitor exception"); }
    catch(const std::runtime_error& e) { Require(std::string(e.what())=="visitor","wrong visitor exception"); }
    api.ForEachTexture(reinterpret_cast<std::uint32_t>(a),[](auto t){RwTextureDestroy(reinterpret_cast<RwTexture*>(t));return t;});
    count=0; RwTexDictionaryForAllTextures(a,CountTexture,&count); Require(count==0,"delete-current iteration");
    Require(RwTexDictionaryRemoveTexture(first)==first && !first->dict && first->refCount==1,"remove must not destroy or release");
    Require(RwTexDictionaryRemoveTexture(first)==first,"remove detached texture"); RwTextureDestroy(first);
    Require(RwTexDictionarySetCurrent(b)==b && RwTexDictionaryGetCurrent()==b,"current dictionary");
    count=0; RwTexDictionaryForAllTexDictionaries(CountDictionary,&count); Require(count>=2,"global dictionary traversal");
    RwTexDictionaryDestroy(b); Require(RwTexDictionaryGetCurrent()==nullptr,"destroy current dictionary must clear it");
    RwTexDictionaryDestroy(a);
}
int read_calls{}; std::string last_mask; bool read_failure{};
RwTexture* Read(const RwChar* name,const RwChar* mask) {
    ++read_calls; last_mask=mask?mask:"<null>"; return read_failure?nullptr:Make(name);
}
RwTexture* Found(const RwChar*) { return nullptr; }
RwRaster* mip_raster{}; RwImage* mip_image{}; bool mip_ok{};
RwRaster* Generate(RwRaster* raster,RwImage* image) { mip_raster=raster;mip_image=image;return mip_ok?raster:nullptr; }
void ReadPath() {
    const auto saved_read=RwTextureGetReadCallBack(); const auto saved_find=RwTextureGetFindCallBack();
    auto* saved_dictionary=RwTexDictionaryGetCurrent(); auto* dict=RwTexDictionaryCreate(); Require(dict!=nullptr,"read dictionary");
    RwTexDictionarySetCurrent(dict); RwTextureSetReadCallBack(Read);
    auto* first=RwTextureRead("Hero","mask-a"); Require(first && read_calls==1 && first->dict==dict && first->refCount==1,"read miss insertion");
    auto* cached=RwTextureRead("hErO","different-mask");
    Require(cached==first && read_calls==1 && first->refCount==2,"cached name read must addref and ignore mask");
    RwTextureDestroy(cached); RwTextureDestroy(first);
    read_failure=true; Require(!RwTextureRead("missing",nullptr),"read failure not propagated"); read_failure=false;
    RwTextureSetFindCallBack(Found); RwTexDictionarySetCurrent(nullptr);
    auto* detached=RwTextureRead("detached",nullptr); Require(detached && !detached->dict && last_mask=="<null>","null current dictionary read");
    RwTextureDestroy(detached); RwTexDictionaryDestroy(dict);
    RwTextureSetReadCallBack(saved_read); RwTextureSetFindCallBack(saved_find); RwTexDictionarySetCurrent(saved_dictionary);
}
void State() {
    const auto mip=RwTextureGetMipmapping(),automatic=RwTextureGetAutoMipmapping();
    Require(RwTextureSetMipmapping(7) && RwTextureGetMipmapping()==7,"mip bool must preserve raw nonzero value");
    Require(RwTextureSetAutoMipmapping(-3) && RwTextureGetAutoMipmapping()==-3,"auto-mip raw value");
    RwTextureSetMipmapping(mip); RwTextureSetAutoMipmapping(automatic);
    const auto saved=RwTextureGetMipmapGenerationCallBack(); RwTextureSetMipmapGenerationCallBack(Generate);
    auto* texture=Make(); auto* image=RwImageCreate(4,4,32); Require(image!=nullptr,"mipmap callback image");
    mip_ok=true; Require(RwTextureRasterGenerateMipmaps(texture->raster,image) && mip_raster==texture->raster && mip_image==image,"mipmap callback args/result");
    mip_ok=false; Require(!RwTextureRasterGenerateMipmaps(texture->raster,image),"mipmap null callback result");
    RwTextureSetMipmapGenerationCallBack(saved); RwImageDestroy(image); RwTextureDestroy(texture);
}
void GameCleanup() {
    NativeTextureDictionaryRuntime api; RwTextureDictionaryState state{};
    (void)RwTextureDictionary::Construct(state); Require(RwTextureDictionary::Initialize(state,api),"game dictionary initialize");
    auto* dict=reinterpret_cast<RwTexDictionary*>(state.dictionary);
    auto* a=Make("a"); auto* b=Make("b"); RwTextureAddRef(a); RwTextureAddRef(a);
    RwTexDictionaryAddTexture(dict,a); RwTexDictionaryAddTexture(dict,b); RwTexDictionarySetCurrent(dict);
    Require(RwTextureDictionary::Cleanup(state,api)==1 && state.dictionary==0 && state.enabled==1,"game cleanup fields");
    Require(destroyed==2 && destructor_references==std::vector<int>({1,1}),"game must clamp references before SDK destroy");
    Require(RwTexDictionaryGetCurrent()==nullptr && RwTextureDictionary::Cleanup(state,api)==0,"game repeated cleanup");
    Require(RwTextureDictionary::Initialize(state,api) && RwTextureDictionary::Cleanup(state,api)==1,"game reinitialize");
}
}
void RegisterTexturePlugin() {
    plugin_offset=RwTextureRegisterPlugin(4,0x123420,Construct,Destruct,nullptr);
    Require(plugin_offset>=static_cast<int>(sizeof(RwTexture)),"texture plugin offset");
}
void RunTextureCase(std::string_view name) {
    if(name=="texture_lifetime") Lifetime(); else if(name=="texture_dictionary") Dictionary();
    else if(name=="texture_read") ReadPath(); else if(name=="texture_state") State();
    else if(name=="texture_game_cleanup") GameCleanup(); else throw std::runtime_error("unknown texture case");
    Require(created>0 && created==destroyed,"texture plugin lifetime imbalance");
}
}
