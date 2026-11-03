#include "renderware_slist_cases.h"
#include "pal4/rw_dict_schema_list_native.h"
#include <rwcore.h>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <type_traits>
static_assert(std::is_same_v<decltype(&_rwSListEmpty),void (__cdecl *)(RwSList*)>);
static_assert(std::is_same_v<decltype(&_rwSListDestroyEndEntries),void (__cdecl *)(RwSList*,RwInt32)>);
namespace pal4::test {
namespace {
void Require(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
void Growth(bool& fail_next){
    auto* list=_rwSListCreate(4,0);Require(list&&list->numElementsAlloced==20&&list->entrySize==4,"initial list capacity");
    Require(_rwSListGetBegin(list)==_rwSListGetEnd(list)&&_rwSListGetBegin(list)&&!_rwSListGetArray(list),"empty list begin is allocated but array query is null");
    for(int i=0;i<20;++i)*static_cast<int*>(_rwSListGetNewEntry(list,0))=i*7;
    auto* old=list->listElements;fail_next=true;Require(!_rwSListGetNewEntry(list,0)&&!fail_next&&list->listElements==old&&list->numElementsFilled==20&&list->numElementsAlloced==20,"failed growth preserves list state");
    *static_cast<int*>(_rwSListGetNewEntry(list,0))=140;Require(list->numElementsAlloced==25&&list->numElementsFilled==21,"single growth adds quarter capacity");
    for(int i=0;i<21;++i)Require(*static_cast<int*>(_rwSListGetEntry(list,i))==i*7,"growth preserves entries");
    auto* more=static_cast<int*>(_rwSListGetNewEntries(list,4,0));Require(more&&list->numElementsFilled==25&&list->numElementsAlloced==35,"batch equal capacity triggers growth plus requested amount");
    for(int i=0;i<4;++i)more[i]=200+i;
    _rwSListDestroyEndEntries(list,4);Require(_rwSListGetNumEntries(list)==21,"destroy end removes count only");
    _rwSListEmpty(list);Require(_rwSListGetNumEntries(list)==0&&list->numElementsAlloced==35&&!_rwSListGetArray(list),"empty preserves capacity");Require(_rwSListDestroy(list),"destroy list");
}
void Transfer(){
    auto* list=_rwSListCreate(4,0);Require(list,"transfer list");*static_cast<int*>(_rwSListGetNewEntry(list,0))=0x12345678;auto* before=_rwSListGetBegin(list);
    auto* array=static_cast<RwUInt8*>(_rwSListToArray(list));Require(array==before&&*reinterpret_cast<int*>(array)==0x12345678,"ToArray transfers backing allocation and frees wrapper");Require(_rwSListDestroyArray(array),"transferred array cleanup");
    auto* empty=_rwSListCreate(4,0);Require(empty&&!_rwSListToArray(empty),"empty ToArray frees backing and wrapper");
}
class GameServices final : public NativeRwDictSchemaListRuntime {
public:
    std::vector<std::uint32_t> updated,applied;
    void DestroyDict(std::uint32_t) override {throw std::runtime_error("unreviewed dictionary fixture path");}
    std::uint32_t SetCurrentDict(std::uint32_t dict) override {Require(!dict,"cleanup clears current dict");return 77;}
    std::uint32_t OpenStream(std::uint32_t,std::uint32_t,const char*) override {throw std::runtime_error("unused stream fixture");}
    bool FindChunk(std::uint32_t,std::uint32_t) override {throw std::runtime_error("unused stream fixture");}
    std::uint32_t ReadDict(std::uint32_t) override {throw std::runtime_error("unused dictionary fixture");}
    void CloseStream(std::uint32_t) override {throw std::runtime_error("unused stream fixture");}
    using Callback=std::uint32_t (*)(std::uint32_t,std::uint32_t,IRwDictSchemaRuntime&);
    std::uint32_t ForAllAtomics(std::uint32_t,Callback,std::uint32_t) override {throw std::runtime_error("unused atomic fixture");}
    std::uint32_t ForAllGeometryMaterials(std::uint32_t,Callback,std::uint32_t) override {throw std::runtime_error("unused geometry fixture");}
    std::uint32_t ForAllWorldMaterials(std::uint32_t world,Callback callback,std::uint32_t list) override {
        for(std::uint32_t i=1;i<=50;++i){Require(callback(i,list,*this)==i,"game material callback");callback(i,list,*this);}return world;
    }
    bool HasUVAnim(std::uint32_t material) override {return material%2!=0;}
    void AdvanceUVAnim(std::uint32_t material,float elapsed) override {Require(elapsed==3.0f,"game scale before UV advance");updated.push_back(material);}
    void ApplyUVAnim(std::uint32_t material) override {Require(updated.back()==material,"advance before apply");applied.push_back(material);}
};
void Game(){
    GameServices runtime;RwDictSchema schema;Require(schema.CreateWorldList(123,runtime)==123&&schema.list,"game world creates native list");
    auto* list=reinterpret_cast<RwSList*>(schema.list);Require(_rwSListGetNumEntries(list)==25,"game rejects duplicates and unanimated materials across reallocation");
    schema.time_scale=1.5f;Require(schema.Update(2.0f,runtime)==runtime.ListEnd(schema.list),"game returns list end");Require(runtime.updated.size()==25&&runtime.updated==runtime.applied&&runtime.updated.front()==1&&runtime.updated.back()==49,"game visits stable native list order");
    Require(schema.CreateWorldList(456,runtime)==456&&_rwSListGetNumEntries(reinterpret_cast<RwSList*>(schema.list))==25,"game rebuild destroys old native list");
    Require(schema.Cleanup(runtime)==77&&!schema.list,"game cleanup uses list destruction");
}
}
void RunSListCase(std::string_view name,bool& fail_next){if(name=="slist_growth")Growth(fail_next);else if(name=="slist_transfer")Transfer();else if(name=="slist_game")Game();else throw std::runtime_error("unknown SList case");}
}
