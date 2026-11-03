#include <rwcore.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

static_assert(sizeof(RwFreeList)==36);
static_assert(std::is_same_v<decltype(&RwFreeListCreateAndPreallocateSpace),RwFreeList* (__cdecl *)(RwInt32,RwInt32,RwInt32,RwInt32,RwFreeList*,RwUInt32)>);
static_assert(std::is_same_v<decltype(&RwFreeListForAllUsed),RwFreeList* (__cdecl *)(RwFreeList*,RwFreeListCallBack,void*)>);
namespace {
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
struct Allocation {size_t size;RwUInt32 hint;};
std::map<void*,Allocation> allocations;
std::vector<Allocation> requests;
int fail_after=-1;
bool invalid_free{};
bool Fail(){if(fail_after<0)return false;if(fail_after--==0){fail_after=-1;return true;}return false;}
void* Allocate(size_t size,RwUInt32 hint){requests.push_back({size,hint});if(Fail())return nullptr;void* p=std::malloc(size);if(p)allocations[p]={size,hint};return p;}
void Free(void* p){if(p&&allocations.erase(p)!=1)invalid_free=true;std::free(p);}
void* Reallocate(void* p,size_t size,RwUInt32 hint){
    if(!p)return Allocate(size,hint);requests.push_back({size,hint});if(Fail())return nullptr;
    if(!size){Free(p);return nullptr;}auto* next=std::realloc(p,size);if(next){allocations.erase(p);allocations[next]={size,hint};}return next;
}
void* Callocate(size_t n,size_t size,RwUInt32 hint){void* p=Allocate(n*size,hint);if(p)std::memset(p,0,n*size);return p;}
RwMemoryFunctions memory{Allocate,Free,Reallocate,Callocate};
struct Engine {
    bool active{};
    explicit Engine(bool pools=true,bool custom=true){Check(RwEngineInit(custom?&memory:nullptr,pools?rwENGINEINITFREELISTS:rwENGINEINITNOFREELISTS,0),"memory engine init");active=true;}
    void Close(){Check(RwEngineTerm(),"memory engine term");active=false;Check(allocations.empty()&&!invalid_free,"engine memory released exactly once");}
    ~Engine(){if(active)RwEngineTerm();}
};
struct Pool {
    RwFreeList* p{};
    explicit Pool(int size,int count,int alignment=4,int preallocate=0,RwFreeList* storage=nullptr,RwUInt32 hint=0x12345678):p(RwFreeListCreateAndPreallocateSpace(size,count,alignment,preallocate,storage,hint)){Check(p,"pool create");}
    ~Pool(){if(p)RwFreeListDestroy(p);}
};
void Collect(void* entry,void* user){static_cast<std::vector<void*>*>(user)->push_back(entry);}
std::vector<void*> Entries(RwFreeList* p){std::vector<void*> entries;Check(RwFreeListForAllUsed(p,Collect,&entries)==p,"pool enumeration result");return entries;}
void Alignment(){
    Engine engine;
    for(int alignment:{0,4,16,64}){
        Pool pool(13,10,alignment);const unsigned expectedAlignment=alignment?alignment:4;
        Check(pool.p->entrySize==(13+expectedAlignment-1)/expectedAlignment*expectedAlignment&&pool.p->heapSize==2,"entry rounding and partial bitmap byte");
        std::vector<void*> pointers;for(int i=0;i<21;++i){auto* p=RwFreeListAlloc(pool.p,0xabc00001);Check(p&&reinterpret_cast<std::uintptr_t>(p)%expectedAlignment==0,"native entry alignment");std::memset(p,i,13);pointers.push_back(p);}
        Check(std::set<void*>(pointers.begin(),pointers.end()).size()==21,"distinct entries across three blocks");
        const auto entries=Entries(pool.p);Check(std::set<void*>(entries.begin(),entries.end())==std::set<void*>(pointers.begin(),pointers.end()),"all used bitmap entries enumerated once");
        for(int i=0;i<21;++i)Check(static_cast<unsigned char*>(pointers[i])[0]==i,"growth preserves occupied entries");
        auto* reusable=pointers.back();Check(RwFreeListFree(pool.p,reusable)==pool.p,"free result is list");
        pointers.back()=RwFreeListAlloc(pool.p,0); // Last block may have been freed; no address reuse promise.
        Check(pointers.back(),"allocate after empty-block release");for(auto* p:pointers)Check(RwFreeListFree(pool.p,p)==pool.p,"valid aligned free");
        Check(Entries(pool.p).empty(),"all blocks empty after frees");
    }
    engine.Close();
}
void ReuseAndPurge(){
    Engine engine;
    {
        Pool pool(13,10,4,2);
        const auto flags=RwFreeListGetFlags(pool.p);RwFreeListSetFlags(pool.p,0);
        Check(RwFreeListGetFlags(pool.p)==flags,"SDK SetFlags ORs flags; zero does not clear FREEBLOCKS");
        RwFreeListSetFlags(pool.p,rwFREELISTFLAG_STATIC);
        Check(RwFreeListGetFlags(pool.p)==flags,"SDK SetFlags cannot set borrowed-storage bit");
        auto* first=RwFreeListAlloc(pool.p,0);auto* second=RwFreeListAlloc(pool.p,0);
        std::memset(first,0x65,13);RwFreeListFree(pool.p,first);auto* reused=RwFreeListAlloc(pool.p,0);
        Check(reused==first&&static_cast<unsigned char*>(reused)[0]==0x65,"free slot in still-live block reused without zeroing");
        RwFreeListFree(pool.p,reused);RwFreeListFree(pool.p,second);
        const auto before=allocations.size();const auto purged=RwFreeListPurge(pool.p);
        std::printf("purge diagnostic: report=%d flags=%u stride=%u allocations freed=%zu\n",purged,RwFreeListGetFlags(pool.p),pool.p->entrySize,before-allocations.size());
        Check(purged==16,"purge reports one untouched preallocated block times entry size");
        Check(allocations.size()+1==before&&RwFreeListPurge(pool.p)==0,"used block freed immediately; purge releases remaining unused block once");
    }
    {
        // Isolate global purge's contribution by purging engine empties first.
        (void)RwFreeListPurgeAllFreeLists();Pool a(20,10,4,2),b(28,10,4,3);
        Check(RwFreeListPurgeAllFreeLists()==124,"global purge sums entry-size-based reports");
    }
    engine.Close();
}
struct DrainState {RwFreeList* pool;std::set<void*> seen;};
void Drain(void* entry,void* user){auto& state=*static_cast<DrainState*>(user);Check(state.seen.insert(entry).second,"drain callback duplicate");Check(RwFreeListFree(state.pool,entry)==state.pool,"callback frees current entry");}
void Enumeration(){
    Engine engine;
    {
        Pool pool(12,10);for(int i=0;i<23;++i)Check(RwFreeListAlloc(pool.p,0),"drain source");
        DrainState state{pool.p};Check(RwFreeListForAllUsed(pool.p,Drain,&state)==pool.p&&state.seen.size()==23&&Entries(pool.p).empty(),"bitmap snapshot permits freeing every current entry and block during enumeration");
        auto* p=RwFreeListAlloc(pool.p,0);const auto before=allocations.size();fail_after=0;std::vector<void*> visited;
        Check(!RwFreeListForAllUsed(pool.p,Collect,&visited)&&visited.empty()&&fail_after==-1,"enumeration scratch allocation failure propagates null");
        Check(allocations.size()==before&&Entries(pool.p)==std::vector<void*>{p},"failed enumeration retains entries for retry");RwFreeListFree(pool.p,p);
    }
    engine.Close();
}
void Rollback(){
    Engine engine;RwFreeList storage{};
    const auto before=allocations.size();requests.clear();fail_after=1;
    Check(!RwFreeListCreateAndPreallocateSpace(24,10,8,3,&storage,0xabc12345)&&fail_after==-1,"second preallocated block failure");
    Check(allocations.size()==before&&requests.size()==2,"failed static create releases earlier block, never caller storage");
    {
        Pool pool(24,10,8,0,&storage);const auto count=allocations.size();fail_after=0;
        Check(!RwFreeListAlloc(pool.p,0x5a0000)&&fail_after==-1&&allocations.size()==count,"first dynamic block failure preserves pool");
        Check(Entries(pool.p).empty(),"failed append creates no bitmap entry");auto* p=RwFreeListAlloc(pool.p,0x5a0000);Check(p&&RwFreeListFree(pool.p,p)==pool.p,"block allocation retry");
    }
    engine.Close();
}
void HintsAndStatic(){
    Engine engine;RwFreeList storage{};
    {
        requests.clear();Pool pool(9,10,4,2,&storage,0xabcdef12);
        Check(pool.p==&storage&&(RwFreeListGetFlags(pool.p)&rwFREELISTFLAG_STATIC),"caller-supplied list wrapper is borrowed");
        Check(requests.size()==2,"static wrapper needs only two requested blocks");
        for(auto r:requests)Check(r.size==133&&r.hint==0xabcdef12,"block bytes include bitmap/alignment/link overhead and full hint");
    }
    // Creation with a caller wrapper remains reusable after Destroy.
    {Pool reused(8,8,4,0,&storage);Check(reused.p==&storage,"static wrapper can be initialized again");}
    engine.Close();
}
void NoFreeLists(){
    Engine engine(false);
    {
        RwFreeList storage{};requests.clear();Pool pool(13,10,4,3,&storage);
        Check(requests.empty(),"NOFREELISTS suppresses even explicit preallocation");
        auto* p=RwFreeListAlloc(pool.p,0x654321);Check(p&&allocations.at(p).size==16&&allocations.at(p).hint==0x654321,"NOFREELISTS direct allocation uses aligned entry size and hint");
        Check(Entries(pool.p).empty()&&RwFreeListPurge(pool.p)==0,"direct allocation is absent from block enumeration/purge");
        // Direct free backend's return register is not relied on.
        (void)RwFreeListFree(pool.p,p);Check(!allocations.contains(p),"NOFREELISTS delegates to configured free");
    }
    engine.Close();
}
void MemoryCallbacks(){
    Engine engine;auto* api=RwOsGetMemoryInterface();
    Check(api&&api->rwmalloc==Allocate&&api->rwfree==Free&&api->rwrealloc==Reallocate&&api->rwcalloc==Callocate,"public interface retains all custom callbacks");
    auto* p=static_cast<unsigned char*>(RwMalloc(17,0x789abc));Check(p&&allocations.at(p).hint==0x789abc,"malloc forwards hint");std::memset(p,0x42,17);
    fail_after=0;Check(!RwRealloc(p,40,0x123456)&&allocations.contains(p)&&p[0]==0x42,"realloc failure preserves original allocation");
    p=static_cast<unsigned char*>(RwRealloc(p,40,0x123456));Check(p&&p[16]==0x42&&allocations.at(p).hint==0x123456,"realloc preserves prefix and new hint");RwFree(p);
    auto* zeros=static_cast<unsigned char*>(RwCalloc(7,3,0x987654));Check(zeros&&allocations.at(zeros).size==21,"calloc forwards product");for(int i=0;i<21;++i)Check(zeros[i]==0,"calloc zero fill");RwFree(zeros);
    // MemoryClose destroys registered orphan lists and their blocks on Term.
    auto* orphan=RwFreeListCreate(20,10,4,0);Check(orphan&&RwFreeListAlloc(orphan,0),"orphan list for memory close");engine.Close();
}
void DefaultCallbacks(){
    Engine engine(true,false);auto* api=RwOsGetMemoryInterface();Check(api&&api->rwmalloc&&api->rwrealloc&&api->rwcalloc&&api->rwfree,"default memory wrappers installed");
    auto* p=static_cast<unsigned char*>(api->rwmalloc(17,0xaabbccdd));Check(p,"default malloc accepts ignored hint");std::memset(p,0x37,17);
    p=static_cast<unsigned char*>(api->rwrealloc(p,48,0x11223344));Check(p&&p[16]==0x37,"default realloc wrapper ABI/prefix");api->rwfree(p);
    p=static_cast<unsigned char*>(api->rwcalloc(4,9,0x99887766));Check(p,"default calloc wrapper ABI");for(int i=0;i<36;++i)Check(p[i]==0,"default calloc zeros");api->rwfree(p);engine.Close();
}
}
int main(int argc,char** argv){try{
    Check(argc==2,"expected case");std::string_view name=argv[1];
    if(name=="alignment")Alignment();else if(name=="purge")ReuseAndPurge();else if(name=="enumeration")Enumeration();else if(name=="rollback")Rollback();else if(name=="static_hints")HintsAndStatic();else if(name=="no_freelists")NoFreeLists();else if(name=="callbacks")MemoryCallbacks();else if(name=="defaults")DefaultCallbacks();else throw std::runtime_error("unknown memory case");
    std::puts("native FreeList/memory case passed");return 0;
}catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
