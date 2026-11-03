#include "pal4/portable/player_notifications.h"
#include "pal4/script_player_position.h"
#include <algorithm>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace pal4::portable {
struct PlayerNotifications::Impl {
    struct Record {pal4::PlayerPositionWaitState state{};std::string name;};
    std::unique_ptr<Record> pending;
    std::vector<std::unique_ptr<Record>> records;
    void Require(const pal4::PlayerPositionWaitState* state) const {
        if(!pending || state!=&pending->state)throw std::logic_error("invalid player notification record");
    }
};
PlayerNotifications::PlayerNotifications():impl_(std::make_unique<Impl>()){}
PlayerNotifications::~PlayerNotifications()=default;
pal4::PlayerPositionWaitState* PlayerNotifications::Allocate(std::uint32_t size) {
    if(impl_->pending || size!=sizeof(pal4::PlayerPositionWaitState))throw std::logic_error("invalid player notification allocation");
    impl_->pending=std::make_unique<Impl::Record>();return &impl_->pending->state;
}
void PlayerNotifications::Construct(pal4::PlayerPositionWaitState& state,std::uint32_t mode,std::uint32_t target) {
    impl_->Require(&state);
    if(mode || target!=queue)throw std::logic_error("invalid player notification construction");
    state={};state.phase=0xffffffff;state.script_context=target;
}
bool PlayerNotifications::AllocateText(pal4::PlayerPositionWaitState& state,std::uint32_t size) {
    impl_->Require(&state);impl_->pending->name.reserve(size);state.name_capacity=size;state.name_data=1;return true;
}
void PlayerNotifications::AssignName(pal4::PlayerPositionWaitState& state,std::string_view name,std::uint32_t phase,std::uint32_t vtable) {
    impl_->Require(&state);
    if(state.phase!=phase || state.vtable_address!=vtable || !state.enabled || state.wait_mode)
        throw std::logic_error("invalid player notification state");
    impl_->pending->name=name;state.name_data=1;state.name_size=static_cast<std::uint32_t>(name.size());state.name_capacity=state.name_size;
}
void PlayerNotifications::Close(pal4::PlayerPositionWaitState& state) {impl_->Require(&state);state.enabled=1;}
void PlayerNotifications::Add(std::uint32_t target,pal4::PlayerPositionWaitState* state) {
    impl_->Require(state);
    if(target!=queue || state->script_context!=queue)throw std::logic_error("invalid player notification queue");
    std::erase_if(impl_->records,[&](const auto& record){return record->name==impl_->pending->name && record->state.phase==state->phase;});
    impl_->records.push_back(std::move(impl_->pending));
}
void PlayerNotifications::Process(){std::erase_if(impl_->records,[](const auto& record){return record->state.enabled!=0;});}
std::size_t PlayerNotifications::Count(std::uint32_t phase) const {
    return static_cast<std::size_t>(std::count_if(impl_->records.begin(),impl_->records.end(),[&](const auto& record){return record->state.phase==phase;}));
}
void PlayerNotifications::WriteReport(std::ostream& output) const {
    output<<'[';bool first=true;for(const auto& record:impl_->records) {
        if(!first)output<<',';
        first=false;
        output<<"{\"name\":"<<std::quoted(record->name)<<",\"phase\":"<<record->state.phase
            <<",\"closed\":"<<(record->state.enabled?"true":"false")<<",\"mode\":"<<unsigned(record->state.wait_mode)<<'}';
    }output<<']';
}
}
