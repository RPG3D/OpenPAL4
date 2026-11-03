#include "pal4/pal_console_camera_commands.h"

#include "pal4/pal_console_native_commands.h"

#include <cstdlib>
#include <string>

namespace pal4 {
namespace {

constexpr char kBadArguments[] = "\xb4\xed\xce\xf3\xb2\xce\xca\xfd";
constexpr char kInvalidPlayer[] = "\xce\xde\xd0\xa7\xbd\xc7\xc9\xab";

std::int32_t Output(IPalConsoleCameraRuntime& runtime,
                    std::string_view message) {
    return runtime.AddOutputLine(runtime.Console(), message);
}

std::int32_t SetPlayerAutoseek(std::uint32_t enabled,
                               IPalConsoleCameraRuntime& runtime) {
    const auto manager = runtime.PlayerManager();
    const auto index = runtime.ReadDword(manager + 0x2c50u);
    const auto player = manager + 2836u * index;
    if (player == 0u) return Output(runtime, kInvalidPlayer);
    runtime.WriteDword(player + 0xb0cu, enabled);
    return static_cast<std::int32_t>(player);
}

}  // namespace

std::int32_t PalConsoleCameraCommands::SetAutoseek(
    const char* command, IPalConsoleCameraRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1)) return Output(runtime, kBadArguments);
    const std::string value_text(*parser.Parse(command, 1));
    const auto value = std::atoi(value_text.c_str());
    if (value < 0 || value > 1) return Output(runtime, kBadArguments);
    return SetPlayerAutoseek(value == 1 ? 1u : 0u, runtime);
}

std::int32_t PalConsoleCameraCommands::SetCameraMode(
    const char* command, IPalConsoleCameraRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1) || !parser.Parse(command, 2))
        return Output(runtime, kBadArguments);

    const std::string mode_text(*parser.Parse(command, 1));
    const auto mode = std::atoi(mode_text.c_str());
    if (mode < 0 || mode > 5) return Output(runtime, kBadArguments);
    const std::string follow_text(*parser.Parse(command, 2));
    const auto follow = std::atoi(follow_text.c_str());
    if (follow < 0 || follow > 1) return Output(runtime, kBadArguments);

    runtime.SetModeByIndex(runtime.CameraManager(),
                           static_cast<std::uint32_t>(mode));
    if (runtime.ReadDword(runtime.CameraManager() + 0xc0u) == 0u) {
        const auto camera = runtime.ActiveCameraObject(runtime.CameraManager());
        runtime.SetLookAtFromObjectData(camera, 1u);
        runtime.FlushMainCameraTailYaw(runtime.ActorControl());
    } else {
        const auto camera = runtime.ActiveCameraInternalId(
            runtime.CameraManager());
        runtime.SetPositionAndUpdateMatrix(camera, 0u, 0u, 0u, 1u);
        if (follow == 1) {
            const auto object = runtime.ActiveCameraObject(
                runtime.CameraManager());
            runtime.SetLookAtFromObjectData(object, 1u);
        }
    }
    return SetPlayerAutoseek(follow == 1 ? 1u : 0u, runtime);
}

std::int32_t PalConsoleCameraCommands::SetDistanceOptionEnabled(
    const char* command, IPalConsoleCameraRuntime& runtime) {
    PalConsoleArgumentParser parser;
    if (!parser.Parse(command, 1)) return Output(runtime, kBadArguments);
    const std::string value_text(*parser.Parse(command, 1));
    const auto value = std::atoi(value_text.c_str());
    return runtime.SetDistanceOptimization(value == 1 ? 1u : 0u);
}

}  // namespace pal4
