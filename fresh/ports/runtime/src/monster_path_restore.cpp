#include "pal4/portable/monster_random_state.h"
#include "pal4/monster_random_point_restore.h"
#include "pal4/monster_behavior_manager.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool valid,const char* message){if(!valid)throw std::invalid_argument(message);}
class Runtime final:public IMonsterPathEndpointRestoreRuntime,public IMonsterRandomPointRuntime {
    BehaviorPathEndpointState& helper_;const MonsterPathRestoreInput& input_;ActorPlayback& playback_;IMonsterPathRestoreWorld& world_;
    BehaviorHelperConfiguration configuration_;std::int32_t kind_;
    std::vector<const BinaryXmlNode*> nodes_;
    static constexpr std::uint32_t helper_id=0x10000,grid_id=0x20000,memory_id=0x30000,actor_id=0x40000,manager_id=0x50000;
    const BinaryXmlNode& Node(std::uint32_t node) const {Require(node && node<=nodes_.size(),"invalid restore XML handle");return *nodes_[node-1];}
    static float Float(const BinaryXmlNode& node,std::string_view key,float fallback){const auto* a=node.FindAttribute(key);const auto value=a?a->ToFloat(fallback):fallback;Require(std::isfinite(value),"non-finite monster restore value");return value;}
    static std::int32_t Int(const BinaryXmlNode& node,std::string_view key,std::int32_t fallback){const auto* a=node.FindAttribute(key);return a?std::bit_cast<std::int32_t>(a->ToInt(std::bit_cast<std::uint32_t>(fallback))):fallback;}
    static void Helper(std::uint32_t h){Require(h==helper_id,"invalid monster helper handle");}
    static void Animation(std::uint32_t h){Require(h==manager_id+0x170,"invalid monster animation handle");}
public:
    bool registered{};
    Runtime(std::int32_t kind,BehaviorPathEndpointState& helper,const MonsterPathRestoreInput& input,ActorPlayback& playback,IMonsterPathRestoreWorld& world)
        :helper_(helper),input_(input),playback_(playback),world_(world),configuration_(world),kind_(kind){
        if(!input.behavior.Root().children.empty())nodes_.push_back(&input.behavior.Root().children.front());
    }
    bool Run(){
        MonsterBehaviorManagerState manager{};MonsterPathEndpointTargetState target{};
        manager.words[0x204/4]=grid_id;manager.words[0x6d0/4]=nodes_.empty()?0:1;manager.words[0x668/4]=memory_id;
        manager.words[0x14/4]=actor_id;manager.words[0xdc/4]=std::bit_cast<std::uint32_t>(input_.manager_radius);
        target.words[(kind_==3?0x04:kind_==0?0x1c:0x14)/4]=manager_id;
        if(kind_==0)MonsterPathEndpointRestore::Accept(target,manager,helper_,helper_id,*this);
        else if(kind_==1)MonsterPathEndpointC03Restore::Accept(target,manager,helper_,helper_id,*this);
        else MonsterRandomPointRestore::Accept(target,manager,helper_,helper_id,*this,*this);
        return (helper_.words[0xb8/4]&0xff)!=0;
    }
    std::string ReadStringAttribute(std::uint32_t node,std::string_view key,std::string_view fallback) override {
        if(!node)return std::string(fallback);
        const auto* a=Node(node).FindAttribute(key);
        if(!a)return std::string(fallback);
        const auto* value=std::get_if<std::string>(&a->value);Require(value!=nullptr,"monster path reference is not text");return *value;
    }
    std::uint32_t ResolveWorldPath(std::string_view name) override {return world_.FindPath(name);}
    std::uint32_t GridParameter(std::uint32_t g) override {Require(g==grid_id,"invalid monster grid handle");return world_.GridParameter();}
    void ConfigurePath(BehaviorPathEndpointState& helper,std::uint32_t h,std::uint32_t parameter,std::uint32_t path) override {
        Helper(h);Require(&helper==&helper_,"monster helper owner mismatch");(void)configuration_.Configure(helper,parameter,path);
    }
    std::int32_t ChildCount(std::uint32_t node) override {
        if(!node)return 0;
        const auto n=Node(node).children.size();
        Require(n<=10000,"too many monster restore XML children");
        return static_cast<std::int32_t>(n);
    }
    std::uint32_t Child(std::uint32_t node,std::uint32_t i) override {
        const auto& children=Node(node).children;Require(i<children.size() && nodes_.size()<100000,"invalid monster restore child");
        nodes_.push_back(&children[i]);return static_cast<std::uint32_t>(nodes_.size());
    }
    std::string ChildName(std::uint32_t node) override {return Node(node).name;}
    std::int32_t ReadIntAttribute(std::uint32_t node,std::string_view key,std::int32_t fallback) override {return node?Int(Node(node),key,fallback):fallback;}
    float ReadFloatAttribute(std::uint32_t node,std::string_view key,float fallback) override {return node?Float(Node(node),key,fallback):fallback;}
    std::int32_t ReadMemoryInt(std::uint32_t m,std::string_view key,std::int32_t fallback) override {Require(m==memory_id,"invalid monster memory handle");return Int(input_.memory,key,fallback);}
    float ReadMemoryFloat(std::uint32_t m,std::string_view key,float fallback) override {Require(m==memory_id,"invalid monster memory handle");return Float(input_.memory,key,fallback);}
    void SetHelperScalar(std::uint32_t h,std::uint32_t slot,float value) override {Helper(h);Require(std::isfinite(value),"non-finite helper scalar");BehaviorHelperConfiguration::SetScalar(helper_,slot,value);}
    MonsterTransformRows ActorTransform(std::uint32_t a) override {Require(a==actor_id,"invalid monster actor handle");return input_.transform;}
    void SetHelperVector(std::uint32_t h,std::uint32_t slot,const BehaviorVector3& value) override {Helper(h);BehaviorHelperConfiguration::SetVector(helper_,slot,value);}
    void AddGridElement(std::uint32_t grid,std::uint32_t h) override {Require(grid==grid_id,"invalid monster grid handle");Helper(h);world_.AddElement(helper_);registered=true;}
    bool SelectAnimation(std::uint32_t a,std::string_view name) override {Animation(a);return playback_.Select(name);}
    void SetAnimationFlag(std::uint32_t a,std::uint32_t flag) override {Animation(a);playback_.SetRestart(flag!=0);}
    std::int32_t ScriptVersion() override {return kind_;}
    std::int32_t SoundRuntimeType() override {return 3;}
    BehaviorVector3 CurrentHelperPoint(std::uint32_t h) override {Helper(h);return BehaviorHelperConfiguration::ReadVector(helper_,0x18);}
    float ReadSoundActionTime(std::uint32_t m,float fallback) override {return ReadMemoryFloat(m,"AAB_MST_memory_actiontime",fallback);}
    void SetAnimationTime(std::uint32_t a,float time) override {Animation(a);playback_.Seek(time);}
    std::uint32_t AnimationFlag(std::uint32_t a) override {Animation(a);return playback_.Completed();}
};
bool Restore(std::int32_t kind,BehaviorPathEndpointState& helper,const MonsterPathRestoreInput& input,ActorPlayback& playback,IMonsterPathRestoreWorld& world){
    Require(helper.words[0]==(kind==3?0x842ae8u:BehaviorPathEndpointHelper::kVtable),"invalid monster restore helper");
    Require(!helper.words[42] && !(helper.words[46]&0xff),"restore requires unbound inactive helper");
    Require(std::isfinite(input.manager_radius),"invalid monster manager radius");
    for(const auto& row:input.transform)for(float f:row)Require(std::isfinite(f),"invalid monster transform");
    const auto before=helper;Runtime runtime(kind,helper,input,playback,world);
    try{return runtime.Run();}
    catch(...){if(runtime.registered)world.RemoveElement(helper);world.ReleaseAssociation(helper);helper=before;throw;}
}
}
bool RestoreMonsterPath(std::int32_t kind,BehaviorPathEndpointState& helper,const MonsterPathRestoreInput& input,ActorPlayback& playback,IMonsterPathRestoreWorld& world){
    Require(kind==0 || kind==1,"unsupported monster path restore kind");return Restore(kind,helper,input,playback,world);
}
bool RestoreMonsterRandom(BehaviorPathEndpointState& helper,const MonsterPathRestoreInput& input,ActorPlayback& playback,IMonsterPathRestoreWorld& world){
    return Restore(3,helper,input,playback,world);
}
}
