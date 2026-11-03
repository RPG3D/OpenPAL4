#include "pal4/script_builtin_handlers.h"

namespace pal4 {
namespace {

double CameraProperty(const std::uint32_t offset,
                      IScriptCameraQueryRuntime& runtime) {
    const auto manager = runtime.InitializeCameraSubsystem();
    const auto camera = runtime.ActiveCameraInternalId(manager);
    return static_cast<double>(runtime.CameraFloat(camera + offset));
}

}  // namespace

std::uint32_t ScriptImmediateMode::Begin(
    IScriptImmediateModeRuntime& runtime) {
    const auto debugger = runtime.ScriptInstance() + 0x54u;
    return runtime.SetDebuggerMode(debugger, 1);
}

std::uint32_t ScriptImmediateMode::End(
    IScriptImmediateModeRuntime& runtime) {
    const auto debugger = runtime.ScriptInstance() + 0x54u;
    auto context = runtime.ReadWord(debugger + 12u);
    if (context != 0) {
        runtime.WriteWord(context + 0x10cu,
                          runtime.ReadWord(context + 0x110u));
        context = runtime.ReadWord(debugger + 12u);
        runtime.WriteWord(context + 0x110u, 0);
    }
    return context;
}

int ScriptNewGameHandler::Run(IScriptNewGameRuntime& runtime) {
    runtime.ResetAllPlayerStates(runtime.InitializePlayerManager());
    runtime.LoadAndProcessPlayerData(runtime.InitializePlayerManager());
    runtime.ShutdownActorControl(runtime.ActorControlInstance());
    return runtime.InitializeActorControl(runtime.ActorControlInstance());
}

int ScriptCameraHandler::AdjustYpr(
    const float yaw, const float pitch, const float roll, const int option,
    IScriptCameraRuntime& runtime) {
    const auto manager = runtime.InitializeCameraSubsystem();
    const auto camera = runtime.ActiveCameraObject(manager);
    return runtime.AdjustAngles(camera, yaw, pitch, roll,
                                static_cast<std::uint8_t>(option));
}

int ScriptCameraHandler::AdjustScalar(
    const float distance, const int option, IScriptCameraRuntime& runtime) {
    const auto manager = runtime.InitializeCameraSubsystem();
    const auto camera = runtime.ActiveCameraObject(manager);
    return runtime.AdjustDistance(camera, distance,
                                  static_cast<std::uint8_t>(option));
}

int ScriptCameraHandler::AdjustYprd(
    const float yaw, const float pitch, const float roll,
    const float distance, const int option, IScriptCameraRuntime& runtime) {
    const auto manager = runtime.InitializeCameraSubsystem();
    const auto camera = runtime.ActiveCameraObject(manager);
    return runtime.AdjustAnglesAndDistance(
        camera, yaw, pitch, roll, distance,
        static_cast<std::uint8_t>(option));
}

double ScriptCameraHandler::Distance(IScriptCameraQueryRuntime& runtime) {
    return CameraProperty(72, runtime);
}

double ScriptCameraHandler::Yaw(IScriptCameraQueryRuntime& runtime) {
    return CameraProperty(60, runtime);
}

double ScriptCameraHandler::Pitch(IScriptCameraQueryRuntime& runtime) {
    return CameraProperty(64, runtime);
}

double ScriptCameraHandler::Roll(IScriptCameraQueryRuntime& runtime) {
    return CameraProperty(68, runtime);
}

}  // namespace pal4
