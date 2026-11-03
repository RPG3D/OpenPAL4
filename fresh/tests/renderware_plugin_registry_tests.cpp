#include <rwcore.h>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

static_assert(sizeof(RwPluginRegistry)==24 && sizeof(RwPluginRegEntry)==60);
static_assert(offsetof(RwPluginRegEntry,constructCB)==32 && offsetof(RwPluginRegEntry,copyCB)==40);
static_assert(offsetof(RwPluginRegEntry,nextRegEntry)==48 && offsetof(RwPluginRegEntry,parentRegistry)==56);
static_assert(std::is_same_v<RwPluginObjectCopy,void* (__cdecl *)(void*,const void*,RwInt32,RwInt32)>);
static_assert(std::is_same_v<decltype(&_rwPluginRegistryCopyObject),const RwPluginRegistry* (__cdecl *)(const RwPluginRegistry*,void*,const void*)>);
namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::set<void*> allocations;
bool fail_next{},invalid_free{};
void* Allocate(size_t n,RwUInt32){if(fail_next){fail_next=false;return nullptr;}auto* p=std::malloc(n);if(p)allocations.insert(p);return p;}
void Free(void* p){if(p&&allocations.erase(p)!=1)invalid_free=true;std::free(p);}
void* Reallocate(void* p,size_t n,RwUInt32 hint){
    if(!p)return Allocate(n,hint);if(fail_next){fail_next=false;return nullptr;}
    if(!n){Free(p);return nullptr;}auto* q=std::realloc(p,n);if(q){allocations.erase(p);allocations.insert(q);}return q;
}
void* Callocate(size_t n,size_t size,RwUInt32 hint){auto* p=Allocate(n*size,hint);if(p)std::memset(p,0,n*size);return p;}
RwMemoryFunctions memory{Allocate,Free,Reallocate,Callocate};
struct Engine {
    bool active{};
    explicit Engine(bool pools=true){Check(RwEngineInit(&memory,pools?rwENGINEINITFREELISTS:rwENGINEINITNOFREELISTS,0),"engine init");active=true;}
    void Close(){Check(RwEngineTerm(),"engine term");active=false;Check(allocations.empty()&&!invalid_free,"all allocations freed once");}
    ~Engine(){if(active)RwEngineTerm();}
};
RwPluginRegistry Registry(int capacity=0){return {16,16,capacity,0,nullptr,nullptr};}
int Add(RwPluginRegistry& r,int size,unsigned id,RwPluginObjectConstructor c=nullptr,RwPluginObjectDestructor d=nullptr,RwPluginObjectCopy copy=nullptr){return _rwPluginRegistryAddPlugin(&r,size,id,c,d,copy);}
void Empty(const RwPluginRegistry& r){Check(r.sizeOfStruct==r.origSizeOfStruct&&!r.firstRegEntry&&!r.lastRegEntry,"term resets registry metadata");}
void Layout(){
    auto r=Registry(28); // Registry storage must outlive EngineTerm, including failure unwind.
    Check(Add(r,1,1)==-1,"registration rejected before engine init");
    Engine engine;
    Check(Add(r,1,1)==16&&Add(r,5,2)==20&&r.sizeOfStruct==28,"four-byte extension rounding");
    auto* first=r.firstRegEntry;auto* last=r.lastRegEntry;
    Check(first->size==1&&last->size==5&&first->nextRegEntry==last&&last->prevRegEntry==first,"requested size retained and links ordered");
    Check(first->parentRegistry==&r&&last->parentRegistry==&r&&!first->prevRegEntry&&!last->nextRegEntry,"parent and list ends");
    Check(!first->readCB&&!first->writeCB&&!first->getSizeCB&&!first->alwaysCB&&!first->rightsCB&&!first->errStrCB,"stream slots initially empty");
    Check(Add(r,100,1)==16&&r.sizeOfStruct==28&&r.lastRegEntry==last,"duplicate returns original offset before capacity check");
    Check(Add(r,1,3)==-1&&r.lastRegEntry==last&&r.sizeOfStruct==28,"capacity failure is transactional");
    Check(_rwPluginRegistryGetPluginOffset(&r,2)==20&&_rwPluginRegistryGetPluginOffset(&r,999)==-1,"ID lookup");
    Check(Add(r,0,4)==28&&r.sizeOfStruct==28&&r.lastRegEntry->size==0,"zero-size callback registration fits full capacity");
    engine.Close();Empty(r);Check(Add(r,4,5)==-1,"registration rejected after term");
}
std::vector<int> events;
int fail_constructor=-1;
void* expected_dst{};
const void* expected_src{};
void* Construct(void* p,int offset,int size){Check(p==expected_dst&&size==4,"constructor ABI");events.push_back(offset);return offset==fail_constructor?nullptr:p;}
void* Destruct(void* p,int offset,int size){Check(p==expected_dst&&size==4,"destructor ABI");events.push_back(-offset);return nullptr;}
void* Copy(void* dst,const void* src,int offset,int size){Check(dst==expected_dst&&src==expected_src&&size==4,"four-argument copy ABI");events.push_back(offset);std::memcpy(static_cast<char*>(dst)+offset,static_cast<const char*>(src)+offset,size);return nullptr;}
void RegisterThree(RwPluginRegistry& r){for(int i=0;i<3;++i)Check(Add(r,4,100+i,Construct,Destruct,Copy)==16+4*i,"callback registration");}
void Rollback(){
    auto r=Registry();Engine engine;RegisterThree(r);std::array<char,28> object{};expected_dst=object.data();
    for(int failure:{16,20,24}){
        events.clear();fail_constructor=failure;Check(!_rwPluginRegistryInitObject(&r,object.data()),"constructor failure propagates");
        std::vector<int> expected;for(int i=16;i<=failure;i+=4)expected.push_back(i);for(int i=failure-4;i>=16;i-=4)expected.push_back(-i);
        Check(events==expected,"only earlier successful constructors unwind in reverse; failing plugin not destroyed");
    }
    fail_constructor=-1;events.clear();Check(_rwPluginRegistryInitObject(&r,object.data())==&r&&events==std::vector<int>{16,20,24},"retry succeeds in registration order");
    events.clear();Check(_rwPluginRegistryDeInitObject(&r,object.data())==&r&&events==std::vector<int>{-24,-20,-16},"destructors reverse order and null returns ignored");
    events.clear();engine.Close();Check(events.empty(),"registration teardown does not destruct objects");Empty(r);
}
void CopyFailure(){
    auto r=Registry();Engine engine;RegisterThree(r);std::array<char,28> src{},dst{};src.fill(0x35);dst.fill(0x19);expected_dst=dst.data();expected_src=src.data();
    Check(_rwPluginRegistryCopyObject(&r,dst.data(),src.data())==&r&&events==std::vector<int>{16,20,24},"copy ignores each null result and continues forward");
    Check(dst[0]==0x19&&std::memcmp(dst.data()+16,src.data()+16,12)==0,"callbacks alone copy extension; base object untouched");
    engine.Close();
}
void Defaults(){
    auto r=Registry();Engine engine;Check(Add(r,7,1)==16,"default callbacks registration");
    std::array<char,24> src{},dst{};src.fill(0x55);dst.fill(0x21);auto before=dst;
    Check(_rwPluginRegistryInitObject(&r,dst.data())==&r&&dst==before,"missing constructor does not initialize bytes");
    Check(_rwPluginRegistryCopyObject(&r,dst.data(),src.data())==&r&&dst==before,"missing copy does not memcpy");
    Check(_rwPluginRegistryDeInitObject(&r,dst.data())==&r&&dst==before,"missing destructor no-op");engine.Close();Empty(r);
}
void AllocationFailure(){
    auto r=Registry();RwPluginRegistrySetFreeListCreateParams(1,0);Engine engine;
    // Each block holds one registration. Failed block growth must leave this registry unchanged.
    fail_next=true;Check(Add(r,4,1)==-1&&!fail_next,"registration pool allocation failure");Empty(r);
    Check(Add(r,4,1)==16,"registration retry");auto* first=r.firstRegEntry;
    fail_next=true;Check(Add(r,4,2)==-1&&!fail_next&&r.sizeOfStruct==20&&r.lastRegEntry==first&&!first->nextRegEntry,"failed append preserves size/links");
    Check(Add(r,4,2)==20,"append retry");engine.Close();Empty(r);
}
void NoFreeLists(){
    auto a=Registry(),b=Registry();Engine engine(false);Check(Add(a,4,1)==16&&Add(b,5,2)==16,"direct allocation registries");
    const auto before=allocations.size();fail_next=true;
    // Registry already tracked: avoids the original unchecked tracking-vector allocation path.
    Check(Add(a,4,3)==-1&&!fail_next&&allocations.size()==before&&a.sizeOfStruct==20,"direct entry failure keeps existing registration");
    Check(Add(a,4,3)==20&&Add(b,99,2)==16,"direct retry and duplicate");engine.Close();Empty(a);Empty(b);
    {Engine second(false);Check(Add(a,4,1)==16&&Add(b,4,2)==16,"registry reuse across fresh engine");second.Close();}Empty(a);Empty(b);
}
void CloseFailure(){
    auto r=Registry();Engine engine;Check(Add(r,4,1)==16,"registration before failed enumeration");
    auto* first=r.firstRegEntry;fail_next=true;engine.Close();Check(!fail_next,"close enumeration scratch failed");
    // SDK ignores enumeration failure and still destroys pool. Never dereference this dangling value.
    Check(r.sizeOfStruct==20&&r.firstRegEntry==first&&r.lastRegEntry==first,"SDK leaves stale registry metadata after scratch failure");
    r=Registry();
}
}
int main(int argc,char** argv){try{
    Check(argc==2,"one case required");const std::string_view c=argv[1];
    if(c=="layout")Layout();else if(c=="rollback")Rollback();else if(c=="copy")CopyFailure();else if(c=="defaults")Defaults();
    else if(c=="allocation")AllocationFailure();else if(c=="no_freelists")NoFreeLists();else if(c=="close_failure")CloseFailure();else throw std::runtime_error("unknown case");
    std::printf("plugin registry %s passed\n",argv[1]);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"plugin registry failed: %s\n",e.what());return 1;}}
