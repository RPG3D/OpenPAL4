#pragma once
#include "pal4/portable/actor.h"
#include <functional>
#include <memory>
#include <optional>

namespace pal4::portable {
class IControllerResetServices {
public:
    virtual ~IControllerResetServices()=default;
    virtual void SetDefaultCursor()=0;
    virtual void ClearPlatformInput()=0;
    virtual void HideInteractionUi()=0;
    virtual void SetMonsterPursuitStopped(bool stopped)=0;
};
struct ControllerInput {
    float speed{}, secondary_speed{}, turn_scale{}, multiplier{1};
    std::uint32_t result{}, event_mode{1}, mode{1};
    std::array<std::uint32_t,4> pending{};
};
// 平台只提交按键状态和可选世界点击点；方向选择仍由原428630执行。
struct FreeControlInput {
    bool forward{},backward{},left{},right{};
    std::optional<std::array<float,3>> mouse_world;
};
struct ControlCameraBasis {std::array<float,3> right{},forward{};};
struct ControllerMotion {
    bool active{}, pending{}, reached{true};
    std::uint32_t mode{};
    float speed{40};
    std::array<float,3> target{}, direction{};
};
struct ControllerDatabase {
    std::array<std::uint32_t,7> flags{0,0,0,1,1,0,1};
    std::optional<std::uint32_t> actor;
};
struct ControllerState {
    ControllerInput input;
    ControllerMotion motion;
    ControllerDatabase database;
    bool enabled{};
    std::optional<std::uint32_t> actor;
    float interaction_distance{150}, collision_radius{18}, vertical_scale{}, horizontal_scale{}, floor_elapsed{};
    std::array<float,3> input_vector{};
    std::array<std::uint32_t,8> transient{};
    float animation_scale{1};
    std::uint32_t reset_count{};
};
class ActorController {
public:
    using Loader=std::function<Actor()>;
    ActorController()=default;
    ActorController(const ActorController&)=delete;
    ActorController& operator=(const ActorController&)=delete;
    void Initialize(std::int32_t logical_width, std::int32_t logical_height, const Loader& load_dummy);
    void Shutdown() noexcept;
    void SelectActor(std::uint32_t index,std::uint32_t scene_type);
    void ResetAndSync(bool enabled,IControllerResetServices& services);
    std::uint32_t SetMovementMode(std::uint32_t mode);
    // Optional platform analog adapter. Stock dispatch never calls this.
    void SetAnalogMovementTuning(std::uint32_t mode,float speed_multiplier);
    void SetAnalogMovementResult(bool moved);
    bool UpdateFreeInput(const FreeControlInput& input,const ControlCameraBasis& camera,
        std::array<float,3> position,float elapsed,
        const std::function<void(std::array<float,3>,float)>& dispatch);
    bool UpdateEventInput(const FreeControlInput& input,float elapsed,
        const std::function<void(std::uint32_t,float)>& dispatch);
    void ProcessInput(bool clear_vector,IControllerResetServices& services);
    bool Initialized() const noexcept { return bool(dummy_); }
    const Actor* Dummy() const noexcept { return dummy_.get(); }
    const ControllerState& State() const noexcept { return state_; }
    bool FloorCallbacksRegistered() const noexcept { return bool(floor_callbacks_[0]) && bool(floor_callbacks_[1]); }
    // 返回地面特效类别；并非音频播放。AMF事件与特效提交将在场景层接入。
    unsigned ClassifyFloor(unsigned foot, std::string_view path) const;
private:
    ControllerState state_;
    std::unique_ptr<Actor> dummy_;
    std::array<std::function<unsigned(std::string_view)>,2> floor_callbacks_;
};
void WriteControllerReport(std::ostream& output, const ActorController& controller);
} // namespace pal4::portable
