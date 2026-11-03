#include "pal4/pal4_game_system_bootstrap.h"

#include <array>
#include <utility>

namespace pal4 {

std::int32_t Pal4GameSystemBootstrap::Initialize(
    IPal4GameSystemBootstrapRuntime& runtime) {
    runtime.SetGlobalFrameCallback(kGlobalFrameCallback, kRenderFrame);
    constexpr std::array callbacks{
        std::pair{0x30u, kShutdown},
        std::pair{0x34u, kShutdown},
        std::pair{0x38u, kCheckAndShutdown},
        std::pair{0x3cu, kShutdown},
        std::pair{0x40u, kShutdown},
        std::pair{0x44u, kShutdown},
        std::pair{0x48u, kShutdown},
        std::pair{0x54u, kShutdown},
        std::pair{0x58u, kShutdown},
        std::pair{0x5cu, kShutdown},
        std::pair{0x60u, kCleanupResources},
        std::pair{0x64u, kCleanupResourcesAlt},
    };
    for (const auto [offset, callback] : callbacks)
        runtime.SetRenderCallback(runtime.RenderSystem(), offset, callback);

    runtime.InitializeGameSystems(runtime.GameInstance());
    const auto effects = runtime.RegisterArenaEffectManager();
    runtime.SetUpdateHandler(effects, kRenderFrameWithEffects);
    const auto active_camera = runtime.ActiveCameraObject(runtime.CameraSubsystem());
    const auto game = runtime.GameInstance();
    runtime.InitializeTexture(game + 0x178u,
                              runtime.CameraContainer(active_camera), 256u, 256u);
    return runtime.InitializeStateAndUi(runtime.GameInstance(), 0u, 0u);
}

}  // namespace pal4
