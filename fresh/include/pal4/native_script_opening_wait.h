#pragma once

#include "pal4/native_script_frame_runner.h"

#include <cstdint>

namespace pal4 {

class NativeScriptFlashRuntime;
class NativeScriptMoviePlayer;
class NativeScriptTimedWaitRuntime;
class NativeScriptTalkRuntime;

// Advances opening and scene waits for one script context.
class NativeScriptOpeningWait final : public INativeScriptFrameWaitRuntime {
public:
    NativeScriptOpeningWait(NativeScriptFlashRuntime& flash,
                            NativeScriptMoviePlayer& movie)
        : flash_(flash), movie_(movie) {}
    NativeScriptOpeningWait(const NativeScriptOpeningWait&) = delete;
    NativeScriptOpeningWait& operator=(const NativeScriptOpeningWait&) = delete;

    // Combined scene tick advances the shared fade. Video-state updates use
    // Handlers(), which observes flash/movie completion without advancing fade
    // or decoding video; the state renderer owns those operations.
    [[nodiscard]] bool Tick(float elapsed) override;
    class HandlerRuntime final : public INativeScriptHandlerWaitRuntime {
    public:
        explicit HandlerRuntime(NativeScriptOpeningWait& owner) : owner_(owner) {}
        bool BlocksScript() const override { return owner_.BlocksScript(); }
        bool Tick(float elapsed) override;
    private:
        NativeScriptOpeningWait& owner_;
    };
    [[nodiscard]] INativeScriptHandlerWaitRuntime& Handlers() { return handler_runtime_; }
    [[nodiscard]] bool BlocksScript() const override;
    [[nodiscard]] bool OnKeyDownScanCode(std::uint32_t scan_code);
    void BindSceneAction(INativeScriptFrameWaitRuntime& action) noexcept {
        scene_action_ = &action;
    }
    void BindSceneCamera(INativeScriptFrameWaitRuntime& camera) noexcept {
        scene_camera_ = &camera;
    }
    void BindSceneTurn(INativeScriptFrameWaitRuntime& turn) noexcept {
        scene_turn_ = &turn;
    }
    void BindSceneWalk(INativeScriptFrameWaitRuntime& walk) noexcept {
        scene_walk_ = &walk;
    }
    void BindSceneBlend(INativeScriptFrameWaitRuntime& blend) noexcept { scene_blend_=&blend; }
    void BindMonsterBlend(INativeScriptFrameWaitRuntime& blend) noexcept { monster_blend_=&blend; }
    void BindObjectEffects(INativeScriptFrameWaitRuntime& effects) noexcept { object_effects_=&effects; }
    void BindSceneTutorial(INativeScriptFrameWaitRuntime& tutorial) noexcept {
        scene_tutorial_ = &tutorial;
    }
    void BindSceneTrade(INativeScriptFrameWaitRuntime& trade) noexcept { scene_trade_=&trade; }
    void BindScenePoetry(INativeScriptFrameWaitRuntime& poetry) noexcept { scene_poetry_=&poetry; }
    void BindSceneQuest(INativeScriptFrameWaitRuntime& quest) noexcept { scene_quest_=&quest; }
    void BindSceneInn(INativeScriptFrameWaitRuntime& inn) noexcept { scene_inn_=&inn; }
    void BindSceneCommonDialog(INativeScriptFrameWaitRuntime& dialog) noexcept { scene_common_=&dialog; }
    void BindSceneSelectDialog(INativeScriptFrameWaitRuntime& dialog) noexcept { scene_select_=&dialog; }
    void BindScenePuzzle(INativeScriptFrameWaitRuntime& puzzle) noexcept { scene_puzzle_=&puzzle; }
    void BindWorldMap(INativeScriptFrameWaitRuntime& map) noexcept { world_map_=&map; }
    void BindTimedWait(NativeScriptTimedWaitRuntime& timed_wait) noexcept {
        timed_wait_ = &timed_wait;
    }
    void BindScenarioDialog(NativeScriptTalkRuntime& dialog) noexcept {
        scene_dialog_ = &dialog;
    }
    void UnbindSceneServices() noexcept;

private:
    bool TickServices(float elapsed, bool advance_screen);
    NativeScriptFlashRuntime& flash_;
    NativeScriptMoviePlayer& movie_;
    INativeScriptFrameWaitRuntime* scene_action_{};
    INativeScriptFrameWaitRuntime* scene_camera_{};
    INativeScriptFrameWaitRuntime* scene_turn_{};
    INativeScriptFrameWaitRuntime* scene_walk_{};
    INativeScriptFrameWaitRuntime* scene_blend_{};
    INativeScriptFrameWaitRuntime* monster_blend_{};
    INativeScriptFrameWaitRuntime* object_effects_{};
    INativeScriptFrameWaitRuntime* scene_tutorial_{};
    INativeScriptFrameWaitRuntime* scene_poetry_{};
    INativeScriptFrameWaitRuntime* scene_trade_{};
    INativeScriptFrameWaitRuntime* scene_quest_{};
    INativeScriptFrameWaitRuntime* scene_inn_{};
    INativeScriptFrameWaitRuntime* scene_common_{};
    INativeScriptFrameWaitRuntime* scene_select_{};
    INativeScriptFrameWaitRuntime* scene_puzzle_{};
    INativeScriptFrameWaitRuntime* world_map_{};
    NativeScriptTimedWaitRuntime* timed_wait_{};
    NativeScriptTalkRuntime* scene_dialog_{};
    bool movie_stop_pending_{};
    HandlerRuntime handler_runtime_{*this};
};

}  // namespace pal4
