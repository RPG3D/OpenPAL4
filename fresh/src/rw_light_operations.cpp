#include "pal4/rw_light_operations.h"

namespace pal4 {

std::uint32_t RwLightOperations::CreateTypeTwo(IRwLightRuntime& runtime) {
    return runtime.CreateLight(2);
}

std::uint32_t RwLightOperations::CreateWithFrame(
    const std::array<float, 3>& position,
    const std::array<float, 3>& angles,
    IRwLightRuntime& runtime) {
    const auto light = runtime.CreateLight(1);
    if (!light)
        return 0;
    const auto frame = runtime.CreateFrame();
    if (!frame) {
        runtime.DestroyLight(light);
        return 0;
    }
    runtime.RotateFrame(frame, {1.0f, 0.0f, 0.0f}, angles[0], 0);
    runtime.RotateFrame(frame, {0.0f, 1.0f, 0.0f}, angles[1], 2);
    runtime.RotateFrame(frame, {0.0f, 0.0f, 1.0f}, angles[2], 2);
    runtime.TranslateFrame(frame, position, 2);
    runtime.SetFrame(light, frame);
    return light;
}

std::uint32_t RwLightOperations::SetColor(
    const std::uint32_t light, const std::array<float, 4>& color,
    IRwLightRuntime& runtime) {
    return light ? runtime.SetColor(light, color) : light;
}

std::uint32_t RwLightOperations::SetRadius(
    const std::uint32_t light, const std::uint32_t radius_bits,
    IRwLightRuntime& runtime) {
    return light ? runtime.SetRadius(light, radius_bits) : light;
}

std::uint32_t RwLightOperations::SetConeAngle(
    const std::uint32_t light, const float angle,
    IRwLightRuntime& runtime) {
    return light ? runtime.SetConeAngle(light, angle) : light;
}

void RwLightOperations::Destroy(
    const std::uint32_t light, IRwLightRuntime& runtime) {
    if (!light)
        return;
    const auto world = runtime.WorldOfLight(light);
    if (world)
        runtime.RemoveLight(world, light);
    const auto frame = runtime.ReadWord(light + 4);
    runtime.SetFrame(light, 0);
    if (frame)
        runtime.DestroyFrame(frame);
    runtime.DestroyLight(light);
}

}  // namespace pal4
