#pragma once

#include "pal4/native_animation_event_queue_runtime.h"
#include "pal4/native_base_object_member_runtime.h"
#include "pal4/native_player_manager_vector_runtime.h"
#include "pal4/native_player_slot_array_runtime.h"
#include "pal4/native_player_slot_base_runtime.h"
#include "pal4/native_player_slot_script_runtime.h"

#include <cstdint>

namespace pal4 {

// Owns the four x86 player slots and their native construction chain.
// Scene, animation and slot activity remain services of the game host.
class NativePlayerManagerSession final {
public:
    NativePlayerManagerSession(
        NativeBaseObjectMemory& memory, IRwDictSchemaRuntime& schema,
        IGameObjectEmbeddedAnimationRuntime& animation,
        IAnimationEventQueueActivityRuntime& queue_activity,
        IPlayerManagerSlotRuntime& slot_host,
        IBaseObjectActivityRuntime& base_activity);
    NativePlayerManagerSession(const NativePlayerManagerSession&) = delete;
    NativePlayerManagerSession& operator=(const NativePlayerManagerSession&) =
        delete;
    ~NativePlayerManagerSession();

    // Pins the four slots during synchronous scene-frame callbacks.
    class FrameLease final {
    public:
        FrameLease(const FrameLease&)=delete;
        FrameLease& operator=(const FrameLease&)=delete;
        ~FrameLease();
    private:
        friend class NativePlayerManagerSession;
        explicit FrameLease(NativePlayerManagerSession&);
        NativePlayerManagerSession& owner_;
    };
    [[nodiscard]] FrameLease BorrowFrame();
    [[nodiscard]] std::uint32_t Construct();
    void Destroy();
    [[nodiscard]] bool Constructed() const noexcept { return constructed_; }
    [[nodiscard]] std::uint32_t Address() noexcept;
    [[nodiscard]] PlayerManagerObjectState& State() noexcept { return state_; }
    [[nodiscard]] NativePlayerSlotCopyRuntime& CopyRuntime() noexcept {
        return copy_;
    }
    [[nodiscard]] NativePlayerSlotBaseRuntime& SlotBaseRuntime() noexcept {
        return bases_;
    }
    [[nodiscard]] IAnimationEventQueueRuntime& QueueRuntime() noexcept {
        return queue_;
    }

private:
    PlayerManagerObjectState state_;
    NativeBaseObjectMemberRuntime members_;
    NativeAnimationEventQueueRuntime queue_;
    NativePlayerSlotBaseRuntime bases_;
    NativePlayerSlotScriptRuntime scripts_;
    NativePlayerSlotCopyRuntime copy_;
    NativePlayerSlotArrayRuntime arrays_;
    NativePlayerManagerVectorRuntime vector_;
    bool constructed_{};
    bool frame_active_{};
};

}  // namespace pal4
