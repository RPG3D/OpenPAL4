#pragma once

#include "pal4/animation_string_table.h"
#include "pal4/base_object_lifecycle.h"
#include "pal4/game_object_animation_event_data.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <utility>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

class NativeAnimatedActorAsset;
class NativeBaseObjectMemory;
class IGameObjectEmbeddedAnimationRuntime;
struct GameObjectEmbeddedAnimationState;

// Owns embedded animation records and borrows original actor clips.
class NativeActorAnimationStateBinding final {
public:
    NativeActorAnimationStateBinding(
        BaseObjectState& object, const NativeAnimatedActorAsset& actor,
        NativeBaseObjectMemory& memory,
        IGameObjectEmbeddedAnimationRuntime& lifecycle,
        std::filesystem::path game_root, std::string model);
    ~NativeActorAnimationStateBinding();
    NativeActorAnimationStateBinding(
        const NativeActorAnimationStateBinding&) = delete;
    NativeActorAnimationStateBinding& operator=(
        const NativeActorAnimationStateBinding&) = delete;

    void Bind();
    // After Release and replacement of the borrowed actor's resources, reuse
    // this binding object so script services retain valid references.
    void BindModel(std::string model);
    // Takes ownership of counted clip records installed by NativeBorrowedActorBinding.
    // After validation, allocation failure releases the adopted records as well.
    void AdoptBorrowedRecords();
    [[nodiscard]] bool Select(std::string_view action);
    [[nodiscard]] bool SelectBlended(std::string_view action, float blend_time);
    // Actor virtual wrappers443FF0/444080 compare names before invoking the
    // primitive selectors above. Equal names preserve time, blend and flags.
    [[nodiscard]] bool SelectActorAction(std::string_view action,
        bool blended = false, float blend_time = 0.0f);
    [[nodiscard]] bool AdvanceBlend(float elapsed);
    [[nodiscard]] bool AdvanceFrame(float elapsed);
    [[nodiscard]] bool SetTime(float time);
    [[nodiscard]] float Time();
    [[nodiscard]] std::uint32_t Completed();
    std::uint32_t SetRestart(std::uint32_t flag);
    [[nodiscard]] std::string_view CurrentRecordName();
    [[nodiscard]] std::string_view SelectedListenerName();
    [[nodiscard]] std::string_view ControllerMemberName(std::uint32_t member) const;
    [[nodiscard]] bool AddControllerMember(std::uint32_t member);
    [[nodiscard]] bool RemoveControllerMember(std::string_view name);
    // Scene services must unbind before their owners are released.
    void BindDisplayReset(std::function<void(std::uint32_t)> reset) {
        display_reset_ = std::move(reset);
    }
    void BindFloorSound(std::function<void(std::uint32_t)> callback) { floor_sound_ = std::move(callback); }
    [[nodiscard]] bool HasFloorSound() const noexcept { return bool(floor_sound_); }
    void DispatchFloorSound(std::uint32_t node);
    [[nodiscard]] bool HasDisplayReset() const noexcept { return bool(display_reset_); }
    void DispatchDisplayReset(std::uint32_t member);
    [[nodiscard]] bool HoldEventSeen() const noexcept { return hold_event_seen_; }
    void DispatchHoldEvent(GameObjectEmbeddedAnimationState& state);
    void ResetRecordEvents(std::uint32_t record);
    // Actual model-specific default listener; distinct from scene Hold/Rpe.
    void BindEventFallback(std::function<void(std::uint32_t)> callback) {event_fallback_=std::move(callback);}
    [[nodiscard]] bool HasEventFallback() const noexcept {return bool(event_fallback_);}
    void DispatchEventFallback(std::uint32_t event) {event_fallback_(event);}
    // Retained with the clip's lazy AMF data; invalid action names return empty.
    [[nodiscard]] std::span<const AnimationStringTableData> AnimationParameters(
        std::string_view action);
    void Release();
    [[nodiscard]] bool Bound() const noexcept { return allocation_ != 0u; }
    [[nodiscard]] bool Matches(const BaseObjectState& object,
        const NativeAnimatedActorAsset& actor) const noexcept {
        return &object_ == &object && &actor_ == &actor;
    }

private:
    class Runtime;

    BaseObjectState& object_;
    const NativeAnimatedActorAsset& actor_;
    NativeBaseObjectMemory& memory_;
    IGameObjectEmbeddedAnimationRuntime& lifecycle_;
    std::uint32_t allocation_ = 0u;
    std::uint32_t count_ = 0u;
    std::uint32_t collection_allocation_ = 0u;
    std::filesystem::path game_root_;
    std::string model_;
    std::vector<std::vector<AnimationEventData>> record_events_;
    std::vector<std::vector<AnimationStringTableData>> record_parameters_;
    std::vector<bool> record_events_loaded_;
    bool hold_event_seen_ = false;
    std::function<void(std::uint32_t)> display_reset_, floor_sound_;
    std::function<void(std::uint32_t)> event_fallback_;

    void EnsureRecordEvents(std::string_view action);
    void InitializeActionTable();
    void InitializeBlendSnapshots();
};

}  // namespace pal4
