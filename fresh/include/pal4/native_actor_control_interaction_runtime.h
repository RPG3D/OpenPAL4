#pragma once

#include "pal4/native_npc_closest_selection.h"
#include "pal4/native_gob_closest_selection.h"
#include "pal4/pal_actor_control_interaction.h"

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

class InputManager;
class NativeArenaNpcSceneLoader;
class NativeBaseObjectMemory;
class NativeGameArenaManagerOwner;
class NativeMinimapManagerOwner;
class NativeNewGameScriptSession;
class NativeSceneEvfAssets;
class NativeSceneGobAssets;
class INativeInteractionCursorService;

class INativeGameStateTransition {
public:
    virtual ~INativeGameStateTransition() = default;
    [[nodiscard]] virtual std::uint32_t Initialize(
        std::uint32_t state, std::uint32_t argument) = 0;
};

// Binds ActorControl's NPC, GOB and EVF selection to live scene owners.
class NativeActorControlInteractionRuntime final
    : public IActorControlInteractionRuntime {
public:
    NativeActorControlInteractionRuntime(
        InputManager& input, std::uint32_t window,
        NativeArenaNpcSceneLoader& npcs,
        NativeGameArenaManagerOwner& arena,
        NativeNewGameScriptSession& scripts,
        NativeBaseObjectMemory& memory,
        NativeSceneEvfAssets* effects = nullptr,
        NativeSceneGobAssets* objects = nullptr,
        NativeMinimapManagerOwner* minimap = nullptr,
        INativeGameStateTransition* states = nullptr,
        INativeInteractionCursorService* cursors = nullptr);
    void SetCursorOverride(std::optional<ActorInteractionPoint> point) noexcept {
        cursor_override_ = point;
    }

    ActorInteractionInput Input() override;
    ActorInteractionPoint CursorClient() override;
    ActorInteractionRay PixelRay(std::uint32_t camera_internal,
                                  ActorInteractionPoint point) override;
    std::uint32_t NpcByRay(const ActorInteractionRay& ray) override;
    std::uint32_t NpcByPosition(std::uint32_t actor_position) override;
    std::uint32_t DialogByRay(const ActorInteractionRay& ray) override;
    std::uint32_t DialogByPosition(std::uint32_t actor_position,
                                   float actor_property) override;
    bool DialogEnabled(std::uint32_t dialog) override;
    std::int32_t EventCode(std::uint32_t dialog) override;
    bool DialogInRange(std::uint32_t dialog,
                       std::uint32_t actor_position,
                       float actor_property) override;
    std::uint32_t EffectByRay(std::uint32_t camera_internal,
                              ActorInteractionPoint point) override;
    std::vector<std::uint32_t> EffectCandidates() override;
    ActorControlEffectPoint EffectVertex(std::uint32_t effect,std::int32_t index) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    float ReadFloat(std::uint32_t address) override;
    double ActorProperty(std::uint32_t actor) override;
    void ExecuteScript(std::uint32_t script) override;
    void AttachDialog(std::uint32_t dialog,
                      std::uint32_t actor_position) override;
    // Object virtual +0x50, also used by giGOMTouch without selection checks.
    void TouchObject(std::uint32_t dialog, std::uint32_t actor_position);
    void TriggerEffect(std::uint32_t effect_action) override;
    void SetCursor(std::string_view resource) override;

private:
    InputManager& input_;
    std::uint32_t window_;
    NativeNpcClosestSelection selection_;
    NativeGameArenaManagerOwner& arena_;
    NativeNewGameScriptSession& scripts_;
    NativeBaseObjectMemory& memory_;
    NativeSceneEvfAssets* effects_;
    NativeSceneGobAssets* objects_;
    NativeMinimapManagerOwner* minimap_;
    INativeGameStateTransition* states_;
    INativeInteractionCursorService* cursors_;
    std::optional<NativeGobClosestSelection> gob_selection_;
    std::optional<ActorInteractionPoint> cursor_override_;
};

}  // namespace pal4
