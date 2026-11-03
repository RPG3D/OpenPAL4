#pragma once
#include "pal4/native_script_frame_wait_runtime.h"
#include "pal4/script_completed_handler_queue.h"
#include "pal4/script_object_blend.h"
#include "pal4/script_object_action.h"
#include "pal4/script_object_timed_move.h"
#include "pal4/script_object_timed_scale.h"
#include "pal4/script_current_player_timed_move.h"
#include <memory>

class asIScriptContext;
namespace pal4 {
class NativeSceneGobAssets;
class NativeScriptObjectVisibilityRuntime;
class NativeScriptPlayerActionRuntime;
class IScriptPlayerPositionRuntime;

// One SDK context's ordered GOB effects and current-player timed moves.
// Clear before scene release; bound player services outlive this queue.
class NativeScriptObjectEffectsRuntime final : public INativeScriptFrameWaitRuntime,
                                             public IScriptCompletedHandlerQueue {
public:
    NativeScriptObjectEffectsRuntime(NativeSceneGobAssets&,
        NativeScriptObjectVisibilityRuntime&, asIScriptContext&);
    ~NativeScriptObjectEffectsRuntime();
    NativeScriptObjectEffectsRuntime(const NativeScriptObjectEffectsRuntime&) = delete;
    NativeScriptObjectEffectsRuntime& operator=(const NativeScriptObjectEffectsRuntime&) = delete;
    IScriptObjectBlendRuntime& Blend();
    IScriptObjectTimedMoveRuntime& Movement();
    IScriptObjectTimedScaleRuntime& Scale();
    IScriptObjectActionRuntime& Action();
    void BindCurrentPlayer(NativeScriptPlayerActionRuntime&, IScriptPlayerPositionRuntime&);
    IScriptCurrentPlayerTimedMoveRuntime& CurrentPlayerMovement();
    bool Tick(float elapsed) override;
    bool BlocksScript() const override;
    bool Waiting() const;
    void AppendCompletedHandler(std::uint32_t vtable, std::int32_t marker,
                                std::string_view name) override;
    void ClearHandlers();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
