#include "renderware_stream_cases.h"
#include "pal4/data_stream_buffer_native.h"
#include <windows.h>
#include <rwcore.h>
#include <rtfsyst.h>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <cstdio>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace pal4::test {
namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
struct DropStream { void operator()(RwStream* p) const { if (p) RwStreamClose(p, nullptr); } };
using Stream = std::unique_ptr<RwStream, DropStream>;
Stream Open(RwStreamType type, RwStreamAccessType access, const void* data) {
    Stream s(RwStreamOpen(type, access, data)); Require(s != nullptr, "stream open"); return s;
}
struct Memory {
    RwMemory value{};
    ~Memory() { if (value.start) RwFree(value.start); }
    void Take(Stream& stream) { Require(RwStreamClose(stream.release(), &value), "memory stream close"); }
};
Stream ReadMemory(void* data, RwUInt32 size) {
    const RwMemory memory{static_cast<RwUInt8*>(data), size};
    return Open(rwSTREAMMEMORY, rwSTREAMREAD, &memory);
}
void MemoryStreams() {
    std::array<RwUInt8, 700> data{};
    for (size_t i=0;i<data.size();++i) data[i]=static_cast<RwUInt8>(i*17);
    Memory output; auto writer=Open(rwSTREAMMEMORY,rwSTREAMWRITE,nullptr);
    Require(RwStreamWrite(writer.get(),data.data(),500)==writer.get(),"first memory write");
    Require(RwStreamWrite(writer.get(),data.data()+500,200)==writer.get(),"growing memory write");
    output.Take(writer);
    Require(output.value.length==700 && std::memcmp(output.value.start,data.data(),700)==0,"memory close transfers used length and payload");
    auto append=Open(rwSTREAMMEMORY,rwSTREAMAPPEND,&output.value); output.value.start=nullptr;
    const RwUInt8 tail=0xa7;Require(RwStreamWrite(append.get(),&tail,1)==append.get(),"append realloc");output.Take(append);
    Require(output.value.length==701 && output.value.start[700]==tail,"append retained data/length");
    auto reader=ReadMemory(output.value.start,output.value.length);
    Require(RwStreamSkip(reader.get(),699)==reader.get(),"relative memory skip");
    std::array<RwUInt8,4> read{0xcc,0xcc,0xcc,0xcc};
    Require(RwStreamRead(reader.get(),read.data(),4)==2 && read[0]==data[699] && read[1]==tail && read[2]==0xcc,"short memory read preserves remainder");
    Require(!RwStreamSkip(reader.get(),1) && reader->Type.memory.position==701,"past-end skip clamps cursor");
    Require(RwStreamSkip(reader.get(),0)==reader.get(),"zero skip succeeds at EOF");
    RwMemory sentinel{reinterpret_cast<RwUInt8*>(1),999};
    Require(RwStreamClose(reader.release(),&sentinel) && sentinel.start==reinterpret_cast<RwUInt8*>(1) && sentinel.length==999,"read close does not transfer/free input");
}
struct Custom {
    std::vector<RwUInt32> writes;
    int read_calls{},skip_calls{},close_calls{};
    RwUInt32 last_skip{}, read_count=2;
    bool write_ok=true,skip_ok=true;
};
RwBool CloseCustom(void* p) { ++static_cast<Custom*>(p)->close_calls; return FALSE; }
RwUInt32 ReadCustom(void* p,void* out,RwUInt32 length) {
    auto& c=*static_cast<Custom*>(p);++c.read_calls;
    const auto n=std::min(length,c.read_count);std::memset(out,0x51,n);return n;
}
RwBool WriteCustom(void* p,const void*,RwUInt32 n) { auto& c=*static_cast<Custom*>(p);c.writes.push_back(n);return c.write_ok; }
RwBool SkipCustom(void* p,RwUInt32 n) { auto& c=*static_cast<Custom*>(p);++c.skip_calls;c.last_skip=n;return c.skip_ok; }
Stream CustomStream(Custom& c) {
    const RwStreamCustom callbacks{CloseCustom,ReadCustom,WriteCustom,SkipCustom,&c};
    return Open(rwSTREAMCUSTOM,rwSTREAMREAD,&callbacks);
}
void CustomStreams() {
    Custom c;auto stream=CustomStream(c);std::array<RwUInt8,8> bytes{};
    Require(RwStreamRead(stream.get(),bytes.data(),8)==2 && c.read_calls==1,"custom count forwarded");
    Require(RwStreamWrite(stream.get(),bytes.data(),8)==stream.get(),"access mode not enforced by write dispatch");
    c.write_ok=false;Require(!RwStreamWrite(stream.get(),bytes.data(),8),"custom write failure");
    Require(RwStreamSkip(stream.get(),0)==stream.get() && c.skip_calls==0,"zero skip avoids callback");
    Require(RwStreamSkip(stream.get(),7)==stream.get() && c.last_skip==7,"custom skip value");
    c.skip_ok=false;Require(!RwStreamSkip(stream.get(),4),"custom skip failure");
    Require(RwStreamClose(stream.release(),nullptr) && c.close_calls==1,"custom close ignores false result");
}
void BinaryStreams() {
    constexpr RwUInt32 type=0x1234;
    for (RwUInt32 version : {0x35000u,0x37002u,0x34000u,0x37003u}) {
        Memory buffer;auto writer=Open(rwSTREAMMEMORY,rwSTREAMWRITE,nullptr);
        Require(_rwStreamWriteVersionedChunkHeader(writer.get(),1,4,0x34000,0)==writer.get(),"unrelated old header");
        RwUInt32 payload=0x78563412;Require(RwStreamWrite(writer.get(),&payload,4)==writer.get(),"chunk body");
        Require(_rwStreamWriteVersionedChunkHeader(writer.get(),type,4,version,0x12345678)==writer.get(),"target header");
        Require(RwStreamWrite(writer.get(),&payload,4)==writer.get(),"target payload");buffer.Take(writer);
        RwUInt32 packed{};std::memcpy(&packed,buffer.value.start+24,4);
        Require((packed&0xffff)==0x5678,"build truncates to16 bits");
        auto reader=ReadMemory(buffer.value.start,buffer.value.length);RwUInt32 length=999,actual=999;
        const bool supported=version>=0x35000 && version<=0x37002;
        Require((RwStreamFindChunk(reader.get(),type,&length,&actual)!=0)==supported,"target version interval");
        Require(reader->Type.memory.position==28,"find leaves cursor at matching payload even if version rejected");
        if(supported) Require(length==4 && actual==version,"decoded header");
        else Require(length==999 && actual==999,"failed version leaves outputs untouched");
    }
    // Legacy header encodes version directly in low16 bits.
    RwUInt32 legacy[]{type,0,0x350};auto reader=ReadMemory(legacy,sizeof(legacy));RwUInt32 version{};
    Require(RwStreamFindChunk(reader.get(),type,nullptr,&version) && version==0x35000,"legacy header version");
    RwUInt32 truncated[]{type,4};reader=ReadMemory(truncated,sizeof(truncated));
    Require(!RwStreamFindChunk(reader.get(),type,nullptr,nullptr) && reader->Type.memory.position==8,"short header consumes available bytes then fails");
    Custom c;auto custom=CustomStream(c);RwInt32 integers[2]{};RwReal reals[130]{};
    c.write_ok=false;
    Require(RwStreamWriteInt16(custom.get(),reinterpret_cast<RwInt16*>(integers),8)==custom.get(),"Int16 ignores write failure");
    Require(RwStreamWriteInt32(custom.get(),integers,8)==custom.get(),"Int32 ignores write failure");
    Require(!RwStreamWriteReal(custom.get(),reals,sizeof(reals)),"real write propagates failure");
    c.write_ok=true;c.writes.clear();
    Require(RwStreamWriteReal(custom.get(),reals,sizeof(reals))==custom.get() && c.writes==std::vector<RwUInt32>({256,256,8}),"real write256-byte blocks");
    Require(RwStreamReadInt16(custom.get(),reinterpret_cast<RwInt16*>(integers),8)==custom.get(),"Int16 accepts short nonzero read");
    Require(RwStreamReadInt32(custom.get(),integers,8)==custom.get(),"Int32 accepts short nonzero read");
    Require(RwStreamReadReal(custom.get(),reals,8)==custom.get(),"real accepts short nonzero read");
    c.read_count=0;Require(!RwStreamReadInt16(custom.get(),reinterpret_cast<RwInt16*>(integers),0),"zero-byte typed read fails");
}
void AllocationFailure(bool& fail_next) {
    Memory buffer;auto writer=Open(rwSTREAMMEMORY,rwSTREAMWRITE,nullptr);std::array<RwUInt8,600> data{};
    fail_next=true;Require(!RwStreamWrite(writer.get(),data.data(),1) && !fail_next,"initial buffer allocation fails");
    Require(writer->Type.memory.memBlock==nullptr && writer->Type.memory.position==0,"initial failure keeps empty stream");
    Require(RwStreamWrite(writer.get(),data.data(),500)==writer.get(),"retry initial allocation");
    auto* old=writer->Type.memory.memBlock;const auto size=writer->Type.memory.nSize;
    fail_next=true;Require(!RwStreamWrite(writer.get(),data.data(),600) && !fail_next,"reallocation fails");
    Require(writer->Type.memory.memBlock==old && writer->Type.memory.nSize==size && writer->Type.memory.position==500,"reallocation failure retains prior buffer and position");
    Require(RwStreamWrite(writer.get(),data.data(),600)==writer.get(),"retry growth");buffer.Take(writer);
    Require(buffer.value.length==1100,"retry output length");
}
RwObject* CountObject(RwObject* object,void* count) { ++*static_cast<int*>(count);return object; }
int ObjectCount(RwFrame* frame) { int count=0;RwFrameForAllObjects(frame,CountObject,&count);return count; }
void FrameAttachment() {
    auto* a=RwFrameCreate();auto* b=RwFrameCreate();auto* camera=RwCameraCreate();
    Require(a && b && camera,"frame attachment fixtures");
    RwCameraSetFrame(camera,a);Require(ObjectCount(a)==1 && RwCameraGetFrame(camera)==a,"initial object link");
    RwCameraSetFrame(camera,b);Require(ObjectCount(a)==0 && ObjectCount(b)==1,"move object between lists");
    RwCameraSetFrame(camera,b);Require(ObjectCount(b)==1,"same-frame reassignment has one link");
    RwCameraSetFrame(camera,nullptr);Require(ObjectCount(b)==0 && !RwCameraGetFrame(camera),"set-null unlinks and clears parent");
    RwCameraSetFrame(camera,a);Require(RwCameraDestroy(camera) && ObjectCount(a)==0,"destroy release-frame unlinks without owning frame");
    Require(RwFrameDestroy(a) && RwFrameDestroy(b),"caller-owned frames survive camera");
}
int allocated{},freed{};
std::uint8_t* GameAllocate(std::uint32_t n) { ++allocated;auto* p=static_cast<std::uint8_t*>(std::malloc(n));Require(p!=nullptr,"game fixture allocation");std::memset(p,0xcc,n);return p; }
void GameFree(std::uint8_t* p) { ++freed;std::free(p); }
void GameBuffer() {
    NativeDataStreamBufferRuntime runtime(GameAllocate,GameFree);
    std::array<RwUInt8,8> input{4,0,0,0,'A','B','C','X'};auto reader=ReadMemory(input.data(),8);
    auto* output=GameAllocate(3);DataStreamBuffer::ReadInto(reader.get(),output,runtime);
    Require(allocated==2 && freed==1 && std::memcmp(output,"ABC\0",4)==0,"game length-prefixed buffer and forced terminator");GameFree(output);
    reader=ReadMemory(input.data(),6);output=nullptr;DataStreamBuffer::ReadInto(reader.get(),output,runtime);
    Require(output[0]=='A' && output[1]=='B' && output[2]==0xcc && output[3]==0,"game ignores short read; untouched allocation bytes survive");GameFree(output);
    RwUInt32 zero=0;reader=ReadMemory(&zero,4);output=GameAllocate(3);const auto dangling=reinterpret_cast<std::uintptr_t>(output);
    DataStreamBuffer::ReadInto(reader.get(),output,runtime);
    Require(reinterpret_cast<std::uintptr_t>(output)==dangling && allocated==freed,"zero-length input leaves freed output value; do not free twice");
}
void FileStreams(bool append_diagnostic) {
    const auto directory=std::filesystem::current_path()/("rw_stream_fixture_"+std::to_string(GetCurrentProcessId())+"_"+std::to_string(GetTickCount64()));
    Require(std::filesystem::create_directory(directory),"stream fixture directory");
    struct Files { std::filesystem::path dir;bool open{};~Files(){if(open)RtFSManagerClose();std::error_code e;std::filesystem::remove_all(dir,e);} } files{directory};
    Require(RtFSManagerOpen(1),"stream filesystem open");files.open=true;
    auto prefix=directory.string()+"\\";char name[]="str";auto* fs=RtWinFSystemInit(8,prefix.data(),name);
    Require(fs && RtFSManagerRegister(fs),"stream filesystem register");RtFSManagerSetDefaultFileSystem(fs);
    auto writer=Open(rwSTREAMFILENAME,rwSTREAMWRITE,"data.bin");const RwUInt8 data[]{1,2,3,4};
    Require(RwStreamWrite(writer.get(),data,4)==writer.get(),"filename write");writer.reset();
    if (append_diagnostic) {
        const RwUInt8 more[]{9,10};auto append=Open(rwSTREAMFILENAME,rwSTREAMAPPEND,"data.bin");
        Require(RwStreamWrite(append.get(),more,2)==append.get(),"append-mode write");append.reset();
        std::ifstream disk(directory/"data.bin",std::ios::binary);
        const std::vector<unsigned char> actual{std::istreambuf_iterator<char>(disk),{}};disk.close();
        Require(actual==std::vector<unsigned char>({9,10,3,4}),"pinned SDK append diagnostic changed; re-evaluate backend");
        std::puts("SDK APPEND VARIANT: expected original1,2,3,4,9,10; native9,10,3,4. Original7D8150 seeks FILE_END; matching backend/adaptation pending.");
        return;
    }
    auto file_owner=Open(rwSTREAMFILENAME,rwSTREAMREAD,"data.bin");
    auto* file=file_owner->Type.file.fpFile;
    auto reader=Open(rwSTREAMFILE,rwSTREAMREAD,file);Require(RwStreamSkip(reader.get(),3)==reader.get(),"borrowed file relative skip");
    std::array<RwUInt8,5> read{};
    Require(RwStreamRead(reader.get(),read.data(),5)==1 && read[0]==4,"file EOF short read");reader.reset();
    Require(RwStreamRead(file_owner.get(),read.data(),1)==0,"borrowed stream shares owner's cursor");
    Require(RwStreamClose(file_owner.release(),nullptr),"borrowed handle remains valid for owner close");
    Require(!RwStreamOpen(rwSTREAMFILENAME,rwSTREAMREAD,"missing.bin"),"missing filename fails");
}
}
void RunStreamCase(std::string_view name,bool& fail_next) {
    if(name=="stream_memory")MemoryStreams();
    else if(name=="stream_custom")CustomStreams();
    else if(name=="stream_binary")BinaryStreams();
    else if(name=="stream_failure")AllocationFailure(fail_next);
    else if(name=="stream_frame")FrameAttachment();
    else if(name=="stream_game_buffer")GameBuffer();
    else if(name=="stream_file")FileStreams(false);
    else if(name=="stream_file_append_variant")FileStreams(true);
    else throw std::runtime_error("unknown stream case");
}
}
