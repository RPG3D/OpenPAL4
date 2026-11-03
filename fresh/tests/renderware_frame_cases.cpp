#include "renderware_frame_cases.h"
#include <rwcore.h>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace pal4::test {
namespace {
static_assert(offsetof(RwFrame, modelling)==16 && offsetof(RwFrame, ltm)==80);
static_assert(offsetof(RwFrame, objectList)==144 && offsetof(RwFrame, child)==152);
static_assert(offsetof(RwFrame, next)==156 && offsetof(RwFrame, root)==160);
static_assert(std::is_same_v<decltype(&RwFrameRotate), RwFrame* (__cdecl *)(RwFrame*,const RwV3d*,RwReal,RwOpCombineType)>);
static_assert(std::is_same_v<decltype(&RwFrameGetRoot), RwFrame* (__cdecl *)(const RwFrame*)>);
static_assert(std::is_same_v<decltype(&RwFrameDestroy), RwBool (__cdecl *)(RwFrame*)>);
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void Near(float a,float b,const char* message) { Require(std::fabs(a-b)<0.0002f,message); }
int offset{}, constructed{}, destroyed{}, copied{};
bool callback_valid=true;
int& Data(void* frame) { return *reinterpret_cast<int*>(static_cast<char*>(frame)+offset); }
void* Construct(void* frame,RwInt32 at,RwInt32 size) {
    callback_valid &= at==offset && size==4; Data(frame)=42; ++constructed; return frame;
}
void* Destruct(void* frame,RwInt32 at,RwInt32 size) {
    callback_valid &= at==offset && size==4 && Data(frame)>=42; ++destroyed; return frame;
}
void* Copy(void* frame,const void* source,RwInt32 at,RwInt32 size) {
    callback_valid &= at==offset && size==4;
    Data(frame)=Data(const_cast<void*>(source)); ++copied; return frame;
}
RwFrame* New() { auto* f=RwFrameCreate(); Require(f!=nullptr,"frame allocation failed"); return f; }
void Position(RwFrame* f,float x,float y,float z) {
    const RwV3d v{x,y,z}; Require(RwFrameTranslate(f,&v,rwCOMBINEREPLACE)==f,"translate return");
}
void CheckPosition(RwFrame* f,float x,float y,float z) {
    auto* m=RwFrameGetLTM(f); Near(m->pos.x,x,"LTM x"); Near(m->pos.y,y,"LTM y"); Near(m->pos.z,z,"LTM z");
}
struct Visit { std::vector<RwFrame*> frames; bool stop{},remove{}; };
RwFrame* Visitor(RwFrame* f,void* context) {
    auto& visit=*static_cast<Visit*>(context); visit.frames.push_back(f);
    if (visit.remove) RwFrameRemoveChild(f);
    return visit.stop ? nullptr : f;
}
void Hierarchy() {
    auto* root=New(); auto* a=New(); auto* b=New(); auto* leaf=New(); auto* other=New();
    Require(!RwFrameDirty(root) && RwFrameGetRoot(root)==root && !RwFrameGetParent(root),"initial root state");
    Position(root,10,20,30); Position(a,1,2,3); Position(leaf,4,5,6);
    Require(RwFrameAddChild(a,leaf)==a && RwFrameAddChild(root,a)==root && RwFrameAddChild(root,b)==root,"attach result");
    Require(RwFrameCount(root)==4 && RwFrameGetRoot(leaf)==root,"recursive count/root");
    Require(RwFrameGetParent(leaf)==a && RwFrameGetParent(a)==root,"parent links");
    Visit visit; Require(RwFrameForAllChildren(root,Visitor,&visit)==root,"iteration return");
    Require(visit.frames==std::vector<RwFrame*>({b,a}),"children must prepend; iterator only visits direct children");
    visit={}; visit.stop=true; RwFrameForAllChildren(root,Visitor,&visit);
    Require(visit.frames==std::vector<RwFrame*>({b}),"null callback must stop");
    CheckPosition(leaf,15,27,39);
    Require((RwFrameDirty(root)&1)==0 && (RwFrameDirty(root)&2)!=0,"GetLTM only synchronizes matrix dirtiness");
    Position(other,-10,0,0); Require(RwFrameAddChild(other,a)==other,"reparent result");
    Require(RwFrameCount(root)==2 && RwFrameCount(other)==3 && RwFrameGetRoot(leaf)==other,"reparent roots/counts");
    CheckPosition(leaf,-5,7,9);
    Require(RwFrameRemoveChild(a)==a && !RwFrameGetParent(a) && RwFrameGetRoot(leaf)==a,"detach subtree root");
    CheckPosition(leaf,5,7,9);
    Require(RwFrameDestroyHierarchy(a) && RwFrameDestroyHierarchy(root) && RwFrameDestroy(other),"hierarchy cleanup");
    Require(RwFrameDestroyHierarchy(nullptr),"null hierarchy destroy");
}
void NoUpdate() {
    auto* root=New(); auto* child=New();
    Position(root,10,0,0); CheckPosition(root,10,0,0);
    // Keep fresh child clean. The no-update variant links it but does not
    // schedule the new hierarchy's matrix update; cached child LTM stays stale.
    Require(RwFrameAddChildNoUpdate(root,child)==root,"no-update return");
    Require(RwFrameGetRoot(child)==root && RwFrameGetParent(child)==root,"no-update links");
    CheckPosition(child,0,0,0);
    Require(RwFrameUpdateObjects(child)==child,"update return");
    CheckPosition(child,10,0,0);
    RwFrameDestroyHierarchy(root);
}
void Transforms() {
    auto* frame=New();
    const RwV3d scale{2,3,4}, delta{1,2,3}, axis{0,0,1};
    Position(frame,10,20,30);
    RwFrameScale(frame,&scale,rwCOMBINEPRECONCAT); CheckPosition(frame,10,20,30);
    auto* m=RwFrameGetMatrix(frame); Near(m->right.x,2,"pre scale right"); Near(m->up.y,3,"pre scale up");
    RwFrameTranslate(frame,&delta,rwCOMBINEPRECONCAT); CheckPosition(frame,12,26,42);
    Position(frame,10,20,30); RwFrameScale(frame,&scale,rwCOMBINEPOSTCONCAT); CheckPosition(frame,20,60,120);
    Require(RwFrameSetIdentity(frame)==frame,"identity return");
    Require(RwFrameRotate(frame,&axis,90,rwCOMBINEREPLACE)==frame,"rotate return");
    m=RwFrameGetMatrix(frame); Near(m->right.x,0,"rotation right x"); Near(m->right.y,1,"rotation right y");
    Near(m->up.x,-1,"rotation up x");
    RwMatrix transform{}; RwMatrixSetIdentity(&transform); transform.pos={5,6,7}; RwMatrixUpdate(&transform);
    Require(RwFrameTransform(frame,&transform,rwCOMBINEREPLACE)==frame,"matrix transform return");
    CheckPosition(frame,5,6,7);
    // Direct matrix writes need both matrix flag invalidation and frame update.
    RwFrameGetMatrix(frame)->pos={8,9,10}; RwMatrixUpdate(RwFrameGetMatrix(frame));
    RwFrameUpdateObjects(frame); CheckPosition(frame,8,9,10);
    RwFrameSetIdentity(frame); CheckPosition(frame,0,0,0); RwFrameDestroy(frame);
}
void Clone() {
    auto* root=New(); auto* a=New(); auto* b=New(); auto* leaf=New();
    Data(root)=100; Data(a)=101; Data(b)=102; Data(leaf)=103;
    Position(root,10,0,0); Position(a,1,2,3); Position(leaf,4,5,6);
    RwFrameAddChild(a,leaf); RwFrameAddChild(root,a); RwFrameAddChild(root,b);
    auto* camera=RwCameraCreate(); Require(camera!=nullptr,"clone camera allocation"); RwCameraSetFrame(camera,a);
    auto* clone=RwFrameCloneHierarchy(root); Require(clone && clone!=root,"clone allocation");
    Require(RwFrameCount(clone)==4 && copied==4,"clone plugins/count");
    Require(Data(clone)==100 && Data(clone->child)==101 && Data(clone->child->next)==102,"clone reverses sibling order and copies plugin data");
    Require(RwFrameGetRoot(leaf)==root && RwFrameGetRoot(a)==root && RwFrameGetRoot(b)==root,"original roots restored after temporary clone mapping");
    auto* clone_leaf=clone->child->child;
    Require(clone_leaf && Data(clone_leaf)==103 && RwFrameGetRoot(clone_leaf)==clone,"clone subtree root/data");
    CheckPosition(clone_leaf,15,7,9);
    Require(rwLinkListEmpty(&clone->child->objectList) && RwCameraGetFrame(camera)==a,"clone must not duplicate attached camera");
    // Cloning a subtree attached to another root must restore its original root.
    auto* subtree=RwFrameCloneHierarchy(a); Require(subtree && RwFrameCount(subtree)==2,"attached subtree clone");
    Require(RwFrameGetRoot(leaf)==root && RwFrameGetRoot(a)==root && !RwFrameGetParent(subtree),"attached source root restoration");
    CheckPosition(subtree->child,5,7,9);
    Position(clone,100,0,0); CheckPosition(leaf,15,7,9); CheckPosition(clone_leaf,105,7,9);
    RwCameraSetFrame(camera,nullptr); RwCameraDestroy(camera);
    RwFrameDestroyHierarchy(subtree); RwFrameDestroyHierarchy(clone); RwFrameDestroyHierarchy(root);
}
void Destruction() {
    auto* root=New(); auto* a=New(); auto* b=New();
    RwFrameAddChild(root,a); RwFrameAddChild(root,b);
    Visit visit; visit.remove=true; RwFrameForAllChildren(root,Visitor,&visit);
    Require(visit.frames==std::vector<RwFrame*>({b,a}) && RwFrameCount(root)==1,"iterator caches next before removal");
    RwFrameAddChild(root,a); RwFrameAddChild(root,b);
    const int before=destroyed;
    Require(RwFrameDestroy(b) && destroyed==before+1 && RwFrameCount(root)==2,"single destroy removes leaf only");
    RwFrameRemoveChild(a); RwFrameDestroy(a); RwFrameDestroy(root);
    // Nonrecursive parent destruction leaves children allocated and clears only
    // parent links. Its cached root remains the old address: inspect, never use.
    auto* parent=New(); auto* child=New(); auto* safe_parent=New();
    RwFrameAddChild(parent,child); auto* old_root=RwFrameGetRoot(child);
    const int before_parent=destroyed; RwFrameDestroy(parent);
    Require(destroyed==before_parent+1 && !RwFrameGetParent(child) && child->root==old_root,"nonrecursive destroy contract");
    // Reattach before querying LTM/dirty state; those dereference the root.
    RwFrameAddChild(safe_parent,child); RwFrameDestroyHierarchy(safe_parent);
}
void Failure(bool& fail_next) {
    fail_next=true; Require(!RwFrameCreate() && !fail_next,"Create allocation failure");
    auto* root=New(); auto* child=New(); RwFrameAddChild(root,child);
    fail_next=true; Require(!RwFrameCloneHierarchy(root) && !fail_next,"Clone root allocation failure");
    Require(RwFrameGetRoot(child)==root && RwFrameCount(root)==2,"failed clone changed source hierarchy");
    auto* retry=RwFrameCloneHierarchy(root); Require(retry && RwFrameCount(retry)==2,"clone retry failed");
    RwFrameDestroyHierarchy(retry); RwFrameDestroyHierarchy(root);
}
}
void RegisterFramePlugin() {
    offset=RwFrameRegisterPlugin(4,0x123404,Construct,Destruct,Copy);
    Require(offset>=164 && RwFrameGetPluginOffset(0x123404)==offset,"frame plugin registration/layout");
}
void RunFrameCase(std::string_view name,bool& fail_next_allocation) {
    if(name=="frame_hierarchy") Hierarchy();
    else if(name=="frame_no_update") NoUpdate();
    else if(name=="frame_transforms") Transforms();
    else if(name=="frame_clone") Clone();
    else if(name=="frame_destruction") Destruction();
    else if(name=="frame_failure") Failure(fail_next_allocation);
    else Require(false,"unknown frame case");
    Require(callback_valid && constructed==destroyed,"frame plugin lifetime mismatch");
}
}
