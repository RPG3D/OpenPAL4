#pragma once

#include "pal4/base_object_lifecycle.h"

#include <cstdint>

namespace pal4 {

class NativeArenaCollisionTree;

// Owns the scene bindings made by scriptOnAdd for the current player.
class NativeArenaActivePlayerSceneBinding final {
public:
    NativeArenaActivePlayerSceneBinding(
        BaseObjectState& player, std::uint32_t arena,
        NativeArenaCollisionTree& collision);
    ~NativeArenaActivePlayerSceneBinding();
    NativeArenaActivePlayerSceneBinding(
        const NativeArenaActivePlayerSceneBinding&) = delete;
    NativeArenaActivePlayerSceneBinding& operator=(
        const NativeArenaActivePlayerSceneBinding&) = delete;

    void Enter();
    void EnterWithPreviousState(std::uint32_t previous_state_flag);
    void Release();
    // Synchronize ownership after the real script callbacks have completed.
    // Foreign player slots are deliberately ignored by this current-role owner.
    void ScriptMembershipChanged(std::uint32_t player, bool added) noexcept;
    [[nodiscard]] bool Entered() const noexcept { return entered_; }

private:
    class Runtime;

    BaseObjectState& player_;
    std::uint32_t arena_;
    NativeArenaCollisionTree& collision_;
    std::uint32_t previous_state_flag_ = 0u;
    std::uint32_t previous_restart_flag_ = 0u;
    bool state_changed_ = false;
    bool restart_changed_ = false;
    bool world_attached_ = false;
    bool atomics_processed_ = false;
    bool collision_bound_ = false;
    bool entered_ = false;
};

}  // namespace pal4
