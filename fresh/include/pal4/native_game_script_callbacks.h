#pragma once

#include "pal4/native_angelscript_engine.h"
#include "pal4/script_builtin_handlers.h"
#include "pal4/script_execution_context.h"

#include <array>
#include <string_view>

namespace pal4 {

class NativeScriptRandomTalkPool;
class IScriptCacheContextRuntime;
class IScriptCommandRuntime;
class ScriptFlashEffect;
class IScriptMusicTransportRuntime;
class IScriptMoviePlayer;
class IScriptMovieFlagRuntime;
class IScriptNewGameRuntime;
class IGameArenaLoadRuntime;
class IScriptObjectBlendRuntime;
class IScriptMonsterBlendRuntime;
class IScriptObjectActionRuntime;
class IScriptObjectTimedMoveRuntime;
class IScriptObjectTimedScaleRuntime;
class IScriptCurrentPlayerTimedMoveRuntime;
class BackgroundMusicPlayer;
class IBackgroundMusicRuntime;
class IScriptBgmAreaName;
class IScriptCameraCallbackRuntime;
class IScriptPlayerActionsRuntime;
class IScriptCurrentPlayerVisibilityRuntime;
class IScriptPlayerLockRuntime;
class IScriptAlwaysJumpRuntime;
class IScriptPlayerPositionRuntime;
class IScriptRandomPlayerPositionRuntime;
class IScriptRandomIntegerRuntime;
class IScriptMinimapLevelRuntime;
class IScriptPlayerRotationRuntime;
class IGameResourceManagerRuntime;
class IGameArenaReadyRuntime;
class IScriptSystemUiRuntime;
class IScriptPlayerActionRuntime;
class IScriptPlayerEquipmentRuntime;
class IScriptCameraSequenceRuntime;
class IScriptWorldMapWaitRuntime;
class IWorldMapButtonStateRuntime;
class IWorldMapExternalButtonRuntime;
class IScriptSound2dRuntime;
class IScriptWaitHandlerRuntime;
class IScriptTalkHandlerRuntime;
class IScriptScenarioPortraitRuntime;
class IScriptActionHoldRuntime;
class IScriptPlayerTurnRuntime;
class IScriptObjectVisibilityRuntime;
class IScriptObjectTouchRuntime;
class IScriptEventVolumeVisibilityRuntime;
class IScriptObjectResearchRuntime;
class IScriptNpcVisibilityRuntime;
class IScriptNpcBehaviorRuntime;
class IScriptMonsterVisibilityRuntime;
class IMonsterPursuitCommandRuntime;
class IScriptNightTimeQueryRuntime;
class IScriptCameraDistanceRuntime;
class IScriptCameraModeRuntime;
class ScriptPlayerWalk;
class IScriptTutorialWaitRuntime;
class IScriptPoetryWaitRuntime;
class IScriptTradeWaitRuntime;
class IScriptPuzzleRuntime;
class IScriptPuzzleResultRuntime;
class IScriptQuestDialogRuntime;
class IScriptInnDialogRuntime;
class IScriptDialogsRuntime;
class IScriptCommonDialogSelectionRuntime;
class NativeScriptSelectDialogRuntime;
class IScriptEffectRuntime;
class IScriptTimedStateRuntime;
class IScriptGobResetRuntime;
class IScriptWeatherRuntime;
class IScriptPetVisibilityRuntime;
class IScriptInventoryChangeRuntime;
class IScriptMoneyTransactionRuntime;
class IScriptInventoryQueryRuntime;
class IScriptHintDisplayRuntime;
class IScriptPrescriptionAdditionRuntime;
class IScriptPartyMembershipRuntime;
class IScriptPlayerGarbRuntime;
class IScriptPlayerStatusRuntime;
class IScriptPlayerTextureRuntime;
class IScriptNpcMaterialRuntime;
class IScriptShadowRuntime;
class IScriptPlayerFavorRuntime;
class IScriptCombatConfigRuntime;
class IScriptCombatStartRuntime;
class IScriptCombatTextRuntime;
class IScriptPartyRecoveryRuntime;
class IScriptPlayerRestRuntime;
class IScriptPartyMagicQueryRuntime;
class IScriptNpcHandEffectRuntime;
class IScriptPlayerGobAttachmentRuntime;
class IScriptPlayerBlendRuntime;
class IScriptPalTestResultRuntime;
class IScriptArenaSkillStateRuntime;
class IScriptUiCountdown;
class IScriptUiTimerSaveDataRuntime;
class IScriptCompletedHandlerQueue;
class IScriptPlayerSkillAcquisitionRuntime;
class IScriptForbiddenSkillRuntime;

struct NativeGameScriptServices {
    ScriptFlashEffect* flash{};
    IScriptMusicTransportRuntime* music{};
    IScriptMoviePlayer* movie{};
    IScriptMovieFlagRuntime* globals{};
    IScriptNewGameRuntime* new_game{};
    IGameArenaLoadRuntime* arena{};
    BackgroundMusicPlayer* background_music_player{};
    IBackgroundMusicRuntime* background_music_runtime{};
    // Borrows the current arena owner; queries +E0 for each callback.
    const IScriptBgmAreaName* background_music_area{};
    IScriptCameraCallbackRuntime* camera{};
    IScriptPlayerActionsRuntime* player_actions{};
    IScriptPlayerTextureRuntime* player_material{};
    IScriptNpcMaterialRuntime* npc_material{};
    IScriptShadowRuntime* shadow{};
    IScriptPlayerLockRuntime* player_lock{};
    IScriptAlwaysJumpRuntime* always_jump{};
    IScriptPlayerPositionRuntime* player_position{};
    IScriptRandomPlayerPositionRuntime* random_player_position{};
    // Null selects the public host CRT, with normal diagnostic tracing.
    IScriptRandomIntegerRuntime* random_integer{};
    IScriptCacheContextRuntime* cache_context{};
    IScriptMinimapLevelRuntime* minimap_level{};
    IScriptPlayerRotationRuntime* player_rotation{};
    IGameResourceManagerRuntime* entity_transforms{};
    IScriptCompletedHandlerQueue* completed_handlers{};
    IGameArenaReadyRuntime* arena_ready{};
    IScriptSystemUiRuntime* system_ui{};
    IScriptPlayerActionRuntime* player_action{};
    IScriptPlayerSkillAcquisitionRuntime* player_skill_acquisition{};
    IScriptForbiddenSkillRuntime* forbidden_skill{};
    IScriptPlayerEquipmentRuntime* player_equipment{};
    IScriptCameraSequenceRuntime* camera_sequences{};
    IScriptWorldMapWaitRuntime* world_map{};
    IWorldMapButtonStateRuntime* world_map_buttons{};
    IWorldMapExternalButtonRuntime* world_map_external{};
    IScriptSound2dRuntime* sound_2d{};
    IScriptWaitHandlerRuntime* wait{};
    IScriptTalkHandlerRuntime* talk{};
    IScriptScenarioPortraitRuntime* portrait{};
    IScriptActionHoldRuntime* action_hold{};
    IScriptPlayerTurnRuntime* player_turn{};
    IScriptObjectVisibilityRuntime* object_visibility{};
    IScriptObjectTouchRuntime* object_touch{};
    IScriptObjectBlendRuntime* object_blend{};
    IScriptObjectActionRuntime* object_action{};
    IScriptObjectTimedMoveRuntime* object_move{};
    IScriptObjectTimedScaleRuntime* object_scale{};
    IScriptCurrentPlayerTimedMoveRuntime* current_player_move{};
    IScriptNightTimeQueryRuntime* night_time{};
    IScriptNpcVisibilityRuntime* npc_visibility{};
    IScriptNpcBehaviorRuntime* npc_behavior{};
    IScriptMonsterVisibilityRuntime* monster_visibility{};
    IScriptMonsterBlendRuntime* monster_blend{};
    IMonsterPursuitCommandRuntime* monster_pursuit{};
    IScriptCameraDistanceRuntime* camera_distance{};
    ScriptPlayerWalk* player_walk{};
    IScriptCameraModeRuntime* camera_mode{};
    IScriptTutorialWaitRuntime* tutorial{};
    IScriptPoetryWaitRuntime* poetry{};
    IScriptTradeWaitRuntime* trade{};
    IScriptPuzzleRuntime* puzzle{};
    IScriptPuzzleResultRuntime* puzzle_result{};
    IScriptQuestDialogRuntime* quest{};
    IScriptInnDialogRuntime* inn{};
    IScriptDialogsRuntime* dialogs{};
    IScriptCommonDialogSelectionRuntime* common_selection{};
    NativeScriptSelectDialogRuntime* select_dialog{};
    IScriptEventVolumeVisibilityRuntime* event_volume_visibility{};
    IScriptObjectResearchRuntime* object_research{};
    IScriptEffectRuntime* effects{};
    IScriptTimedStateRuntime* timer{};
    IScriptGobResetRuntime* gob_reset{};
    IScriptWeatherRuntime* weather{};
    IScriptPetVisibilityRuntime* pet_visibility{};
    IScriptCommandRuntime* commands{};
    NativeScriptRandomTalkPool* random_talk{};
    IScriptInventoryChangeRuntime* inventory_change{};
    IScriptMoneyTransactionRuntime* money_transaction{};
    IScriptInventoryQueryRuntime* inventory_query{};
    IScriptHintDisplayRuntime* hint_display{};
    IScriptPrescriptionAdditionRuntime* prescription_addition{};
    IScriptPartyMembershipRuntime* party_membership{};
    IScriptPlayerGarbRuntime* player_garb{};
    IScriptPlayerStatusRuntime* player_status{};
    IScriptPlayerFavorRuntime* player_favor{};
    IScriptCombatConfigRuntime* combat_config{};
    IScriptCombatStartRuntime* combat_start{};
    IScriptCombatTextRuntime* combat_text{};
    IScriptPartyRecoveryRuntime* party_recovery{};
    IScriptPlayerRestRuntime* player_rest{};
    IScriptPalTestResultRuntime* pal_test{};
    IScriptArenaSkillStateRuntime* arena_skill_state{};
    IScriptUiCountdown* ui_countdown{};
    IScriptUiTimerSaveDataRuntime* ui_timer_save_data{};
    IScriptPartyMagicQueryRuntime* party_magic_query{};
    IScriptNpcHandEffectRuntime* npc_hand_effect{};
    IScriptPlayerGobAttachmentRuntime* player_gob_attachment{};
    IScriptPlayerBlendRuntime* npc_blend{};
    IScriptPlayerBlendRuntime* player_blend{};
    IScriptCurrentPlayerVisibilityRuntime* current_player_visibility{};
};

// Services and their underlying game state must outlive script execution.
class NativeGameScriptCallbacks final : public IGameScriptCallbackRuntime,
                                        private IScriptImmediateModeRuntime {
public:
    explicit NativeGameScriptCallbacks(NativeGameScriptServices services)
        : services_(services) { ResetImmediateMode(); }

    void SetServices(NativeGameScriptServices services) noexcept {
        services_ = services;
    }
    void SetObjectVisibility(
        IScriptObjectVisibilityRuntime* runtime) noexcept {
        services_.object_visibility = runtime;
    }
    void ResetImmediateMode() noexcept;
    [[nodiscard]] IScriptCompletedHandlerQueue* CompletedHandlers() const noexcept {
        return services_.completed_handlers;
    }
    [[nodiscard]] std::uint32_t ImmediateMode() const noexcept {
        return immediate_record_.word_268;
    }
    void Invoke(int system_id, asIScriptGeneric& arguments) override;

private:
    void InvokeImplementation(int system_id, asIScriptGeneric& arguments);
    [[nodiscard]] std::uint32_t ScriptInstance() override;
    [[nodiscard]] std::uint32_t SetDebuggerMode(
        std::uint32_t debugger, std::uint32_t mode) override;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;

    NativeGameScriptServices services_;
    // Mirrors palScriptExe's debugger slot and active 276-byte module record.
    std::array<std::uint32_t, 25> immediate_instance_{};
    ScriptStackModuleRecord immediate_record_{};
};

}  // namespace pal4
