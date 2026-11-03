#include "pal4/game_systems_shutdown.h"

namespace pal4 {

std::int32_t GameSystemsShutdown::Shutdown(
    IGameSystemsShutdownRuntime& runtime) const {
    runtime.ShutdownRenderSystem();
    const std::uint32_t logger = runtime.InitializeLogger();
    return runtime.AppendLog(
        logger,
        "FILE:  [%s]\nLINE:  [%d]\nINFO:  Terminate3D success\n\n\n",
        R"(D:\\VcProject\\Pal4\\project\\Sourcecode\\RwCasing\\rwc_Initialize.cpp)",
        0x493u);
}

}  // namespace pal4
