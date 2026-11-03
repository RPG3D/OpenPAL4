#include "pal4/portable/player.h"
#include "pal4/player_manager_leader_transition.h"
#include "pal4/player_manager_object.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace pal4::portable {
// Numeric addresses below are checked layout handles, never host pointers.
struct PlayerRoster::LeaderRuntime final:pal4::IPlayerManagerLeaderRuntime {
    static constexpr std::uint32_t manager=0x10000;
    PlayerRoster& players;
    const std::function<void(std::uint32_t)>& bind;
    explicit LeaderRuntime(PlayerRoster& value,const std::function<void(std::uint32_t)>& callback):players(value),bind(callback){}
    static std::uint32_t Index(std::uint32_t slot) {
        if(slot<manager || (slot-manager)%pal4::PlayerManagerObject::kElementBytes ||
           (slot-manager)/pal4::PlayerManagerObject::kElementBytes>=4)throw std::logic_error("invalid player layout handle");
        return (slot-manager)/pal4::PlayerManagerObject::kElementBytes;
    }
    PlayerSlot& Slot(std::uint32_t slot){return players.slots_.at(Index(slot));}
    std::uint32_t ReadDword(std::uint32_t address) override {
        if(address==manager+pal4::PlayerManagerLeaderTransition::kCurrentIndexOffset)return players.leader_;
        for(unsigned i=0;i<4;++i){const auto base=manager+i*pal4::PlayerManagerObject::kElementBytes;
            if(address==base+0xb08)return players.slots_[i].inventory_enabled;
            if(address==base+0xb0c)return players.slots_[i].auto_seek;
        }
        throw std::logic_error("unbound player layout read");
    }
    void WriteDword(std::uint32_t address,std::uint32_t value) override {
        if(address!=manager+pal4::PlayerManagerLeaderTransition::kCurrentIndexOffset || value>=4)throw std::logic_error("invalid player leader write");
        players.leader_=value;
    }
    void SetStateFlag(std::uint32_t slot,std::uint32_t value) override {Slot(slot).active=value!=0;}
    void SetAction(std::uint32_t component,std::uint32_t action) override {
        if(action!=pal4::PlayerManagerLeaderTransition::kActionName)throw std::logic_error("unbound leader action");
        Slot(component-pal4::PlayerManagerLeaderTransition::kActionOffset).animation="C01";
    }
    void CopyTransform(std::uint32_t slot,std::uint32_t method,std::uint32_t source,std::uint32_t option) override {
        if(option)throw std::logic_error("invalid leader transform mode");
        if(method==4)Slot(slot).position=Slot(source-0xac).position;
        else if(method==8)Slot(slot).rotation=Slot(source-0xa0).rotation;
        else throw std::logic_error("unbound leader transform method");
    }
    void SetPlayerState(std::uint32_t owner,std::uint32_t value) override {
        if(owner!=manager)throw std::logic_error("invalid player manager handle");
        players.SetAutoSeek(value!=0);
    }
    std::uint32_t Arena() override{return 1;}
    void OnRemove(std::uint32_t,std::uint32_t slot) override {auto& p=Slot(slot);p.active=false;p.world.clear();p.scene.clear();}
    void OnAdd(std::uint32_t,std::uint32_t slot) override {auto& p=Slot(slot);p.active=true;p.world=players.world_;p.scene=players.scene_;}
    std::uint32_t ActorControl() override{return 1;}
    void BindActor(std::uint32_t,std::uint32_t slot) override {bind(Index(slot));}
    void SortActive(std::vector<std::uint32_t>& slots,std::uint32_t comparator) override {
        if(comparator!=pal4::PlayerManagerLeaderTransition::kComparator)throw std::logic_error("invalid player priority comparator");
        // The original insertion sort keeps equal priorities in slot order.
        std::stable_sort(slots.begin(),slots.end(),[&](auto a,auto b){return std::bit_cast<std::int32_t>(Slot(a).script_index)<std::bit_cast<std::int32_t>(Slot(b).script_index);});
    }
    std::int32_t GetPriority(std::uint32_t script) override{return std::bit_cast<std::int32_t>(Slot(script-pal4::PlayerManagerLeaderTransition::kScriptOffset).script_index);}
    void SetPriority(std::uint32_t script,std::int32_t value) override {Slot(script-pal4::PlayerManagerLeaderTransition::kScriptOffset).script_index=std::bit_cast<std::uint32_t>(value);}
};
void PlayerRoster::SetAutoSeek(bool enabled) {
    if(!prepared_)throw std::logic_error("camera follow requires prepared players");
    for(auto& slot:slots_)slot.auto_seek=false;
    slots_[leader_].auto_seek=enabled;
}
void PlayerRoster::SetLeader(std::int32_t index,const std::function<void(std::uint32_t)>& bind_actor) {
    if(!prepared_ || index<0 || index>=4)throw std::invalid_argument("invalid player leader index");
    const auto& next=slots_[static_cast<unsigned>(index)];
    if(leader_!=static_cast<unsigned>(index) && next.inventory_enabled) {
        if(!bind_actor)throw std::logic_error("leader scene binding is unavailable");
        if(std::none_of(next.actor->clips.begin(),next.actor->clips.end(),[](const auto& clip){return clip.name=="C01";}))
            throw std::runtime_error("leader C01 animation is missing");
    }
    LeaderRuntime runtime(*this,bind_actor);
    pal4::PlayerManagerLeaderTransition::Switch(LeaderRuntime::manager,static_cast<unsigned>(index),runtime);
}
} // namespace pal4::portable
