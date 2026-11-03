#include "pal4/portable/controller.h"
#include "pal4/actor_control_free_movement.h"
#include "pal4/actor_control_input_component.h"
#include "pal4/game_vector_math.h"
#include <bit>
#include <cmath>
#include <stdexcept>
namespace pal4::portable {
namespace {
using Vector=std::array<float,3>;
const pal4::GameMathTables& Tables(){
    static const auto tables=[]{pal4::GameMathTables value;(void)value.CalculateSquareRootTable();return value;}();
    return tables;
}
void Finite(const Vector& value){for(float v:value)if(!std::isfinite(v))throw std::invalid_argument("non-finite free input vector");}
class Runtime final:public pal4::IActorControlFreeMovementRuntime {
public:
    const FreeControlInput& input;const ControlCameraBasis& camera;Vector position;float threshold;
    const std::function<void(Vector,float)>& dispatch;
    Runtime(const FreeControlInput& i,const ControlCameraBasis& c,Vector p,float t,
        const std::function<void(Vector,float)>& d):input(i),camera(c),position(p),threshold(t),dispatch(d){}
    std::uint32_t InputManager() override{return 0x1000;}
    std::uint32_t ReadInputWord(std::uint32_t manager,std::uint32_t offset) override {
        if(manager!=0x1000)throw std::logic_error("invalid free input manager");
        switch(offset){
        case 0x1f44:case 0x1f38:return input.forward;
        case 0x1f54:case 0x1f48:return input.backward;
        case 0x1f24:case 0x1f18:return input.right;
        case 0x1f34:case 0x1f28:return input.left;
        case 0x21d4:case 0x21c8:return input.mouse_world.has_value();
        // 第二组原键盘别名已由平台合并为同一方向状态。
        case 0x1e34:case 0x1e28:case 0x1df4:case 0x1de8:
        case 0x1cd4:case 0x1cc8:case 0x1d04:case 0x1cf8:return 0;
        default:throw std::logic_error("invalid free input key offset");
        }
    }
    std::uint32_t CameraSystem() override{return 0x2000;}
    std::uint32_t ActiveCameraObjectPointer(std::uint32_t system) override {
        if(system!=0x2000)throw std::logic_error("invalid free input camera system");
        return 0x3000;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if(address==0x3004)return 0x4000;
        if(address>=0x5000 && address<=0x5008 && (address-0x5000)%4==0)return std::bit_cast<std::uint32_t>(camera.right[(address-0x5000)/4]);
        if(address>=0x5020 && address<=0x5028 && (address-0x5020)%4==0)return std::bit_cast<std::uint32_t>(camera.forward[(address-0x5020)/4]);
        if(address>=0x60ac && address<=0x60b4 && (address-0x60ac)%4==0)return std::bit_cast<std::uint32_t>(position[(address-0x60ac)/4]);
        throw std::logic_error("invalid free input numeric handle");
    }
    std::uint32_t ObjectTransform(std::uint32_t object) override {
        if(object!=0x4000)throw std::logic_error("invalid free input camera object");
        return 0x5000;
    }
    bool MouseWorldCoordinates(pal4::ActorControlInputComponentState&,std::uint32_t actor,pal4::ActorControlVec3& output) override {
        if(actor!=0x6000 || !input.mouse_world)throw std::logic_error("invalid free input world target");
        const auto& p=*input.mouse_world;output={p[0],p[1],p[2]};return true;
    }
    float ControllerThreshold() override{return threshold;}
    double ApproximateMagnitude(const pal4::ActorControlVec3& p) override {
        return pal4::GameVectorMath::ApproximateMagnitude({p.x,p.y,p.z},Tables());
    }
    void NormalizeDirection(pal4::ActorControlInputComponentState& state) override {
        Vector direction{};for(unsigned i=0;i<3;++i)direction[i]=std::bit_cast<float>(state.words[6+i]);
        pal4::GameVectorMath::NormalizeAndClamp(direction,Tables());
        for(unsigned i=0;i<3;++i)state.words[6+i]=std::bit_cast<std::uint32_t>(direction[i]);
    }
    void DispatchVectorMovement(pal4::ActorControlInputComponentState& state,std::uint32_t actor,float elapsed) override {
        if(actor!=0x6000)throw std::logic_error("invalid free input dispatch actor");
        Vector direction{};for(unsigned i=0;i<3;++i)direction[i]=std::bit_cast<float>(state.words[6+i]);
        dispatch(direction,elapsed);
    }
};
}
bool ActorController::UpdateFreeInput(const FreeControlInput& input,const ControlCameraBasis& camera,
    Vector position,float elapsed,const std::function<void(Vector,float)>& dispatch) {
    if(!Initialized())throw std::logic_error("free input before controller initialization");
    if(!std::isfinite(elapsed) || elapsed<0)throw std::invalid_argument("invalid free input elapsed");
    Finite(camera.right);Finite(camera.forward);Finite(position);if(input.mouse_world)Finite(*input.mouse_world);
    if(!state_.enabled || !state_.actor)return false;
    if(state_.input.event_mode)throw std::logic_error("event input mode requires its own controller route");
    if(!dispatch)throw std::logic_error("free movement dispatch is not bound");
    pal4::ActorControlInputComponentState image;
    image.words[1]=std::bit_cast<std::uint32_t>(state_.input.speed);
    image.words[2]=std::bit_cast<std::uint32_t>(state_.input.secondary_speed);
    image.words[3]=std::bit_cast<std::uint32_t>(state_.input.turn_scale);
    image.words[4]=state_.input.result;image.words[5]=state_.input.event_mode;
    for(unsigned i=0;i<4;++i)image.words[6+i]=state_.input.pending[i];
    image.words[10]=state_.input.mode;image.words[11]=std::bit_cast<std::uint32_t>(state_.input.multiplier);
    Runtime runtime(input,camera,position,state_.collision_radius,dispatch);
    const auto result=pal4::ActorControlInputComponent::UpdateFreeMovement(image,0x6000,elapsed,runtime);
    state_.input.result=image.words[4];for(unsigned i=0;i<4;++i)state_.input.pending[i]=image.words[6+i];
    return result!=0;
}
}
