#include "pal4/game_resource_manager.h"

namespace pal4 {

void GameResourceManager::FinalizeClumpLoad(IGameResourcePoseRuntime& runtime) {
    const auto frame = runtime.ClumpFrame(words[5]);
    runtime.FrameSetIdentity(frame);
    runtime.FrameRotate(frame, 0x84b87cu, 0.0f, 2);
    runtime.FrameRotate(frame, 0x84b870u, 360.0f, 2);
    runtime.FrameRotate(frame, 0x84b888u, 360.0f, 2);
    runtime.FrameUpdateObjects(frame);
    words[9] = 1;
}

}  // namespace pal4
