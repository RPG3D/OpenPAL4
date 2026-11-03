#include "support/native_test_display.h"
#include <windows.h>
#include <rwcore.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// Test-only private probe. Product callers use public image/DDS path APIs.
extern "C" RwBool _rwpathisabsolute(const RwChar*);
static_assert(sizeof(RwError)==8 && sizeof(RwStringFunctions)==68);
static_assert(std::is_same_v<decltype(&RwErrorSet),RwError* (__cdecl *)(RwError*)>);
static_assert(std::is_same_v<decltype(&_rwStringStreamWrite),const RwChar* (__cdecl *)(const RwChar*,RwStream*)>);
namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct Allocation {size_t size;RwUInt32 hint;};
std::map<void*,Allocation> allocations;
bool fail_next{},invalid_free{};
void* Allocate(size_t size,RwUInt32 hint){if(fail_next){fail_next=false;return nullptr;}auto* p=std::malloc(size);if(p){allocations[p]={size,hint};std::memset(p,0xcc,size);}return p;}
void Free(void* p){if(p&&allocations.erase(p)!=1)invalid_free=true;std::free(p);}
void* Reallocate(void* p,size_t size,RwUInt32 hint){if(!p)return Allocate(size,hint);if(fail_next){fail_next=false;return nullptr;}if(!size){Free(p);return nullptr;}auto* q=std::realloc(p,size);if(q){allocations.erase(p);allocations[q]={size,hint};}return q;}
void* Callocate(size_t n,size_t size,RwUInt32 hint){auto* p=Allocate(n*size,hint);if(p)std::memset(p,0,n*size);return p;}
struct Session {
    bool initialized{},opened{},started{};HWND window{};
    Session(){const RwMemoryFunctions memory{Allocate,Free,Reallocate,Callocate};Check(RwEngineInit(&memory,0,0),"engine init");initialized=true;
        window=pal4::test::CreateTestWindow("STATIC","PAL4 error/string boundary",WS_OVERLAPPEDWINDOW,64,64,nullptr,nullptr,GetModuleHandleA(nullptr),nullptr);Check(window,"hidden window");
        RwEngineOpenParams params{window};Check(RwEngineOpen(&params),"engine open");opened=true;Check(RwEngineStart(),"engine start");started=true;
    }
    ~Session(){if(started)RwEngineStop();if(opened)RwEngineClose();if(initialized)RwEngineTerm();if(window)DestroyWindow(window);}
};
struct StreamDrop{void operator()(RwStream* s)const{if(s)RwStreamClose(s,nullptr);}};
using Stream=std::unique_ptr<RwStream,StreamDrop>;
Stream Open(RwStreamType type,RwStreamAccessType mode,const void* data){Stream s(RwStreamOpen(type,mode,data));Check(s!=nullptr,"stream open");return s;}
using Bytes=std::vector<RwUInt8>;
Stream Reader(Bytes& bytes){const RwMemory m{bytes.data(),static_cast<RwUInt32>(bytes.size())};return Open(rwSTREAMMEMORY,rwSTREAMREAD,&m);}
Bytes Take(Stream& s){RwMemory m{};Check(RwStreamClose(s.release(),&m),"close writer");Bytes out(m.start,m.start+m.length);RwFree(m.start);return out;}
Bytes Chunk(RwUInt32 type,RwUInt32 declared,const Bytes& payload,RwUInt32 version=0x37002){auto s=Open(rwSTREAMMEMORY,rwSTREAMWRITE,nullptr);Check(_rwStreamWriteVersionedChunkHeader(s.get(),type,declared,version,0x65),"fixture header");if(!payload.empty())Check(RwStreamWrite(s.get(),payload.data(),static_cast<RwUInt32>(payload.size())),"fixture payload");return Take(s);}
RwUInt32 Word(const Bytes& bytes,size_t at){RwUInt32 value;Check(at+4<=bytes.size(),"word bounds");std::memcpy(&value,bytes.data()+at,4);return value;}
RwError Error(){RwError value{};Check(RwErrorGet(&value)==&value,"ErrorGet returns caller pointer");return value;}
void Errors(){
    (void)Error();RwError first{8450,37},second{8451,99};Check(RwErrorSet(&first)==&first&&RwErrorSet(&second)==&second,"Set returns supplied pointer even when ignored");
    auto got=Error();Check(got.pluginID==8450&&got.errorCode==37,"first plugin error remains until Get");got=Error();Check(got.pluginID==0&&got.errorCode==E_RW_NOERROR,"Get resets both fields");
    RwError common{8450,E_RW_NOMEM};RwErrorSet(&common);RwErrorSet(&first);got=Error();Check(got.pluginID==0&&got.errorCode==E_RW_NOMEM,"negative common error clears plugin ID and blocks overwrite");
    RwError zeroPlugin{0,19};RwErrorSet(&zeroPlugin);RwErrorSet(&first);got=Error();Check(got.pluginID==0&&got.errorCode==19,"zero plugin ID still sticky via non-sentinel code");
    RwError sentinel{8450,E_RW_NOERROR};RwErrorSet(&sentinel);RwErrorSet(&second);got=Error();Check(got.pluginID==8451&&got.errorCode==99,"NOERROR sentinel leaves slot available");
    RwError invalid{999,0};RwErrorSet(&invalid);got=Error();Check(got.pluginID==999&&got.errorCode==0,"zero is a plugin code, not NOERROR");
}
void Strings(){
    Check(rwstricmp(nullptr,"x")==0&&rwstricmp("x",nullptr)==0,"null operands compare equal");
    Check(rwstricmp("AbC","aBc")==0&&rwstricmp("Az","aB")==24,"ASCII fold and exact difference");
    const char high[]{char(0x80),0};Check(rwstricmp(high,"")==-128,"signed high-byte comparison retained");
    std::array<char,7> bytes{'a','Z','[',char(0xe1),'b',0,char(0x5a)};
    Check(rwstrupr(bytes.data())==bytes.data()&&bytes==std::array<char,7>{'A','Z','[',char(0xe1),'B',0,char(0x5a)},"upper changes only ASCII before terminator");
    Check(rwstrlwr(bytes.data())==bytes.data()&&bytes==std::array<char,7>{'a','z','[',char(0xe1),'b',0,char(0x5a)},"lower changes only ASCII before terminator");
    Check(!rwstrupr(nullptr)&&!rwstrlwr(nullptr),"case conversion accepts null");
    const char search[]{'a','b','a',char(0xfe),0};
    Check(rwstrchr(search,'a')==search&&rwstrrchr(search,'a')==search+2,"first and last character");
    Check(rwstrchr(search,0)==search+4&&rwstrrchr(search,0)==search+4,"terminator can be found");
    Check(rwstrchr(search,0x1fe)==search+3&&rwstrrchr(search,-2)==search+3,"int character narrows to low byte");
    Check(!rwstrchr(search,'z')&&!rwstrrchr(search,'z'),"absent character");
    std::array<char,64> buffer{};rwstrcpy(buffer.data(),"pal");rwstrcat(buffer.data(),"4");Check(rwstrlen(buffer.data())==4&&rwstrcmp(buffer.data(),"pal4")==0,"installed CRT string slots work");
    rwsprintf(buffer.data(),"%s:%d","PAL",4);int value=0;Check(rwsscanf(buffer.data(),"PAL:%d",&value)==1&&value==4,"installed variadic formatting slots");
}
void Paths(){
    for(const char* name:{"\\root","\\\\server\\share","C:\\file","c:relative","Z:"})Check(_rwpathisabsolute(name),"Windows root/drive classified absolute");
    for(const char* name:{"","file","/root","./file","1:file",":file","a/file"})Check(!_rwpathisabsolute(name),"other spellings remain relative");
}
void Writer(){
    for(int length:{0,1,3,4,63,64,65,300}){
        Bytes source((length+4)&~3,0xa7);std::fill_n(source.begin(),length,'X');source[length]=0;
        Check(_rwStringStreamGetSize(reinterpret_cast<const char*>(source.data()))==source.size(),"full 32-bit padded size");
        auto s=Open(rwSTREAMMEMORY,rwSTREAMWRITE,nullptr);const auto* text=reinterpret_cast<const char*>(source.data());Check(_rwStringStreamWrite(text,s.get())==text,"writer returns original string pointer");auto bytes=Take(s);
        Check(Word(bytes,0)==rwID_STRING&&Word(bytes,4)==source.size()&&bytes.size()==source.size()+12,"string header/padded extent");
        Check((Word(bytes,8)&0xffff)==0xa,"available SDK build0xA differs from original0x65");
        Check(std::equal(source.begin(),source.end(),bytes.begin()+12),"writer serializes caller padding after NUL without zeroing");
    }
    Check(_rwStringStreamGetSize(nullptr)==4,"null string padded size");auto s=Open(rwSTREAMMEMORY,rwSTREAMWRITE,nullptr);auto* result=_rwStringStreamWrite(nullptr,s.get());Check(result&&result[0]==0,"null input maps to SDK static empty string");auto bytes=Take(s);Check(bytes.size()==16&&bytes[12]==0,"empty string chunk; adjacent static padding unspecified");
}
struct WriteState{int calls{},fail_on;std::vector<unsigned> lengths;};
RwBool WriteCustom(void* p,const void*,RwUInt32 n){auto& state=*static_cast<WriteState*>(p);state.lengths.push_back(n);return ++state.calls!=state.fail_on;}
RwBool CloseCustom(void*){return TRUE;}
void WriterFailure(){
    const char source[8]={'a','b','c','d',0,'P','A','D'};
    for(int failure:{1,2}){WriteState state{0,failure};RwStreamCustom funcs{CloseCustom,nullptr,WriteCustom,nullptr,&state};auto s=Open(rwSTREAMCUSTOM,rwSTREAMWRITE,&funcs);
        Check(!_rwStringStreamWrite(source,s.get()),"header or payload failure propagates null");Check(state.calls==failure&&state.lengths.front()==12,"header write first and stop at failure");if(failure==2)Check(state.lengths.back()==8,"padded payload passed in one write");}
}
void Readers(){
    Bytes payload(132,'Q');payload[0]='A';payload[1]=0;payload[2]='Z';payload.back()=0;
    auto bytes=Chunk(rwID_STRING,static_cast<unsigned>(payload.size()),payload);auto s=Reader(bytes);std::array<char,140> caller;caller.fill(char(0xcc));
    Check(_rwStringStreamFindAndRead(caller.data(),s.get())==caller.data()&&std::memcmp(caller.data(),payload.data(),payload.size())==0&&caller[132]==char(0xcc),"ANSI copies all declared bytes including embedded NUL/padding in64-byte blocks");
    s=Reader(bytes);auto* owned=_rwStringStreamFindAndRead(nullptr,s.get());Check(owned&&allocations.at(owned).size==132&&allocations.at(owned).hint==0x30002&&std::memcmp(owned,payload.data(),132)==0,"owned ANSI buffer exact declared length and hint");RwFree(owned);
    Bytes unicode(260);for(unsigned i=0;i<130;++i){unicode[2*i]=static_cast<unsigned char>(i+1);unicode[2*i+1]=0x4e;}unicode[258]=0;
    bytes=Chunk(rwID_UNICODESTRING,260,unicode);s=Reader(bytes);caller.fill(char(0xcc));Check(_rwStringStreamFindAndRead(caller.data(),s.get())==caller.data(),"Unicode read");for(int i=0;i<130;++i)Check(static_cast<unsigned char>(caller[i])==unicode[2*i],"Unicode discards high byte, no codepage conversion");Check(caller[130]==char(0xcc),"no extra terminator after converted units");
    s=Reader(bytes);owned=_rwStringStreamFindAndRead(nullptr,s.get());Check(owned&&allocations.at(owned).size==260&&static_cast<unsigned char>(owned[130])==0xcc,"Unicode allocates original byte count, not narrow character count");RwFree(owned);
    bytes=Chunk(rwID_UNICODESTRING,3,{'A',0x4e,0xff});s=Reader(bytes);caller.fill('!');Check(_rwStringStreamFindAndRead(caller.data(),s.get())==caller.data()&&caller[0]=='A'&&caller[1]=='!'&&s->Type.memory.position==15,"odd Unicode tail consumed and discarded");
    for(auto type:{rwID_STRING,rwID_UNICODESTRING}){bytes=Chunk(type,0,{});s=Reader(bytes);caller.fill('!');Check(_rwStringStreamFindAndRead(caller.data(),s.get())==caller.data()&&caller[0]=='!',"zero-length caller buffer unchanged");}
}
void Versions(){
    for(unsigned version:{0x35000u,0x37002u,0x34000u,0x37003u}){
        (void)Error();auto bytes=Chunk(0x1234,4,{'S','K','I','P'},version);auto tail=Chunk(rwID_STRING,4,{'o','k',0,0});bytes.insert(bytes.end(),tail.begin(),tail.end());auto s=Reader(bytes);char caller[8]{};
        bool accepted=version>=0x35000&&version<=0x37002;Check((_rwStringStreamFindAndRead(caller,s.get())!=nullptr)==accepted,"version tested even for unrelated skipped chunks");
        if(accepted)Check(!std::strcmp(caller,"ok")&&s->Type.memory.position==32,"valid unknown chunk skipped before string");else Check(s->Type.memory.position==12&&Error().errorCode==E_RW_BADVERSION,"invalid preceding chunk rejected before skip");
    }
    auto bytes=Chunk(rwID_STRING,4,{'N','O','N','E'});auto s=Reader(bytes);std::array<char,8> caller;caller.fill('!');Check(_rwStringStreamFindAndRead(caller.data(),s.get())&&caller[4]=='!',"reader does not add missing terminator");
}
void ReadFailure(){
    for(auto type:{rwID_STRING,rwID_UNICODESTRING}){
        const bool unicode=type==rwID_UNICODESTRING;const unsigned declared=unicode?260:132;Bytes payload(unicode?130:70,'A');
        if(unicode)for(size_t i=1;i<payload.size();i+=2)payload[i]=0x4e;
        auto bytes=Chunk(type,declared,payload);auto s=Reader(bytes);std::array<char,300> caller;caller.fill('!');
        Check(!_rwStringStreamFindAndRead(caller.data(),s.get()),"truncated caller-buffer read fails");Check(std::all_of(caller.begin(),caller.begin()+64,[](char c){return c=='A';})&&caller[64]=='!',"only complete first block copied before short second read");
        s=Reader(bytes);const auto before=allocations;Check(!_rwStringStreamFindAndRead(nullptr,s.get()),"truncated owned read fails");
        std::vector<void*> extra;for(auto [p,a]:allocations)if(!before.contains(p))extra.push_back(p);
        if(unicode)Check(extra.empty(),"Unicode failure frees internally allocated buffer");
        else {Check(extra.size()==1&&allocations.at(extra[0]).size==declared,"ANSI failure retains allocated buffer: SDK leak reproduced");RwFree(extra[0]);} // Explicit diagnostic cleanup.
        bytes=Chunk(type,4,{'A',0,0,0});s=Reader(bytes);(void)Error();const auto count=allocations.size();fail_next=true;
        Check(!_rwStringStreamFindAndRead(nullptr,s.get())&&!fail_next&&allocations.size()==count,"allocation failure has no new buffer");Check(s->Type.memory.position==12&&Error().errorCode==E_RW_NOMEM,"allocation error before payload read");
    }
}
}
int main(int argc,char** argv){try{Check(argc==2,"one case required");{Session session;const std::string_view c=argv[1];
    if(c=="errors")Errors();else if(c=="strings")Strings();else if(c=="paths")Paths();else if(c=="writer")Writer();else if(c=="writer_failure")WriterFailure();else if(c=="readers")Readers();else if(c=="versions")Versions();else if(c=="read_failure")ReadFailure();else throw std::runtime_error("unknown case");
    }Check(allocations.empty()&&!invalid_free,"engine/fixture allocations released once");std::printf("error/string %s passed\n",argv[1]);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"error/string failed: %s\n",e.what());return 1;}}
