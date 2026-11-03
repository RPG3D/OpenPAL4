#include "renderware_texture_stream_cases.h"
#include <rwcore.h>
#include <array>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace pal4::test {
namespace {
void Require(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
int offset{},plugin_reads{},loader_reads{};bool reject_plugin{};RwTexture* last_loaded{};RwTexture* cached{};RwBool observed_mip{},observed_auto{};
RwPluginRegistry stream_registry{20,20,0,0,nullptr,nullptr};
constexpr RwUInt32 stream_plugin_id=0x123478;
std::array<int,5> registry_calls{};
RwInt32 RegistrySize(const void*,RwInt32 at,RwInt32 bytes){
    Require(at==20&&bytes==4,"plugin size callback ABI");++registry_calls[0];return 4;
}
RwStream* RegistryWrite(RwStream* stream,RwInt32 length,const void* object,RwInt32 at,RwInt32 bytes){
    Require(length==4&&at==20&&bytes==4,"plugin write callback ABI");++registry_calls[1];
    return RwStreamWrite(stream,static_cast<const char*>(object)+at,4);
}
RwStream* RegistryRead(RwStream* stream,RwInt32 length,void* object,RwInt32 at,RwInt32 bytes){
    Require(length==4&&at==20&&bytes==4,"plugin read callback ABI");++registry_calls[2];
    return RwStreamRead(stream,static_cast<char*>(object)+at,4)==4?stream:nullptr;
}
RwBool RegistryAlways(void* object,RwInt32 at,RwInt32 bytes){
    Require(object&&at==20&&bytes==4,"plugin always callback ABI");++registry_calls[3];return TRUE;
}
RwBool RegistryRights(void* object,RwInt32 at,RwInt32 bytes,RwUInt32 extra){
    Require(object&&at==20&&bytes==4&&extra==0x55aa,"plugin rights callback ABI");
    ++registry_calls[4];return TRUE;
}
RwUInt32& Value(void* object){return *reinterpret_cast<RwUInt32*>(static_cast<char*>(object)+offset);}
void* Construct(void* object,RwInt32,RwInt32){Value(object)=0;return object;}
RwInt32 Size(const void*,RwInt32,RwInt32){return 4;}
RwStream* Write(RwStream* stream,RwInt32,const void* object,RwInt32,RwInt32){return RwStreamWrite(stream,&Value(const_cast<void*>(object)),4);}
RwStream* Read(RwStream* stream,RwInt32 size,void* object,RwInt32,RwInt32){++plugin_reads;if(reject_plugin)return nullptr;return size==4&&RwStreamRead(stream,&Value(object),4)==4?stream:nullptr;}
struct TextureDrop{void operator()(RwTexture* p)const{if(p)RwTextureDestroy(p);}};using Texture=std::unique_ptr<RwTexture,TextureDrop>;
Texture Make(bool auto_mip=false){
    auto* raster=RwRasterCreate(4,4,32,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888|rwRASTERFORMATMIPMAP|(auto_mip?rwRASTERFORMATAUTOMIPMAP:0));Require(raster,"stream raster");Texture texture(RwTextureCreate(raster));Require(texture!=nullptr,"stream texture");RwTextureSetName(texture.get(),"stone");RwTextureSetMaskName(texture.get(),"mask");(void)RwTextureSetFilterMode(texture.get(),rwFILTERLINEARMIPLINEAR);(void)RwTextureSetAddressingU(texture.get(),rwTEXTUREADDRESSCLAMP);(void)RwTextureSetAddressingV(texture.get(),rwTEXTUREADDRESSMIRROR);Value(texture.get())=0x1234abcd;return texture;
}
struct Bytes {
    RwMemory memory{};
    explicit Bytes(RwTexture* texture){auto* s=RwStreamOpen(rwSTREAMMEMORY,rwSTREAMWRITE,nullptr);Require(s&&RwTextureStreamWrite(texture,s)==texture,"write texture reference");Require(RwStreamClose(s,&memory),"close texture writer");}
    ~Bytes(){if(memory.start)RwFree(memory.start);}
    RwUInt32 Word(int at)const{RwUInt32 v{};Require(at+4<=int(memory.length),"stream field bounds");std::memcpy(&v,memory.start+at,4);return v;}
    void Word(int at,RwUInt32 value){Require(at+4<=int(memory.length),"stream field bounds");std::memcpy(memory.start+at,&value,4);}
};
struct StreamDrop{void operator()(RwStream* s)const{if(s)RwStreamClose(s,nullptr);}};using Stream=std::unique_ptr<RwStream,StreamDrop>;
Stream Open(Bytes& bytes){Stream s(RwStreamOpen(rwSTREAMMEMORY,rwSTREAMREAD,&bytes.memory));Require(s&&RwStreamFindChunk(s.get(),rwID_TEXTURE,nullptr,nullptr),"find texture chunk");return s;}
RwTexture* Find(const RwChar*){return cached;}
RwTexture* Load(const RwChar* name,const RwChar* mask){++loader_reads;Require(std::string(name)=="stone"&&std::string(mask)=="mask","stream passes names to texture loader");observed_mip=RwTextureGetMipmapping();observed_auto=RwTextureGetAutoMipmapping();last_loaded=RwTextureCreate(nullptr);Require(last_loaded,"loader texture");RwTextureSetName(last_loaded,name);RwTextureSetMaskName(last_loaded,mask);return last_loaded;}
struct Callbacks {
    RwTextureCallBackRead read=RwTextureGetReadCallBack();RwTextureCallBackFind find=RwTextureGetFindCallBack();RwTexDictionary* dict=RwTexDictionaryGetCurrent();RwBool mip=RwTextureGetMipmapping(),automatic=RwTextureGetAutoMipmapping();
    Callbacks(){RwTextureSetReadCallBack(Load);RwTextureSetFindCallBack(Find);RwTexDictionarySetCurrent(nullptr);RwTextureSetMipmapping(7);RwTextureSetAutoMipmapping(-3);}
    ~Callbacks(){RwTextureSetReadCallBack(read);RwTextureSetFindCallBack(find);RwTexDictionarySetCurrent(dict);RwTextureSetMipmapping(mip);RwTextureSetAutoMipmapping(automatic);cached=nullptr;}
    void Restored(){Require(RwTextureGetMipmapping()==7&&RwTextureGetAutoMipmapping()==-3,"reader restores global mip settings");}
};
void Layout(){
    auto texture=Make();Bytes bytes(texture.get());Require(bytes.Word(0)==rwID_TEXTURE&&bytes.Word(4)==RwTextureStreamGetSize(texture.get())&&bytes.memory.length==bytes.Word(4)+12,"texture size excludes outer header");
    Require(bytes.Word(12)==rwID_STRUCT&&bytes.Word(16)==4,"texture structure is four bytes");Require((bytes.Word(8)&0xffff)==0xa&&(bytes.Word(20)&0xffff)==0xa,"SDK writes build0xA at both headers; original0x65 mismatch remains");
    Require(bytes.Word(24)==((texture->filterAddressing&0xffff)|0x10000),"packed filter/address and explicit mip marker");Require(bytes.Word(28)==rwID_STRING&&bytes.Word(32)==8&&!std::strcmp(reinterpret_cast<char*>(bytes.memory.start+40),"stone"),"aligned texture name chunk");
    const std::string long_string(300,'x');Require(_rwStringStreamGetSize(long_string.c_str())==304,"string size helper returns full32bit, not char");
}
void Fresh(){
    Callbacks callbacks;
    for(bool automatic:{false,true}){auto source=Make(automatic);Bytes bytes(source.get());auto s=Open(bytes);Texture result(RwTextureStreamRead(s.get()));Require(result&&result.get()==last_loaded&&Value(result.get())==0x1234abcd,"fresh texture reads plugin payload");Require(result->filterAddressing==(source->filterAddressing&0xffff),"fresh texture accepts serialized filter/address");Require(observed_mip==1&&observed_auto==(automatic?1:0),"loader sees stream mip policy");callbacks.Restored();}
}
void Cached(){
    Callbacks callbacks;auto source=Make();Bytes bytes(source.get());auto shared=Make();(void)RwTextureSetFilterMode(shared.get(),rwFILTERNEAREST);Value(shared.get())=0x99887766;cached=shared.get();auto s=Open(bytes);Texture result(RwTextureStreamRead(s.get()));Require(result.get()==shared.get()&&result->refCount==2,"cached read adds reference");Require(plugin_reads==0&&loader_reads==0&&Value(result.get())==0x99887766&&RwTextureGetFilterMode(result.get())==rwFILTERNEAREST,"shared texture skips plugin/filter mutation");callbacks.Restored();cached=nullptr;
}
void Failure(){
    Callbacks callbacks;auto source=Make();Bytes bytes(source.get());auto s=Open(bytes);reject_plugin=true;Require(!RwTextureStreamRead(s.get()),"reject plugin fails stream read");callbacks.Restored();
    Require(last_loaded&&last_loaded->refCount==1,"failed extension leaves loaded texture reference alive");Texture leaked(last_loaded);reject_plugin=false; // Explicit diagnostic cleanup, not SDK behavior.
}
void LegacyAndTruncated(){
    Callbacks callbacks;auto source=Make();Bytes bytes(source.get());bytes.Word(24,bytes.Word(24)&~0xf000u);auto s=Open(bytes);Texture result(RwTextureStreamRead(s.get()));Require(result&&RwTextureGetAddressingV(result.get())==RwTextureGetAddressingU(result.get()),"missing V addressing inherits U");callbacks.Restored();
    bytes.memory.length=48;auto short_stream=Open(bytes);const auto before=loader_reads;Require(!RwTextureStreamRead(short_stream.get())&&loader_reads==before,"missing mask fails before texture loader");callbacks.Restored();
}
void PluginRegistryStream(){
    struct Object {std::array<RwUInt32,5> base{};RwUInt32 value{};};
    static_assert(offsetof(Object,value)==20);
    Object source{},target{};
    source.value=0x89abcdef;
    registry_calls.fill(0);
    Require(_rwPluginRegistryGetSize(&stream_registry,&source)==16,"plugin size includes chunk header");
    RwMemory memory{};
    auto* writer=RwStreamOpen(rwSTREAMMEMORY,rwSTREAMWRITE,nullptr);
    Require(writer&&_rwPluginRegistryWriteDataChunks(&stream_registry,writer,&source)==&stream_registry,
            "plugin extension write");
    Require(RwStreamClose(writer,&memory)&&memory.length==28,"extension and plugin chunk lengths");
    auto word=[&](int offset){RwUInt32 value{};std::memcpy(&value,memory.start+offset,4);return value;};
    Require(word(0)==rwID_EXTENSION&&word(4)==16&&word(12)==stream_plugin_id&&word(16)==4&&word(24)==source.value,
            "extension and plugin payload layout");
    Require((word(8)&0xffff)==10&&(word(20)&0xffff)==10,
            "SDK writes build 10; original registry writer uses build 101");
    auto* reader=RwStreamOpen(rwSTREAMMEMORY,rwSTREAMREAD,&memory);
    Require(reader&&_rwPluginRegistryReadDataChunks(&stream_registry,reader,&target)==&stream_registry&&
                target.value==source.value,"plugin extension read");
    Require(RwStreamClose(reader,nullptr),"close plugin reader");
    reader=RwStreamOpen(rwSTREAMMEMORY,rwSTREAMREAD,&memory);
    Require(reader&&_rwPluginRegistrySkipDataChunks(&stream_registry,reader)==&stream_registry&&
                reader->Type.memory.position==memory.length,"plugin extension skip consumes stream");
    Require(RwStreamClose(reader,nullptr),"close plugin skip reader");
    Require(_rwPluginRegistryInvokeRights(&stream_registry,stream_plugin_id,&target,0x55aa)==&stream_registry&&
                !_rwPluginRegistryInvokeRights(&stream_registry,stream_plugin_id+1,&target,0x55aa),
            "rights callback dispatch and unknown plugin");
    Require(registry_calls[0]>=3&&registry_calls[1]==1&&registry_calls[2]==1&&
                registry_calls[3]==1&&registry_calls[4]==1,"plugin callback counts");
    RwFree(memory.start);
}
}
void RegisterTextureStreamPlugin(){offset=RwTextureRegisterPlugin(4,0x123477,Construct,nullptr,nullptr);Require(offset>=int(sizeof(RwTexture)),"texture stream plugin registration");Require(RwTextureRegisterPluginStream(0x123477,Read,Write,Size)==offset,"stream callbacks target texture registry");}
void RegisterPluginRegistryStreamFixture(){
    Require(_rwPluginRegistryAddPlugin(&stream_registry,4,stream_plugin_id,nullptr,nullptr,nullptr)==20,
            "plugin registry add before EngineOpen");
    Require(_rwPluginRegistryAddPluginStream(&stream_registry,stream_plugin_id,RegistryRead,RegistryWrite,RegistrySize)==20,
            "stream callback registration");
    Require(_rwPluginRegistryAddPlgnStrmlwysCB(&stream_registry,stream_plugin_id,RegistryAlways)==20,
            "always callback registration");
    Require(_rwPluginRegistryAddPlgnStrmRightsCB(&stream_registry,stream_plugin_id,RegistryRights)==20,
            "rights callback registration");
    Require(_rwPluginRegistryAddPluginStream(&stream_registry,stream_plugin_id+1,RegistryRead,RegistryWrite,RegistrySize)==-1,
            "unknown plugin rejected");
}
void RunTextureStreamCase(std::string_view name){if(name=="texstream_layout")Layout();else if(name=="texstream_fresh")Fresh();else if(name=="texstream_cached")Cached();else if(name=="texstream_plugin_failure")Failure();else if(name=="texstream_legacy_truncated")LegacyAndTruncated();else if(name=="texstream_plugin_registry")PluginRegistryStream();else throw std::runtime_error("unknown texture stream case");}
}
