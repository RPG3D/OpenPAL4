#include "pal4/game_systems_initializer.h"

#include <string_view>

namespace pal4 {
namespace {

constexpr std::string_view kSourceFile =
    R"(D:\\VcProject\\Pal4\\project\\Sourcecode\\RwCasing\\rwc_Initialize.cpp)";
constexpr std::string_view kMemoryError =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[RWCInitializeRW failed <MemoryFunctions error>]\n\n\n";
constexpr std::string_view kOpenError =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[RWCInitializeRW failed <RwEngineOpen error>]\n\n\n";
constexpr std::string_view kStartError =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[RWCInitializeRW failed <RwEngineStart error>]\n\n\n";
constexpr std::string_view kCharsetError =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[RWCInitializeRW failed <RtCharsetOpen error>]\n\n\n";
constexpr std::string_view kInputError =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[RWCInitializeRW failed <GameInput error>]\n\n\n";
constexpr std::string_view kSuccess =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  RWCInitializeRW success\n\n\n";

void Log(IGameConfigLogger& logger, const std::string_view format,
         const std::uint32_t line) {
    logger.AppendLog(logger.InitializeLogger(), format, kSourceFile, line);
}

}  // namespace

std::uint32_t GameSystemsInitializer::Initialize(
    IGameSystemsRuntime& runtime) {
    runtime.InitializeAllocator(kAllocatorTable);
    if (runtime.EngineInit(kAllocatorTable, 0u, kResourceArenaSize) == 0u) {
        Log(runtime, kMemoryError, 0x403u);
        return 0u;
    }
    if (runtime.AttachPlugins() == 0u ||
        runtime.BindInputCallback(0u, kKeyboardCallback) == 0u ||
        runtime.BindInputCallback(1u, kMouseCallback) == 0u) {
        return 0u;
    }
    if (runtime.OpenEngine(state_.Read(0u)) == 0u) {
        Log(runtime, kOpenError, 0x418u);
        runtime.ShutdownEngine();
        return 0u;
    }
    if (runtime.SelectDevice() == 0u) {
        runtime.CloseEngine();
        runtime.ShutdownEngine();
        return 0u;
    }
    runtime.SetRefreshRate(0u);
    runtime.EnableDialogBoxMode(1u);
    if (runtime.StartEngine() == 0u) {
        Log(runtime, kStartError, 0x43du);
        runtime.CloseEngine();
        runtime.ShutdownEngine();
        return 0u;
    }

    state_.Write(0x55cu, runtime.GetD3D9Caps());
    state_.Write(0x560u, runtime.GetCurrentD3DDevice());
    (void)runtime.InitializeImageHandlers();
    if (runtime.OpenCharset() == 0u) {
        Log(runtime, kCharsetError, 0x454u);
        runtime.StopEngine();
        runtime.CloseEngine();
        runtime.ShutdownEngine();
        return 0u;
    }
    if (runtime.OpenFileSystemManager(0xffffffffu) == 0u ||
        runtime.InitializeVirtualFileSystem() == 0u) {
        return 0u;
    }
    if (runtime.InitializeDirectInput(state_.Read(4u), state_.Read(0u),
                                      0u) == 0u) {
        Log(runtime, kInputError, 0x46cu);
    }
    runtime.SetInputParameters(0u, 0u, config_[0], config_[1]);
    runtime.SetInputScalar(0x3f800000u);
    Log(runtime, kSuccess, 0x475u);
    return 1u;
}

}  // namespace pal4
