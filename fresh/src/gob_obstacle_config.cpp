#include "pal4/gob_obstacle.h"
#include "pal4/gob_obstacle_config.h"
#include "pal4/gob_scene_package.h"

#include <bit>
#include <cstdint>
#include <string_view>

namespace pal4 {

std::uint32_t GobObstacle::InitializeSceneAndConfig(
    const std::uint32_t object, const std::uint32_t first,
    const std::string_view path, const std::uint32_t fourth,
    const std::uint32_t fifth, IGobObstacleConfigRuntime& runtime) {
    runtime.CleanupBeforeSceneLoad(object);
    SelectGobScenePackage(path, runtime);
    const auto result = runtime.InitializeMachineScene(
        object, first, path, fourth, fifth);
    runtime.CleanupScriptPackages();
    if (result == 0u) return 0u;
    runtime.AttachObstacleHelper(object + kHelperOffset, object);
    const auto document = runtime.ReadWord(object + 0x1c0u);
    const auto parameters = runtime.FindXmlChild(
        document, "PAL4-GameObject-Parameters");
    if (parameters != 0u) {
        const auto old = runtime.FindXmlChild(
            parameters, "PAL4_GameObject-obstacle");
        if (old != 0u) runtime.RemoveXmlChild(parameters, old);
        static_cast<void>(runtime.CreateXmlChild(
            parameters, "PAL4_GameObject-obstacle"));
    }
    return result;
}

std::uint32_t GobObstacle::GetConfigNode(
    const std::uint32_t object, IGobObstacleConfigRuntime& runtime) {
    const auto document = runtime.ReadWord(object + 0x1c0u);
    if (document == 0u) return 0u;
    const auto parameters = runtime.FindXmlChild(
        document, "PAL4-GameObject-Parameters");
    if (parameters == 0u) return 0u;
    return runtime.FindXmlChild(parameters, "PAL4_GameObject-obstacle");
}

std::uint32_t GobObstacle::ReadConfigAndResetLaunchRuntime(
    const std::uint32_t object, IGobObstacleConfigRuntime& runtime) {
    runtime.LoadMachineConfig(object);
    const auto node = GetConfigNode(object, runtime);
    if (node == 0u) return 0u;
    runtime.WriteWord(object + 0x170u,
        static_cast<std::uint32_t>(runtime.ReadConfigInt(
            node, "PAL4_GameObject-obstacle_selfvisible", 1)));
    runtime.WriteFloat(object + 0x2d8u, runtime.ReadConfigFloat(
        node, "PAL4_GameObject-obstacle_suction", 0.0f));
    const auto thrust = runtime.ReadConfigFloat(
        node, "PAL4_GameObject-obstacle_thrust", 0.0f);
    runtime.WriteFloat(object + 0x2dcu, thrust);
    runtime.WriteWord(object + 0x2f4u,
        static_cast<std::uint32_t>(runtime.ReadConfigInt(
            node, "PAL4_GAMEOBJECT_obstacle_applyDirect", 0)));
    runtime.WriteWord(object + 0x2f8u,
        static_cast<std::uint32_t>(runtime.ReadConfigInt(
            node, "PAL4_GameObject-obstacle_effType", 0)));
    runtime.WriteFloat(object + 0x2fcu, runtime.ReadConfigFloat(
        node, "PAL4_GameObject-obstacle_effTime", 0.0f));
    runtime.WriteFloat(object + 0x300u, runtime.ReadConfigFloat(
        node, "PAL4_GameObject-obstacle_effDuration", 0.0f));
    runtime.WriteFloat(object + 0x304u, runtime.ReadConfigFloat(
        node, "PAL4_GameObject-obstacle_effHurt", 0.0f));
    runtime.AssignStringRecord(object + 0x308u, runtime.ReadConfigString(
        node, "CSTR_GAMEOBJECT_obstacle_launch_name", ""));
    const auto minimum = runtime.ReadConfigFloat(
        node, "CSTR_GAMEOBJECT_obstacle_launch_tMin", 0.0f);
    runtime.WriteFloat(object + 0x314u, minimum);
    const auto maximum = runtime.ReadConfigFloat(
        node, "CSTR_GAMEOBJECT_obstacle_launch_tMax", 1.0f);
    runtime.WriteFloat(object + 0x318u, maximum);
    runtime.AssignStringRecord(object + 0x320u, runtime.ReadConfigString(
        node, "CSTR_GAMEOBJECT_obstacle_launchEff", ""));
    runtime.WriteFloat(object + 0x31cu, runtime.RandomDelay(
        minimum, maximum,
        static_cast<float>(runtime.TimeMilliseconds())));
    runtime.WriteWord(object + 0x2f0u, 0u);
    runtime.WriteFloat(object + 0x2ecu, thrust);
    return std::bit_cast<std::uint32_t>(thrust);
}

}  // namespace pal4
