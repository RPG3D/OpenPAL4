#include "pal4/portable/controller.h"
#include "pal4/actor_control_input_events.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace pal4::portable {
namespace {
class EventInput final : public pal4::IActorControlInputEventRuntime {
public:
    EventInput(const FreeControlInput& input,const std::function<void(std::uint32_t,float)>& dispatch)
        : input_(input),dispatch_(dispatch) {}
    std::uint32_t InputManager() override {return 1;}
    std::uint32_t ReadInputWord(std::uint32_t manager,std::uint32_t offset) override {
        if(manager!=1)throw std::logic_error("invalid event input manager");
        switch(offset){
        case 0x1f44:case 0x1f38:return input_.forward;
        case 0x1f54:case 0x1f48:return input_.backward;
        case 0x1f24:case 0x1f18:return input_.right;
        case 0x1f34:case 0x1f28:return input_.left;
        case 0x1e34:case 0x1e28:case 0x1df4:case 0x1de8:
        case 0x1cd4:case 0x1cc8:case 0x1d04:case 0x1cf8:
        case 0x21d4:case 0x21c8:case 0x21e4:case 0x21d8:return 0;
        default:throw std::logic_error("invalid event input offset");
        }
    }
    pal4::ActorControlScreenPoint CursorPoint() override {return {};}
    pal4::ActorControlScreenRect WindowRect() override {return {};}
    void GetInputStateValues(std::uint32_t,std::uint32_t&,std::uint32_t&) override {throw std::logic_error("mouse event input is not bound");}
    void DispatchMovement(pal4::ActorControlInputComponentState&,std::uint32_t,std::uint32_t command,float elapsed) override {dispatch_(command,elapsed);}
    void TurnFromMouse(pal4::ActorControlInputComponentState&,std::uint32_t,std::int32_t) override {throw std::logic_error("mouse event input is not bound");}
private:
    const FreeControlInput& input_;
    const std::function<void(std::uint32_t,float)>& dispatch_;
};
}
bool ActorController::UpdateEventInput(const FreeControlInput& input,float elapsed,
    const std::function<void(std::uint32_t,float)>& dispatch) {
    if(!Initialized())throw std::logic_error("event input before controller initialization");
    if(!std::isfinite(elapsed) || elapsed<0)throw std::invalid_argument("invalid event input elapsed");
    if(!state_.enabled || !state_.actor)return false;
    if(!state_.input.event_mode)throw std::logic_error("event input called in free input mode");
    if(input.mouse_world)throw std::logic_error("event mouse input is not bound");
    if(!dispatch)throw std::logic_error("event movement dispatch is not bound");
    pal4::ActorControlInputComponentState image;
    image.words[1]=std::bit_cast<std::uint32_t>(state_.input.speed);
    image.words[2]=std::bit_cast<std::uint32_t>(state_.input.secondary_speed);
    image.words[3]=std::bit_cast<std::uint32_t>(state_.input.turn_scale);
    image.words[4]=state_.input.result;image.words[5]=state_.input.event_mode;
    for(unsigned i=0;i<4;++i)image.words[6+i]=state_.input.pending[i];
    image.words[10]=state_.input.mode;image.words[11]=std::bit_cast<std::uint32_t>(state_.input.multiplier);
    EventInput runtime(input,dispatch);
    const auto result=pal4::ActorControlInputComponent::HandleInputEvents(image,1,elapsed,runtime);
    state_.input.result=image.words[4];for(unsigned i=0;i<4;++i)state_.input.pending[i]=image.words[6+i];
    return result!=0;
}
}
