#include "pal4/native_script_opening_wait.h"

#include "pal4/native_script_flash_runtime.h"
#include "pal4/native_script_movie_player.h"
#include "pal4/native_script_timed_wait_runtime.h"
#include "pal4/native_script_talk_runtime.h"

#include <utility>
#include <cmath>
#include <stdexcept>

namespace pal4 {

void NativeScriptOpeningWait::UnbindSceneServices() noexcept {
    scene_action_ = nullptr;
    scene_camera_ = nullptr;
    scene_turn_ = nullptr;
    scene_walk_ = nullptr;
    scene_blend_ = nullptr;
    monster_blend_ = nullptr;
    object_effects_ = nullptr;
    scene_tutorial_ = nullptr;
    scene_poetry_ = nullptr;
    scene_trade_ = nullptr;
    scene_quest_ = nullptr;
    scene_inn_ = nullptr;
    scene_common_ = nullptr;
    scene_select_ = nullptr;
    scene_puzzle_ = nullptr;
    world_map_ = nullptr;
    timed_wait_ = nullptr;
    scene_dialog_ = nullptr;
    movie_stop_pending_ = false;
}

bool NativeScriptOpeningWait::BlocksScript() const {
    return flash_.BlocksScript() || movie_.Waiting() || movie_.CompletionPending() || movie_stop_pending_ ||
           (scene_action_ && scene_action_->BlocksScript()) ||
           (scene_camera_ && scene_camera_->BlocksScript()) ||
           (scene_turn_ && scene_turn_->BlocksScript()) ||
           (scene_walk_ && scene_walk_->BlocksScript()) ||
           (scene_blend_ && scene_blend_->BlocksScript()) ||
           (monster_blend_ && monster_blend_->BlocksScript()) ||
           (object_effects_ && object_effects_->BlocksScript()) ||
           (scene_tutorial_ && scene_tutorial_->BlocksScript()) ||
           (scene_poetry_ && scene_poetry_->BlocksScript()) ||
           (scene_trade_ && scene_trade_->BlocksScript()) ||
           (scene_quest_ && scene_quest_->BlocksScript()) ||
           (scene_inn_ && scene_inn_->BlocksScript()) ||
           (scene_common_ && scene_common_->BlocksScript()) ||
           (scene_select_ && scene_select_->BlocksScript()) ||
           (scene_puzzle_ && scene_puzzle_->BlocksScript()) ||
           (world_map_ && world_map_->BlocksScript()) ||
           (timed_wait_ && timed_wait_->Waiting()) ||
           (scene_dialog_ && scene_dialog_->Waiting());
}

bool NativeScriptOpeningWait::Tick(const float elapsed) {
    return TickServices(elapsed, true);
}

bool NativeScriptOpeningWait::HandlerRuntime::Tick(float elapsed) {
    return owner_.TickServices(elapsed, false);
}

bool NativeScriptOpeningWait::TickServices(float elapsed, bool advance_screen) {
    if (!std::isfinite(elapsed) || elapsed < 0.0f)
        throw std::invalid_argument("PAL4 opening handler time is invalid");
    const bool movie_stopped = std::exchange(movie_stop_pending_, false);
    const bool flash_closed = advance_screen ? flash_.Tick(elapsed) : flash_.ProcessWaitHandlers();
    // State-owned render updates decode/draw Bink. Handler-only mode only
    // consumes completion; combined legacy mode still advances the decoder.
    if (advance_screen) (void)movie_.Tick(true);
    const bool movie_closed = movie_.TakeCompletion();
    const bool action_closed = scene_action_ && scene_action_->Tick(elapsed);
    const bool camera_closed = scene_camera_ && scene_camera_->Tick(elapsed);
    const bool turn_closed = scene_turn_ && scene_turn_->Tick(elapsed);
    const bool walk_closed = scene_walk_ && scene_walk_->Tick(elapsed);
    const bool blend_closed = scene_blend_ && scene_blend_->Tick(elapsed);
    const bool monster_blend_closed = monster_blend_ && monster_blend_->Tick(elapsed);
    const bool object_closed = object_effects_ && object_effects_->Tick(elapsed);
    const bool tutorial_closed = scene_tutorial_ &&
                                 scene_tutorial_->Tick(elapsed);
    const bool trade_closed = scene_trade_ && scene_trade_->Tick(elapsed);
    const bool poetry_closed = scene_poetry_ && scene_poetry_->Tick(elapsed);
    const bool quest_closed = scene_quest_ && scene_quest_->Tick(elapsed);
    const bool inn_closed = scene_inn_ && scene_inn_->Tick(elapsed);
    const bool common_closed = scene_common_ && scene_common_->Tick(elapsed);
    const bool select_closed = scene_select_ && scene_select_->Tick(elapsed);
    const bool puzzle_closed = scene_puzzle_ && scene_puzzle_->Tick(elapsed);
    const bool world_map_closed = world_map_ && world_map_->Tick(elapsed);
    const bool timed_was_waiting = timed_wait_ && timed_wait_->Waiting();
    const bool timed_closed = timed_wait_ && timed_wait_->Tick(elapsed);
    if (timed_was_waiting) return timed_closed;
    const bool dialog_was_waiting = scene_dialog_ && scene_dialog_->Waiting();
    const bool dialog_closed = scene_dialog_ && scene_dialog_->Tick(elapsed);
    if (dialog_was_waiting) return dialog_closed;
    return flash_closed || movie_closed || movie_stopped || action_closed ||
           camera_closed || turn_closed || walk_closed || blend_closed || monster_blend_closed || object_closed || timed_closed ||
           tutorial_closed || poetry_closed || trade_closed || quest_closed || inn_closed || common_closed || select_closed || puzzle_closed || world_map_closed || dialog_closed;
}

bool NativeScriptOpeningWait::OnKeyDownScanCode(
    const std::uint32_t scan_code) {
    if (scan_code != 1u || !movie_.Waiting() || !movie_.Stop())
        return false;
    movie_stop_pending_ = true;
    return true;
}

}  // namespace pal4
