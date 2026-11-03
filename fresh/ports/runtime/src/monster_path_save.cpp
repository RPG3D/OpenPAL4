#include "pal4/portable/monster_random_state.h"
#include "pal4/monster_random_point_save.h"
#include "pal4/portable/binary_xml_memory.h"
#include "pal4/monster_path_endpoint_save.h"
#include "pal4/monster_behavior_manager.h"
#include <bit>
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::invalid_argument(message);}
class Runtime final:public IMonsterPathEndpointSaveRuntime,public IMonsterRandomPointRuntime {
    std::int32_t kind_;MonsterHelperHandle helper_;BinaryXmlNode& memory_;ActorPlayback& playback_;MonsterMovementWorld& world_;
    static constexpr std::uint32_t memory_token=1,grid_token=2,manager_token=3,animation_token=manager_token+0x170;
public:
    Runtime(std::int32_t kind,MonsterHelperHandle h,BinaryXmlNode& memory,ActorPlayback& p,MonsterMovementWorld& w):kind_(kind),helper_(h),memory_(memory),playback_(p),world_(w){}
    std::uint32_t Run(){
        auto& helper=world_.Helper(helper_);Require(helper.words[0]==(kind_==3?0x842ae8u:0x842898u) && world_.Registered(helper_),"save requires the registered matching helper");
        MonsterBehaviorManagerState manager{};manager.words[0x204/4]=grid_token;manager.words[0x668/4]=memory_token;
        MonsterPathEndpointTargetState target{};target.words[(kind_==3?0x04:kind_==0?0x1c:0x14)/4]=manager_token;
        if(kind_==0)return MonsterPathEndpointSave::Release(target,manager,helper,helper_.token,*this);
        if(kind_==1)return MonsterPathEndpointC03Save::Release(target,manager,helper,helper_.token,*this);
        return MonsterRandomPointSave::Release(target,manager,helper,helper_.token,*this,*this);
    }
    std::int32_t ScriptVersion() override {return kind_;}
    std::int32_t SoundRuntimeType() override {return 3;}
    BehaviorVector3 CurrentHelperPoint(std::uint32_t) override {throw std::logic_error("save cannot request current point");}
    float ReadSoundActionTime(std::uint32_t,float) override {throw std::logic_error("save cannot read action time from memory");}
    void WriteMemoryInt(std::uint32_t token,std::string_view key,std::int32_t value) override {Require(token==memory_token,"invalid save memory token");WriteBinaryXmlInt(memory_,key,value);}
    void WriteMemoryFloat(std::uint32_t token,std::string_view key,float value) override {Require(token==memory_token,"invalid save memory token");WriteBinaryXmlFloat(memory_,key,value);}
    float HelperSpeed(std::uint32_t token) override {Require(token==helper_.token,"invalid save helper token");return std::bit_cast<float>(world_.Helper(helper_).words[25]);}
    float AnimationTime(std::uint32_t token) override {Require(token==animation_token,"invalid save animation token");return playback_.Time();}
    void RemoveGridElement(std::uint32_t grid,std::uint32_t token) override {Require(grid==grid_token && token==helper_.token,"invalid save grid token");world_.DestroyHelper(helper_);}
    std::uint32_t SelectAction(std::uint32_t token,std::string_view name,float blend) override {Require(token==animation_token,"invalid save animation token");return playback_.SelectBlended(name,blend)?1u:0u;}
    std::uint32_t SetAnimationFlag(std::uint32_t token,std::uint32_t flag) override {Require(token==animation_token && flag==1,"invalid save restart flag");playback_.SetRestart(true);return 1;}
};
}
std::uint32_t SaveMonsterPath(std::int32_t kind,MonsterHelperHandle helper,BinaryXmlNode& memory,ActorPlayback& playback,MonsterMovementWorld& world){
    Require(kind==0 || kind==1,"unsupported monster path save kind");Runtime runtime(kind,helper,memory,playback,world);return runtime.Run();
}
std::uint32_t SaveMonsterRandom(MonsterHelperHandle helper,BinaryXmlNode& memory,ActorPlayback& playback,MonsterMovementWorld& world){
    Runtime runtime(3,helper,memory,playback,world);return runtime.Run();
}
}
