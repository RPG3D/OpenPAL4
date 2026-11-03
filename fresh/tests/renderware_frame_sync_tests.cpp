#include "support/native_test_display.h"
#include <windows.h>
#include <rwcore.h>
#include "pal4/camera_resource_native.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

static_assert(sizeof(RwChunkGroup)==33 && sizeof(RwObjectHasFrame)==20);
static_assert(offsetof(RwObjectHasFrame,lFrame)==8 && offsetof(RwObjectHasFrame,sync)==16);
static_assert(std::is_same_v<decltype(&_rwFrameSyncHierarchyLTM),void (__cdecl *)(RwFrame*)>);
namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void Near(float x,float expected,const char* why){Check(std::fabs(x-expected)<0.0003f,why);}
struct Allocation{size_t size;RwUInt32 hint;};
std::map<void*,Allocation> allocations;
bool fail_group{},invalid_free{};
void* Allocate(size_t size,RwUInt32 hint){if(fail_group&&hint==0x40412){fail_group=false;return nullptr;}auto* p=std::malloc(size);if(p)allocations[p]={size,hint};return p;}
void Free(void* p){if(p&&allocations.erase(p)!=1)invalid_free=true;std::free(p);}
void* Reallocate(void* p,size_t size,RwUInt32 hint){if(!p)return Allocate(size,hint);if(!size){Free(p);return nullptr;}auto* q=std::realloc(p,size);if(q){allocations.erase(p);allocations[q]={size,hint};}return q;}
void* Callocate(size_t n,size_t size,RwUInt32 hint){auto* p=Allocate(n*size,hint);if(p)std::memset(p,0,n*size);return p;}
struct Session {
    bool initialized{},opened{},started{};HWND window{};
    Session(){const RwMemoryFunctions memory{Allocate,Free,Reallocate,Callocate};Check(RwEngineInit(&memory,0,0),"engine init");initialized=true;
        window=pal4::test::CreateTestWindow("STATIC","PAL4 frame sync",WS_OVERLAPPEDWINDOW,64,64,nullptr,nullptr,GetModuleHandleA(nullptr),nullptr);Check(window,"hidden window");
        RwEngineOpenParams params{window};Check(RwEngineOpen(&params),"engine open");opened=true;Start();}
    void Start(){Check(RwEngineStart(),"engine start");started=true;}
    void Stop(){Check(RwEngineStop(),"engine stop");started=false;}
    ~Session(){if(started)RwEngineStop();if(opened)RwEngineClose();if(initialized)RwEngineTerm();if(window)DestroyWindow(window);}
};
RwFrame* Frame(){auto* f=RwFrameCreate();Check(f,"frame create");return f;}
void Position(RwFrame* f,float x,float y=0,float z=0){const RwV3d p{x,y,z};Check(RwFrameTranslate(f,&p,rwCOMBINEREPLACE)==f,"frame translate");}
void At(const RwMatrix& m,float x,float y){Near(m.pos.x,x,"world x");Near(m.pos.y,y,"world y");}
struct Tree {
    RwFrame* root=Frame();RwFrame* a=Frame();RwFrame* b=Frame();RwFrame* leaf=Frame();
    Tree(){RwFrameAddChild(root,a);RwFrameAddChild(root,b);RwFrameAddChild(a,leaf);
        const RwV3d axis{0,0,1};RwFrameRotate(root,&axis,90,rwCOMBINEREPLACE);const RwV3d move{10,20,0};RwFrameTranslate(root,&move,rwCOMBINEPOSTCONCAT);
        Position(a,2,3);Position(b,7,0);Position(leaf,4,5);}
    ~Tree(){RwFrameDestroyHierarchy(root);}
};
struct Event{int id;RwV3d position;unsigned flags;};
std::vector<Event> events;
struct Observer {
    RwObjectHasFrame object{};int id;bool reject{};RwFrame* reschedule{};
    Observer(RwFrame* frame,int id):id(id){rwObjectHasFrameInitialize(&object,0xee,0,Sync);_rwObjectHasFrameSetFrame(&object,frame);}
    ~Observer(){_rwObjectHasFrameSetFrame(&object,nullptr);}
    static RwObjectHasFrame* Sync(RwObjectHasFrame* p){auto& self=*reinterpret_cast<Observer*>(p);auto* frame=static_cast<RwFrame*>(p->object.parent);events.push_back({self.id,frame->ltm.pos,frame->object.privateFlags});
        if(self.reschedule){auto* target=self.reschedule;self.reschedule=nullptr;Position(target,99);}
        return self.reject?nullptr:p;}
};
static_assert(offsetof(Observer,object)==0);
std::vector<int> IDs(){std::vector<int> ids;for(auto e:events)ids.push_back(e.id);return ids;}
void MatrixOnly(){
    Tree t;Observer root(t.root,1),leaf(t.leaf,2);events.clear();
    Check(RwFrameGetLTM(t.leaf)==&t.leaf->ltm,"public GetLTM result");Check(events.empty(),"lazy matrix sync does not call objects");
    At(t.root->ltm,10,20);At(t.a->ltm,7,22);At(t.b->ltm,10,27);At(t.leaf->ltm,2,26);
    Check(!(RwFrameDirty(t.root)&1)&&(RwFrameDirty(t.root)&2),"GetLTM clears hierarchy LTM bit, preserves object dirty bit");
    Check(!(t.a->object.privateFlags&4)&&!(t.leaf->object.privateFlags&4),"recursive LTM dirty bits cleared");
    Position(t.a,3,4);At(*RwFrameGetLTM(t.leaf),1,27);At(t.b->ltm,10,27);Check(events.empty(),"branch recompute preserves sibling and defers object callbacks");
    Check(_rwFrameSyncDirty()&&IDs()==std::vector<int>{1,2},"queued object callbacks delivered after matrix-only pass");Check(!RwFrameDirty(t.root),"dirty bits clear after object sync");
}
void Dirty(){
    Tree t;Observer r1(t.root,1),r2(t.root,2),a(t.a,3),b(t.b,4),leaf(t.leaf,5);r2.reject=true;
    Check(_rwFrameSyncDirty(),"initial flush");events.clear();Position(t.leaf,9,1);RwFrameUpdateObjects(t.leaf);RwFrameUpdateObjects(t.leaf);
    Check(_rwFrameSyncDirty()&&IDs()==std::vector<int>{2,1,4,3,5},"one queue entry, reverse attachment and depth-first frame order; null callback does not stop");
    At(t.leaf->ltm,6,31);At(t.a->ltm,7,22);At(t.b->ltm,10,27);
    Check(events.size()==5&&events.back().position.x==t.leaf->ltm.pos.x,"callback sees newly calculated LTM");
    Check(!RwFrameDirty(t.root)&&!(t.a->object.privateFlags&12)&&!(t.leaf->object.privateFlags&12),"sync clears dirty bits throughout hierarchy");
    events.clear();Check(_rwFrameSyncDirty()&&events.empty(),"empty second flush does not repeat callbacks");
    // Global dirty roots are inserted at the front, independently of their trees.
    auto* other=Frame();{Observer o(other,6);Position(t.root,1);Position(other,2);events.clear();_rwFrameSyncDirty();Check(IDs()==std::vector<int>{2,1,4,3,5,6},"already queued hierarchy is not promoted by later edits");
        Position(t.root,3);Position(other,4);events.clear();_rwFrameSyncDirty();Check(IDs()==std::vector<int>{6,2,1,4,3,5},"newly queued dirty hierarchy first");}RwFrameDestroy(other);
}
void NoLtm(){
    Tree t;Observer root(t.root,1),a(t.a,2),leaf(t.leaf,3);(void)RwFrameGetLTM(t.leaf);events.clear();
    // Deliberate unannounced matrix edits distinguish the object-only traversal.
    const auto saved=t.root->modelling.pos;t.root->modelling.pos={99,88,77};
    Check(_rwFrameSyncDirty()&&IDs()==std::vector<int>{1,2,3},"object-only recursion still visits entire hierarchy");
    At(t.root->ltm,10,20);At(t.leaf->ltm,2,26);Check(!RwFrameDirty(t.root),"object-only pass clears queue without recomputing LTM");
    t.root->modelling.pos=saved;RwMatrixUpdate(&t.root->modelling);RwFrameUpdateObjects(t.root);_rwFrameSyncDirty();
}
void Reschedule(){
    auto* frame=Frame();{Observer o(frame,1);Position(frame,10);o.reschedule=frame;events.clear();Check(_rwFrameSyncDirty()&&events.size()==1,"callback mutates its active root");
        Near(frame->modelling.pos.x,99,"callback changed model");Near(frame->ltm.pos.x,10,"current LTM predates callback change");Check(!RwFrameDirty(frame),"current-root callback requeue is lost when cached flags cleared");
        events.clear();_rwFrameSyncDirty();Check(events.empty(),"lost callback reschedule not delivered next pass");Near(RwFrameGetLTM(frame)->pos.x,10,"lazy lookup also returns stale LTM after lost invalidation");
        RwFrameUpdateObjects(frame);_rwFrameSyncDirty();Near(frame->ltm.pos.x,99,"explicit later invalidation repairs state");}
    RwFrameDestroy(frame);
}
void GroupLifecycle(Session& session){
    auto* a=RwChunkGroupCreate();auto* b=RwChunkGroupCreate();Check(a&&b&&a!=b&&a->name[0]==0&&b->name[0]==0,"chunk groups allocate distinct empty names");
    const std::string name(31,'N'),too_long(32,'X');Check(RwChunkGroupSetName(a,name.c_str())==a&&std::strlen(a->name)==31,"public group name accepts31 bytes");
    Check(!RwChunkGroupSetName(a,too_long.c_str())&&name==a->name,"32-byte group name rejected without modifying prior value");
    Check(RwChunkGroupSetName(a,nullptr)==a&&a->name[0]==0,"null group name clears first byte");
    Check(RwChunkGroupDestroy(a),"group release");(void)b; // Retained entry is reclaimed with the group pool on Stop.
    session.Stop();session.Start();auto* c=RwChunkGroupCreate();Check(c&&c->name[0]==0&&RwChunkGroupDestroy(c),"group pool reusable after engine restart");
}
void GroupFailure(Session& session){
    // Warmed x87 state avoids the separately documented SDK cold rollback fault.
    session.Stop();RwChunkGroupSetFreeListCreateParams(1,1);fail_group=true;
    Check(!RwEngineStart()&&!fail_group,"chunk pool preallocation failure rolls back engine start");
    session.Start();auto* group=RwChunkGroupCreate();Check(group&&RwChunkGroupDestroy(group),"group pool and preceding engine modules retry after warm failure");
}
void GameCamera(){
    pal4::NativeCameraResourceRuntime runtime;const auto handle=pal4::CameraResourceHelpers::Create(16,runtime);Check(handle,"actual game camera create");
    auto* camera=reinterpret_cast<RwCamera*>(handle);auto* raster=RwRasterCreate(16,16,32,rwRASTERTYPECAMERATEXTURE|rwRASTERFORMAT8888);Check(raster,"render target");RwCameraSetRaster(camera,raster);
    Check(pal4::CameraResourceHelpers::SetClipAndWindow(handle,10,runtime)==handle,"game clip setup");
    const pal4::CameraRenderVector3 target{8,9,10};auto* unrelated=Frame();
    {Observer other(unrelated,2),observer(RwCameraGetFrame(camera),1);events.clear();Position(unrelated,42);
        Check(pal4::CameraResourceHelpers::UpdateTarget(handle,reinterpret_cast<std::uint32_t>(&target),runtime)==handle,"actual game target translation");
        Check(events.empty()&&RwFrameDirty(RwCameraGetFrame(camera)),"game translation schedules without immediate callback");
        Check(RwCameraBeginUpdate(camera)==camera,"camera BeginUpdate runs frame flush");
        Check(events.size()==2,"camera BeginUpdate flushes unrelated dirty hierarchy as well");
        At(RwCameraGetFrame(camera)->ltm,8,9);Near(RwCameraGetFrame(camera)->ltm.pos.z,2.5f,"game half-far-plane offset reaches camera LTM");Near(camera->frustumCorners[0].z,2.51f,"actual camera sync computes near frustum from game transform");
        Check(RwCameraEndUpdate(camera)==camera,"camera end");}
    RwFrameDestroy(unrelated);Check(pal4::CameraResourceHelpers::Destroy(handle,runtime)==1,"game camera teardown");Check(RwRasterDestroy(raster),"caller-owned color raster teardown");
}
}
int main(int argc,char** argv){try{Check(argc==2,"one case required");{Session session;const std::string_view c=argv[1];
    if(c=="matrix")MatrixOnly();else if(c=="dirty")Dirty();else if(c=="no_ltm")NoLtm();else if(c=="reschedule")Reschedule();else if(c=="group_lifetime")GroupLifecycle(session);else if(c=="group_failure")GroupFailure(session);else if(c=="game_camera")GameCamera();else throw std::runtime_error("unknown case");
    }Check(allocations.empty()&&!invalid_free,"all engine/fixture allocations released once");std::printf("frame sync %s passed\n",argv[1]);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"frame sync failed: %s\n",e.what());return 1;}}
