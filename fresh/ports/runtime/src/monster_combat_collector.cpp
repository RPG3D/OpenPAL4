#include "pal4/portable/monster_combat_collector.h"
#include "pal4/monster_combat_interaction_callback.h"
#include "pal4/game_vector_math.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::logic_error(message);}
const GameMathTables& Math(){static const auto value=[]{GameMathTables t;(void)t.CalculateSquareRootTable();return t;}();return value;}
}
struct MonsterCombatCollector::Impl final:IMonsterCombatPositionRuntime,IMonsterCombatInteractionRuntime,IMonsterActionDispatchRuntime {
    SceneMonsterInstances& instances;MonsterSceneActors& group;MonsterPlayerView& players;const CollisionMesh& collision;
    std::vector<std::size_t> order;std::size_t initiating{},source_index{};bool busy{};
    Impl(SceneMonsterInstances& i,MonsterSceneActors& g,MonsterPlayerView& p,const CollisionMesh& c,std::vector<std::size_t> o)
        :instances(i),group(g),players(p),collision(c),order(std::move(o)){
        Require(group.Size()==instances.Size() && order.size()==group.Size(),"incomplete monster combat traversal");
        std::vector<bool> seen(group.Size());
        for(auto index:order){Require(index<group.Size() && !seen[index],"invalid monster combat traversal order");seen[index]=true;}
        for(std::size_t n=0;n<group.Size();++n){const auto& a=group.At(n);Require(a.Handle()==instances.Handle(n),"monster combat actor order mismatch");for(std::size_t j=0;j<n;++j)Require(group.At(j).ManagerToken()!=a.ManagerToken(),"duplicate monster combat identity");}
    }
    MonsterSceneActor& Current(){return group.At(initiating);}
    const SceneMonster& Source(std::uint32_t token,std::size_t index){auto& a=group.At(index);Require(a.State().words[0x654/4]==token && token==a.Behavior().SourceToken(),"monster combat source mismatch");return instances.Source(a.Handle());}
    std::int32_t PositionCount(std::uint32_t source) override{return static_cast<std::int32_t>(Source(source,source_index).values.size());}
    std::uint32_t PositionId(std::uint32_t source,std::uint32_t index) override{return Source(source,source_index).values.at(index);}
    std::int32_t PositionValue(std::uint32_t source,const std::string& key,std::int32_t fallback) override{
        const auto* value=Source(source,source_index).cache.Root().FindAttribute(key);return value?std::bit_cast<std::int32_t>(value->ToInt(std::bit_cast<std::uint32_t>(fallback))):fallback;
    }
    void AppendRecord(MonsterBehaviorManagerState& state,std::uint32_t address,std::uint32_t end,std::uint32_t count,const std::array<std::uint32_t,3>& record) override{
        Require(&state==&Current().State() && address==Current().ManagerToken()+0x710 && end==state.words[0x718/4] && count==1,"invalid monster combat record owner");Current().Encounter().Append(record);
    }
    void AddManagerText(std::uint32_t manager,std::uint32_t text) override{
        Require(manager==Current().ManagerToken() && text==group.At(source_index).State().words[1],"invalid monster combat label");Current().Encounter().AddName(group.At(source_index).Label());
    }
    IMonsterActionDispatchRuntime& ActionRuntime() override{return *this;}
    std::uint32_t ManagerInstance() override{return 1;}
    std::uint32_t IterateAndCallback(std::uint32_t instance,std::uint32_t callback,std::uint32_t context) override{
        Require(instance==1 && callback==MonsterCombatPositionDispatcher::kCallback && context==Current().ManagerToken(),"invalid monster combat iteration");
        for(auto index:order)(void)MonsterCombatInteractionCallback::Process(Current().State(),group.At(index).ManagerToken(),context,*this);
        return 1;
    }
    MonsterCombatCandidate Candidate(std::uint32_t token) override{
        for(auto index:order)if(group.At(index).ManagerToken()==token){source_index=index;auto& state=group.At(index).State();MonsterCombatCandidate value{};
            value.active=state.words[0x1f0/4]!=0;value.pursuit_mode=state.words[0x6e0/4];value.source_address=state.words[0x654/4];value.text_address=state.words[1];value.timer_word=state.words[0x730/4];
            for(unsigned a=0;a<3;++a){value.position[a]=std::bit_cast<float>(state.words[43+a]);value.collision_position[a]=std::bit_cast<float>(state.words[56+a]);}return value;}
        throw std::invalid_argument("unknown monster combat candidate");
    }
    std::string ScriptFunction(std::uint32_t source) override{
        const auto* value=Source(source,source_index).cache.Root().FindAttribute("AAB_MST_ESSBEH_script_func");if(!value)return {};
        const auto* text=std::get_if<std::string>(&value->value);Require(text,"invalid combat script attribute");return *text;
    }
    double TargetDoubleProperty(std::uint32_t target) override{const auto player=players.ControlledActor();Require(player.active && player.address==target && std::isfinite(player.half_height) && player.half_height>=0,"invalid combat target");return 2.0*player.half_height;}
    double PlanarDistance(float x,float z) override{return GameVectorMath::ApproximateMagnitude({x,0,z},Math());}
    float InteractionRadius(std::uint32_t source) override{const auto value=std::bit_cast<float>(Source(source,initiating).fields[3]);Require(std::isfinite(value),"invalid monster combat radius");return value;}
    std::uint32_t CollisionClump() override{return collision.wall.empty()?0u:1u;}
    bool CollisionBlocked(const std::array<float,6>& e,std::uint32_t clump) override{Require(clump==CollisionClump(),"invalid monster combat wall");return clump && SegmentBlocked(collision,{e[0],e[1],e[2]},{e[3],e[4],e[5]});}
    IMonsterCombatPositionRuntime& Records() override{return *this;}
    void Collect(std::size_t index){
        Require(!busy && index<group.Size() && group.Size()==order.size(),"monster combat collector unavailable");
        initiating=source_index=index;busy=true;struct Guard{bool& flag;~Guard(){flag=false;}} guard{busy};
        (void)MonsterCombatPositionDispatcher::Process(Current().State(),Current().ManagerToken(),*this);
    }
};
MonsterCombatCollector::MonsterCombatCollector(SceneMonsterInstances& i,MonsterSceneActors& g,MonsterPlayerView& p,const CollisionMesh& c,std::vector<std::size_t> o):impl_(std::make_unique<Impl>(i,g,p,c,std::move(o))){}
MonsterCombatCollector::MonsterCombatCollector(SceneMonsterInstances& i,MonsterSceneActors& g,MonsterPlayerView& p,const CollisionMesh& c)
    :MonsterCombatCollector(i,g,p,c,i.OriginalTraversalOrder()){}
MonsterCombatCollector::~MonsterCombatCollector()=default;
void MonsterCombatCollector::Collect(std::size_t index){impl_->Collect(index);}
}
