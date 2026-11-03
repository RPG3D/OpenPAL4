#include "pal4/portable/controller.h"
#include "pal4/floor_effect_kind.h"
#include "pal4/actor_control_movement_mode.h"
#include <bit>
#include <cmath>
#include <ostream>
#include <stdexcept>

namespace pal4::portable {
void ActorController::Shutdown() noexcept {
    floor_callbacks_={};dummy_.reset();state_={};
}
void ActorController::Initialize(std::int32_t width,std::int32_t height,const Loader& load_dummy) {
    Shutdown();
    if(width<=0 || height<=0)throw std::invalid_argument("controller logical dimensions must be positive");
    ControllerState next;
    next.input.speed=std::bit_cast<float>(0x4329e666u);
    next.input.secondary_speed=std::bit_cast<float>(0x42fdcccdu);
    next.input.turn_scale=static_cast<float>(270.0/width);
    next.vertical_scale=static_cast<float>(40.0/height);
    next.horizontal_scale=static_cast<float>(120.0/width);
    auto dummy=std::make_unique<Actor>(load_dummy());
    if(dummy->frames.empty() || dummy->mesh.vertices.empty())throw std::runtime_error("empty controller JDumy resource");
    std::vector<Vertex> pose;SampleActorVertices(*dummy,-1,0,pose);
    // 原初始化只注册FL/FR地面分类入口；没有选中玩家时不会生成脚底特效。
    std::array<std::function<unsigned(std::string_view)>,2> callbacks;
    for(auto& callback:callbacks)callback=[](std::string_view path){return pal4::FloorEffectKind(path);};
    state_=next;floor_callbacks_=std::move(callbacks);dummy_=std::move(dummy);
}
std::uint32_t ActorController::SetMovementMode(std::uint32_t mode) {
    if(!Initialized())throw std::logic_error("movement mode before controller initialization");
    state_.input.mode=mode;
    return pal4::ApplyActorMovementMode(mode,state_.input.multiplier,state_.input.speed);
}
void ActorController::SetAnalogMovementTuning(std::uint32_t mode,float multiplier) {
    if(mode>2 || !std::isfinite(multiplier) || multiplier<=0 || multiplier>1.5f)
        throw std::invalid_argument("invalid analog movement tuning");
    if(!Initialized())throw std::logic_error("analog movement before controller initialization");
    if(state_.input.mode!=mode)SetMovementMode(mode);
    state_.input.speed=169.89999f*multiplier;
}
void ActorController::SetAnalogMovementResult(bool moved) {
    if(!Initialized())throw std::logic_error("analog result before controller initialization");
    state_.input.result=moved?1u:0u;
}
unsigned ActorController::ClassifyFloor(unsigned foot,std::string_view path) const {
    if(foot>=floor_callbacks_.size() || !floor_callbacks_[foot])throw std::logic_error("controller floor callback is not registered");
    return floor_callbacks_[foot](path);
}
void ActorController::SelectActor(std::uint32_t index,std::uint32_t scene_type) {
    if(!Initialized() || index>=4)throw std::logic_error("controller player scene binding is not ready");
    state_.actor=index;state_.database.actor=index;
    state_.input.result=0;state_.input.pending={};state_.input.event_mode=scene_type!=1;
    state_.motion.active=false;state_.motion.pending=false;state_.motion.reached=true;state_.motion.mode=0;
    state_.input_vector={};state_.transient={};
}
void WriteControllerReport(std::ostream& output,const ActorController& controller) {
    const auto& state=controller.State();const auto* dummy=controller.Dummy();
    output<<"{\"initialized\":"<<(controller.Initialized()?"true":"false")
        <<",\"modern_gamepad\":"<<(PAL4_MODERN_GAMEPAD?"true":"false")
        <<",\"pointer_bits\":"<<sizeof(void*)*8<<",\"dummy_frames\":"<<(dummy?dummy->frames.size():0)
        <<",\"dummy_vertices\":"<<(dummy?dummy->mesh.vertices.size():0)
        <<",\"floor_callbacks_registered\":"<<(controller.FloorCallbacksRegistered()?"true":"false")
        <<",\"floor_effect_rendering_bound\":false,\"gameplay_initialized\":false,\"enabled\":"<<(state.enabled?"true":"false")
        <<",\"reset_count\":"<<state.reset_count<<",\"animation_scale\":"<<state.animation_scale
        <<",\"actor_selected\":"<<(state.actor?"true":"false")<<",\"database_actor_selected\":"<<(state.database.actor?"true":"false")
        <<",\"motion_active\":"<<(state.motion.active?"true":"false")<<",\"motion_pending\":"<<(state.motion.pending?"true":"false")
        <<",\"motion_reached\":"<<(state.motion.reached?"true":"false")<<",\"motion_mode\":"<<state.motion.mode
        <<",\"motion_speed\":"<<state.motion.speed<<",\"input_result\":"<<state.input.result<<",\"input_mode\":"<<state.input.mode
        <<",\"event_mode\":"<<state.input.event_mode<<",\"value_bits\":[";
    bool first=true;
    for(float value:{state.input.speed,state.input.secondary_speed,state.input.turn_scale,state.vertical_scale,state.horizontal_scale,state.input.multiplier,
                      state.interaction_distance,state.collision_radius,state.floor_elapsed}) {
        if(!first)output<<',';
        first=false;output<<std::bit_cast<std::uint32_t>(value);
    }
    output<<"],\"database_flags\":[";
    for(std::size_t i=0;i<state.database.flags.size();++i){if(i)output<<',';output<<state.database.flags[i];}
    output<<"]}";
    if(!output)throw std::runtime_error("cannot write controller report");
}
} // namespace pal4::portable
