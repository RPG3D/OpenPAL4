#include "pal4/portable/monster_movement_world.h"
#include "pal4/behavior_path_endpoint_movement.h"
#include "pal4/grid_manager.h"
#include "pal4/object_random_movement_lifecycle.h"
#include "pal4/grid_manager_members.h"
#include "pal4/grid_node_object.h"
#include "pal4/spatial_grid_wrapper.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <map>
#include <span>
#include <variant>
#include <stdexcept>
#include <type_traits>
#include <exception>

namespace pal4::portable {
namespace {
void Require(bool valid,const char* message){if(!valid)throw std::invalid_argument(message);}
void Finite(const std::array<float,3>& p){for(float f:p)Require(std::isfinite(f),"non-finite monster world position");}
template<class T>std::uint32_t ReadPod(const T& value,std::size_t word){
    const auto words=std::bit_cast<std::array<std::uint32_t,sizeof(T)/4>>(value);return words.at(word);
}
template<class T>void WritePod(T& value,std::size_t word,std::uint32_t bits){
    auto words=std::bit_cast<std::array<std::uint32_t,sizeof(T)/4>>(value);words.at(word)=bits;value=std::bit_cast<T>(words);
}
template<class To,class From>void Prefix(To& to,const From& from,std::size_t bytes){
    const auto source=std::as_bytes(std::span{&from,1u});auto target=std::as_writable_bytes(std::span{&to,1u});
    Require(bytes<=source.size() && bytes<=target.size(),"monster world image prefix extent");std::copy_n(source.begin(),bytes,target.begin());
}
struct Guard {
    bool& busy;
    explicit Guard(bool& value):busy(value){if(busy)throw std::logic_error("monster movement world is not reentrant");busy=true;}
    ~Guard(){busy=false;}
};
}
struct MonsterMovementWorld::Impl final:public ISpatialGridNodeRuntime,public IGridManagerMembersRuntime,
    public IBehaviorPathAssociationRuntime,public IGridManagerRuntime {
    struct HelperEntry {BehaviorPathEndpointState state{};SceneMonsterHandle owner;bool member{},random{};};
    struct PathHeader {ScenePathHandle path;std::uint32_t points;};
    struct PathPositions {ScenePathHandle path;};
    struct HelperView {HelperEntry* helper;};
    using Storage=std::variant<std::vector<std::uint32_t>,SpatialGridState,SpatialGridWrapperState,GridNodeObjectState,PathHeader,PathPositions,HelperView>;
    struct Block {std::uint32_t bytes;Storage storage;};
    std::uint64_t generation;
    std::shared_ptr<ScenePathways> paths;
    Random random;MonsterMovementLimits limits;
    std::map<std::uint32_t,Block> blocks;
    std::map<std::uint32_t,std::unique_ptr<HelperEntry>> helpers;
    std::map<const BehaviorPathEndpointState*,std::uint32_t> images;
    std::vector<std::uint32_t> path_tokens;
    GridManagerMembersState members{};
    std::uint32_t next=0x1000,compensating{};
    std::size_t mapped{};bool closed{},busy{};
    const FrameUpdate* frame_update{};
    void MembershipIdle() const{Require(!frame_update,"monster membership mutation during grid frame");}
    Impl(std::uint64_t g,std::shared_ptr<ScenePathways> p,Random r,MonsterMovementLimits l)
        :generation(g),paths(std::move(p)),random(std::move(r)),limits(l){
        Require(generation && paths && bool(random) && limits.max_helpers && limits.max_helpers<=65535 && limits.mapped_bytes,"invalid monster movement world parameters");
        Require(paths->Generation()==generation,"path/world generation mismatch");
        (void)GridManagerMembers::Initialize(members);
        const auto grid=GridManagerMembers::InitializeGrid(members,*this);Require(grid!=0,"monster spatial grid initialization failed");
        path_tokens.resize(paths->Size());
        for(std::size_t i=0;i<paths->Size();++i){
            const auto h=paths->Handle(i);Require(h.generation==generation,"path/world generation mismatch");
            if(!paths->HasGrid(h))continue;
            const auto positions=Reserve(paths->PointCount(h)*12u,PathPositions{h});
            path_tokens[i]=Reserve(88u,PathHeader{h,positions});
        }
    }
    ~Impl(){try{Shutdown();}catch(...){std::terminate();}}
    void Open() const{Require(!closed,"monster movement world is closed");}
    std::uint32_t Reserve(std::uint32_t bytes,Storage value){
        Require(bytes && bytes%4==0,"invalid monster world allocation extent");
        if(mapped>limits.mapped_bytes || bytes>limits.mapped_bytes-mapped)throw std::length_error("monster movement world mapped-byte limit");
        if(next>std::numeric_limits<std::uint32_t>::max()-16u || bytes>std::numeric_limits<std::uint32_t>::max()-next-16u)
            throw std::length_error("monster world numeric token exhausted");
        const auto token=next;blocks.emplace(token,Block{bytes,std::move(value)});next+=bytes+16u;mapped+=bytes;return token;
    }
    std::pair<Block&,std::size_t> Locate(std::uint32_t address){
        auto found=blocks.upper_bound(address);Require(found!=blocks.begin(),"unknown monster world numeric address");--found;
        const auto offset=address-found->first;Require(offset%4==0 && offset<found->second.bytes,"monster world numeric address out of bounds");return {found->second,offset/4};
    }
    template<class T>T& Promote(std::uint32_t address){
        auto found=blocks.find(address);Require(found!=blocks.end() && found->second.bytes==sizeof(T),"invalid typed monster world image");
        auto& storage=found->second.storage;
        if(auto* raw=std::get_if<std::vector<std::uint32_t>>(&storage)){
            std::array<std::uint32_t,sizeof(T)/4> words{};std::copy(raw->begin(),raw->end(),words.begin());storage=std::bit_cast<T>(words);
        }
        auto* typed=std::get_if<T>(&storage);Require(typed!=nullptr,"monster world image type mismatch");return *typed;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Require(bytes && bytes%4==0 && bytes<=16u*1024*1024,"invalid monster grid allocation");
        return Reserve(bytes,std::vector<std::uint32_t>(bytes/4));
    }
    void Erase(std::uint32_t token){
        const auto found=blocks.find(token);Require(found!=blocks.end(),"unknown monster world allocation release");mapped-=found->second.bytes;blocks.erase(found);
    }
    std::uint32_t Free(std::uint32_t token) override {
        if(!token)return 0;
        const auto found=blocks.find(token);Require(found!=blocks.end(),"unknown monster grid release");
        Require(!std::holds_alternative<HelperView>(found->second.storage) && !std::holds_alternative<PathHeader>(found->second.storage) && !std::holds_alternative<PathPositions>(found->second.storage),"monster grid cannot release caller-owned view");
        Erase(token);return 1;
    }
    SpatialGridState& GridAt(std::uint32_t address) override{return Promote<SpatialGridState>(address);}
    SpatialGridNodeState& NodeAt(std::uint32_t address) override {
        Require(address>=4u,"invalid monster node token");return Promote<GridNodeObjectState>(address-4u).node;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        auto [block,word]=Locate(address);
        return std::visit([&](auto& image)->std::uint32_t{
            using T=std::decay_t<decltype(image)>;
            if constexpr(std::is_same_v<T,std::vector<std::uint32_t>>)return image.at(word);
            else if constexpr(std::is_same_v<T,HelperView>)return image.helper->state.words.at(word);
            else if constexpr(std::is_same_v<T,PathPositions>)return std::bit_cast<std::uint32_t>(paths->Point(image.path,word/3)[word%3]);
            else if constexpr(std::is_same_v<T,PathHeader>){
                const auto& source=paths->Source(image.path);
                switch(word){
                case 0:return 0x84b5e4u;case 1:return 0x84b5f8u;
                case 2:return paths->PointCount(image.path);case 3:return image.points;
                case 4:return source.radius_bits;case 5:return source.loop_byte;
                case 19:return std::bit_cast<std::uint32_t>(paths->Length(image.path));case 20:return 0;case 21:return 0x84b5d4u;
                default:throw std::logic_error("path query scratch requires typed ScenePathways API");
                }
            }else return ReadPod(image,word);
        },block.storage);
    }
    void WriteWord(std::uint32_t address,std::uint32_t value) override {
        auto [block,word]=Locate(address);
        std::visit([&](auto& image){
            using T=std::decay_t<decltype(image)>;
            if constexpr(std::is_same_v<T,std::vector<std::uint32_t>>)image.at(word)=value;
            else if constexpr(std::is_same_v<T,HelperView>)image.helper->state.words.at(word)=value;
            else if constexpr(std::is_same_v<T,PathHeader> || std::is_same_v<T,PathPositions>)throw std::logic_error("original monster path views are read-only");
            else WritePod(image,word,value);
        },block.storage);
    }
    void DestroyGridWrapper(std::uint32_t address) override {
        auto& wrapper=Promote<SpatialGridWrapperState>(address);(void)SpatialGridWrapper::Destroy(address,wrapper,true,*this);
    }
    void WriteGridWrapper(std::uint32_t wrapper,std::uint32_t grid) override {Promote<SpatialGridWrapperState>(wrapper).grid=grid;}
    HelperEntry& Get(MonsterHelperHandle h) const {
        Open();Require(h.generation==generation,"stale monster helper generation");const auto found=helpers.find(h.token);
        Require(found!=helpers.end(),"unknown monster helper handle");return *found->second;
    }
    std::uint32_t Token(const BehaviorPathEndpointState& state) const {
        const auto found=images.find(&state);Require(found!=images.end(),"helper image belongs to another movement world");return found->second;
    }
    GridNodeObjectState& Association(std::uint32_t token){
        const auto found=blocks.find(token);Require(found!=blocks.end() && std::holds_alternative<GridNodeObjectState>(found->second.storage),"unknown monster spatial association");
        auto& image=std::get<GridNodeObjectState>(found->second.storage);Require(image.vtable==0x84b6c0u,"invalid monster association vtable");
        const auto owner=helpers.find(image.node.payload);Require(owner!=helpers.end() && owner->second->state.words[42]==token,"monster spatial association owner mismatch");return image;
    }
    void DestroyAssociation(std::uint32_t token){
        auto& association=Association(token);(void)GridNodeObject::Destroy(token,association,true,*this);
    }
    void ReleaseAssociation(std::uint32_t token,std::uint32_t flags) override {Require(flags==1,"invalid monster association release flags");DestroyAssociation(token);}
    std::uint32_t BindAssociation(std::uint32_t wrapper,BehaviorPathEndpointState& helper) override {
        Require(wrapper==members.grid_wrapper,"invalid monster grid binder");
        return SpatialGridWrapper::CreateNode(Promote<SpatialGridWrapperState>(wrapper),Token(helper),*this);
    }
    void Bind(BehaviorPathEndpointState& helper,std::uint32_t wrapper){
        MembershipIdle();
        Open();(void)Token(helper);Require(wrapper==members.grid_wrapper,"foreign monster grid binder");
        if(helper.words[42]){const auto& old=Association(helper.words[42]);Require(old.node.payload==Token(helper),"cannot bind another helper association");}
        (void)BehaviorPathEndpointCallbacks::SetAssociation(helper,wrapper,*this);
    }
    std::uint32_t Position(std::uint32_t association,const BehaviorVector3& position){
        Open();Finite(position);auto& image=Association(association);return GridNodeObject::UpdatePosition(association,image,position[0],position[1],position[2],*this);
    }
    void ReleaseElement(std::uint32_t token) override {
        auto& helper=Get({generation,token});helper.member=false;
        if(compensating!=token)DeleteImage(token);
    }
    void UpdateElement(std::uint32_t token,GridManagerMembersState& state,std::uint32_t argument) override {
        Require(frame_update && &state==&members,"monster movement frame service is not bound");
        const MonsterHelperHandle h{generation,token};Require(Get(h).member,"unlisted helper in original grid traversal");
        (*frame_update)(h,std::bit_cast<float>(argument));
    }
    // Existing GridManager cleanup only uses DestroyNested and Free. Other slots
    // cannot silently stand in for a missing constructor or frame runtime.
    std::array<std::uint32_t,3> TransformBasis(GridManagerState&,const std::array<std::uint32_t,3>&) override {throw std::logic_error("world cleanup is not basis construction");}
    void SetParameter(GridManagerState&,std::uint32_t,std::uint32_t) override {throw std::logic_error("world cleanup is not scalar setup");}
    std::array<std::uint32_t,3> ZeroVector() override {throw std::logic_error("world cleanup does not request zero vectors");}
    std::uint32_t NextIdentifier() override {throw std::logic_error("world requires caller-owned identifier stream");}
    void DestroyNested(std::uint32_t association,bool free_self) override {Require(free_self,"monster helper cleanup requires deleting association");DestroyAssociation(association);}
    void FreeSelf(GridManagerState&) override {throw std::logic_error("copied monster grid image cannot free itself");}
    void DeleteImage(std::uint32_t token){
        auto& entry=Get({generation,token});Require(!entry.member,"cannot delete listed monster helper directly");
        struct Lifecycle final:IBehaviorPathEndpointLifecycleRuntime,IObjectRandomMovementRuntime {
            Impl& world;
            explicit Lifecycle(Impl& w):world(w){}
            void ConstructBase(BehaviorPathEndpointState&) override {throw std::logic_error("world release cannot construct helper");}
            void DestructBase(BehaviorPathEndpointState& state) override {
                GridManagerState grid{};Prefix(grid,state,sizeof(grid));
                try{GridManager::Cleanup(grid,world);}catch(...){Prefix(state,grid,sizeof(grid));throw;}
                Prefix(state,grid,sizeof(grid));
            }
            void ConstructBase(ObjectRandomMovementState&) override {throw std::logic_error("world release cannot construct random helper");}
            void CleanupBase(ObjectRandomMovementState& value) override {
                auto image=std::bit_cast<BehaviorPathEndpointState>(value);
                try{DestructBase(image);}catch(...){value=std::bit_cast<ObjectRandomMovementState>(image);throw;}
                value=std::bit_cast<ObjectRandomMovementState>(image);
            }
            BehaviorVector3 RandomVector() override {throw std::logic_error("world release cannot consume random stream");}
            void Free(std::uint32_t) override {throw std::logic_error("world owns outer helper storage");}
        } lifecycle(*this);
        if(entry.random){
            auto image=std::bit_cast<ObjectRandomMovementState>(entry.state);
            try{ObjectRandomMovementLifecycle::Cleanup(image,lifecycle);}catch(...){entry.state=std::bit_cast<BehaviorPathEndpointState>(image);throw;}
            entry.state=std::bit_cast<BehaviorPathEndpointState>(image);
        }else BehaviorPathEndpointHelper::Destruct(entry.state,lifecycle);
        images.erase(&entry.state);helpers.erase(token);Erase(token);
    }
    MonsterHelperHandle Create(SceneMonsterHandle owner,std::uint32_t identifier,bool is_random=false){
        MembershipIdle();
        Open();Require(owner.generation==generation,"monster/helper generation mismatch");
        if(helpers.size()>=limits.max_helpers)throw std::length_error("monster movement helper limit");
        auto entry=std::make_unique<HelperEntry>();entry->owner=owner;entry->random=is_random;
        if(is_random){
            struct Draw final:IGridRandomRuntime {
                Impl& world;explicit Draw(Impl& w):world(w){}
                std::int32_t Rand() override {
                    const auto value=world.random();Require(value<=32767,"monster world random value exceeds original CRT range");
                    return static_cast<std::int32_t>(value);
                }
            } draw(*this);
            BehaviorHelperConfiguration::ConstructRandom(entry->state,identifier,draw);
        }else BehaviorHelperConfiguration::ConstructEndpoint(entry->state,identifier);
        const auto* image=&entry->state;const auto token=Reserve(sizeof(BehaviorPathEndpointState),HelperView{entry.get()});
        try{helpers.emplace(token,std::move(entry));images.emplace(image,token);}
        catch(...){images.erase(image);helpers.erase(token);Erase(token);throw;}
        return {generation,token};
    }
    void Add(BehaviorPathEndpointState& helper){
        MembershipIdle();
        Open();const auto token=Token(helper);auto& entry=Get({generation,token});Require(!entry.member && helper.words[42],"monster helper is already listed or not configured");
        Require(Association(helper.words[42]).node.payload==token,"monster helper association mismatch");
        if(!GridManagerMembers::AddElement(members,token,*this))throw std::runtime_error("monster member insertion failed");
        entry.member=true;
    }
    void RemoveCompensation(BehaviorPathEndpointState& helper){
        MembershipIdle();
        Open();const auto token=Token(helper);auto& entry=Get({generation,token});if(!entry.member)return;
        Require(!compensating,"nested monster member compensation");compensating=token;
        try{Require(GridManagerMembers::RemoveElement(members,token,*this),"listed monster helper missing from member array");compensating=0;}
        catch(...){compensating=0;throw;}
    }
    void Release(BehaviorPathEndpointState& helper){
        MembershipIdle();
        if(!helper.words[42])return;
        Open();const auto token=Token(helper);Require(Association(helper.words[42]).node.payload==token,"cannot release another monster helper association");
        DestroyAssociation(helper.words[42]);helper.words[42]=0;
    }
    void Destroy(MonsterHelperHandle h){
        MembershipIdle();
        auto& entry=Get(h);
        if(entry.member)Require(GridManagerMembers::RemoveElement(members,h.token,*this),"monster helper missing from member collection");
        else DeleteImage(h.token);
    }
    std::uint32_t Path(std::string_view name) const {
        Open();const auto found=paths->Find(name);return found?path_tokens.at(found->index):0;
    }
    ScenePathHandle PathFrom(std::uint32_t token) const {
        Open();const auto found=blocks.find(token);Require(found!=blocks.end() && std::holds_alternative<PathHeader>(found->second.storage),"invalid monster pathway token");return std::get<PathHeader>(found->second.storage).path;
    }
    std::vector<MonsterNeighbor> Query(const std::array<float,3>& center,float radius){
        Open();Finite(center);Require(std::isfinite(radius) && radius>=0 && std::isfinite(radius*radius),"invalid monster neighbor radius");
        std::vector<MonsterNeighbor> result;const auto grid=Promote<SpatialGridWrapperState>(members.grid_wrapper).grid;
        (void)SpatialGrid::ProcessSphere(grid,center[0],center[1],center[2],radius,[&](std::uint32_t payload,std::uint32_t distance,std::uint32_t){
            const auto& entry=Get({generation,payload});const auto& node=Association(entry.state.words[42]).node;
            const auto position=std::bit_cast<std::array<float,3>>(node.position_bits);
            result.push_back({{generation,payload},entry.owner,position,std::bit_cast<float>(distance),entry.member});return 0;
        },0,*this);
        return result;
    }
    MonsterMovementWorldStats Statistics(){
        if(closed)return {0,0,0,0,0,0,true};
        std::size_t associations{};for(const auto& [token,block]:blocks){(void)token;if(std::holds_alternative<GridNodeObjectState>(block.storage))++associations;}
        const auto count=SpatialGridWrapper::CountNodes(Promote<SpatialGridWrapperState>(members.grid_wrapper),*this);
        return {helpers.size(),(members.elements_end-members.elements_begin)/4,associations,count,blocks.size(),mapped,false};
    }
    void Shutdown(){
        MembershipIdle();
        if(closed)return;
        (void)GridManagerMembers::ClearElements(members,*this);
        while(!helpers.empty())DeleteImage(helpers.begin()->first);
        (void)GridManagerMembers::Cleanup(members,*this);
        for(const auto& [token,block]:blocks){(void)token;Require(std::holds_alternative<PathHeader>(block.storage) || std::holds_alternative<PathPositions>(block.storage),"monster world leaked a runtime block");}
        blocks.clear();mapped=0;path_tokens.clear();paths.reset();closed=true;
    }
};
MonsterMovementWorld::MonsterMovementWorld(std::uint64_t generation,std::shared_ptr<ScenePathways> paths,Random random,MonsterMovementLimits limits)
    :impl_(std::make_unique<Impl>(generation,std::move(paths),std::move(random),limits)){}
MonsterMovementWorld::~MonsterMovementWorld()=default;
MonsterHelperHandle MonsterMovementWorld::CreateEndpoint(SceneMonsterHandle owner,std::uint32_t identifier){Guard guard(impl_->busy);return impl_->Create(owner,identifier);}
MonsterHelperHandle MonsterMovementWorld::CreateRandom(SceneMonsterHandle owner,std::uint32_t identifier){Guard guard(impl_->busy);return impl_->Create(owner,identifier,true);}
BehaviorPathEndpointState& MonsterMovementWorld::Helper(MonsterHelperHandle h){Require(!impl_->busy,"helper access during monster world callback");return impl_->Get(h).state;}
SceneMonsterHandle MonsterMovementWorld::Owner(MonsterHelperHandle h) const{Require(!impl_->busy,"helper owner access during monster world callback");return impl_->Get(h).owner;}
bool MonsterMovementWorld::Registered(MonsterHelperHandle h) const{Require(!impl_->busy,"helper membership access during monster world callback");return impl_->Get(h).member;}
bool MonsterMovementWorld::IsRandom(MonsterHelperHandle h) const{Require(!impl_->busy,"helper kind access during monster world callback");return impl_->Get(h).random;}
bool MonsterMovementWorld::Contains(MonsterHelperHandle h) const{Require(!impl_->busy,"helper lookup during monster world callback");return !impl_->closed && h.generation==impl_->generation && impl_->helpers.contains(h.token);}
bool MonsterMovementWorld::InGridFrame() const noexcept{return impl_->frame_update!=nullptr;}
void MonsterMovementWorld::UpdateMembers(float seconds,const FrameUpdate& update){
    Require(!impl_->busy && !impl_->frame_update,"monster grid frame is not reentrant");impl_->Open();
    Require(std::isfinite(seconds) && seconds>=0 && bool(update),"invalid monster grid frame service");
    impl_->frame_update=&update;struct Clear{Impl& impl;~Clear(){impl.frame_update=nullptr;}} clear{*impl_};
    (void)GridManagerMembers::UpdateElements(impl_->members,std::bit_cast<std::uint32_t>(seconds),*impl_);
}
void MonsterMovementWorld::DestroyHelper(MonsterHelperHandle h){Guard guard(impl_->busy);impl_->Destroy(h);}
std::vector<MonsterNeighbor> MonsterMovementWorld::QuerySphere(const std::array<float,3>& center,float radius){Guard guard(impl_->busy);return impl_->Query(center,radius);}
MonsterMovementWorldStats MonsterMovementWorld::Stats() const{return impl_->Statistics();}
void MonsterMovementWorld::Close(){Guard guard(impl_->busy);impl_->Shutdown();}
ScenePathHandle MonsterMovementWorld::PathHandle(std::uint32_t token) const{return impl_->PathFrom(token);}
float MonsterMovementWorld::PathClosest(std::uint32_t token,const std::array<float,3>& point){
    Guard guard(impl_->busy);const auto path=impl_->PathFrom(token);return impl_->paths->ClosestPosition(path,point);
}
std::array<float,3> MonsterMovementWorld::PathCoordinates(std::uint32_t token,float distance){
    Guard guard(impl_->busy);const auto path=impl_->PathFrom(token);return impl_->paths->CoordinatesAt(path,distance);
}
ScenePathProjection MonsterMovementWorld::PathProject(std::uint32_t token,const std::array<float,3>& point,const std::array<float,3>& initial_direction){
    Guard guard(impl_->busy);const auto path=impl_->PathFrom(token);return impl_->paths->ProjectToTube(path,point,initial_direction);
}
std::uint32_t MonsterMovementWorld::ReadWord(std::uint32_t address){impl_->Open();return impl_->ReadWord(address);}
void MonsterMovementWorld::Bind(BehaviorPathEndpointState& helper,std::uint32_t wrapper){Guard guard(impl_->busy);impl_->Bind(helper,wrapper);}
std::uint32_t MonsterMovementWorld::UpdatePosition(std::uint32_t association,const BehaviorVector3& p){Guard guard(impl_->busy);return impl_->Position(association,p);}
std::uint32_t MonsterMovementWorld::Rand(){
    Guard guard(impl_->busy);impl_->Open();const auto value=impl_->random();Require(value<=32767,"monster world random value exceeds original CRT range");return value;
}
std::uint32_t MonsterMovementWorld::FindPath(std::string_view name){return impl_->Path(name);}
std::uint32_t MonsterMovementWorld::GridParameter(){impl_->Open();return impl_->members.grid_wrapper;}
void MonsterMovementWorld::AddElement(BehaviorPathEndpointState& helper){Guard guard(impl_->busy);impl_->Add(helper);}
void MonsterMovementWorld::RemoveElement(BehaviorPathEndpointState& helper) noexcept {try{Guard guard(impl_->busy);impl_->RemoveCompensation(helper);}catch(...){std::terminate();}}
void MonsterMovementWorld::ReleaseAssociation(BehaviorPathEndpointState& helper) noexcept {try{Guard guard(impl_->busy);impl_->Release(helper);}catch(...){std::terminate();}}
}
