#include "pal4/rw_light_hierarchy_lifecycle.h"

#include <array>

namespace pal4 {

std::uint32_t RwLightHierarchyLifecycle::Initialize(
    RwLightHierarchyState& state,
    IRwLightHierarchyRuntime& runtime) {
    auto& lights = runtime.Lights();
    state.primary_light = RwLightOperations::CreateTypeTwo(lights);
    if (state.primary_light == 0) return 0;

    state.secondary_light = RwLightOperations::CreateWithFrame(
        {0.0f, 0.0f, 0.0f}, {-60.0f, 20.0f, 0.0f}, lights);
    if (state.secondary_light == 0) return 0;

    constexpr std::array<float, 4> white{
        1.0f, 1.0f, 1.0f, 1.0f};
    (void)RwLightOperations::SetColor(state.primary_light, white, lights);
    (void)RwLightOperations::SetColor(state.secondary_light, white, lights);
    (void)RwLightOperations::SetRadius(
        state.secondary_light, 0x461c4000u, lights);
    (void)RwLightOperations::SetConeAngle(
        state.secondary_light, 0.78539819f, lights);
    (void)runtime.SetActiveNormalMapLights(
        state.secondary_light, state.primary_light);
    return 1;
}

std::uint32_t RwLightHierarchyLifecycle::Shutdown(
    RwLightHierarchyState& state,
    IRwLightHierarchyRuntime& runtime) {
    auto& lights = runtime.Lights();
    if (state.primary_light != 0) {
        RwLightOperations::Destroy(state.primary_light, lights);
        state.primary_light = 0;
    }
    if (state.secondary_light != 0) {
        RwLightOperations::Destroy(state.secondary_light, lights);
        state.secondary_light = 0;
    }
    return runtime.SetActiveNormalMapLights(0, 0);
}

}  // namespace pal4
