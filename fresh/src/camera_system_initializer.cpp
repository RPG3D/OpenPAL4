#include "pal4/camera_system_initializer.h"

#include <bit>

namespace pal4 {

bool CameraSystemInitializer::Initialize(
    CameraControllerState& state, std::uint32_t controller_address) {
    runtime_.LogInitialization(false);
    const auto bounds = runtime_.RenderRect();
    const CameraRect viewport{
        bounds[0], bounds[1],
        std::bit_cast<std::int32_t>(
            static_cast<std::uint32_t>(bounds[2]) -
            static_cast<std::uint32_t>(bounds[0])),
        std::bit_cast<std::int32_t>(
            static_cast<std::uint32_t>(bounds[3]) -
            static_cast<std::uint32_t>(bounds[1]))};

    constexpr std::array<const char*, 4> names{
        "MainCamera", "ScriptCamera", "CombatCamera", "UiCamera"};
    bool result = false;
    for (std::size_t index = 0; index < names.size(); ++index) {
        const auto object = runtime_.AllocateObject(0x20u);
        if (object == 0u) return false;
        runtime_.ConstructObject(object);
        state.At(0x34u + index * 4u) = object;
        CameraSetupParameters parameters{
            names[index], viewport, true,
            {0.0f, 0.0f, 0.0f}, 350.0f,
            30.0f, 225.0f, 0.0f};
        result = runtime_.SetupObject(object, parameters);
        if (index == 0u)
            runtime_.CameraDataForObject(object).At(0x2bcu) = 0x5ec270u;
    }

    state.At(0x30u) = state.At(0x34u);
    constexpr std::array<std::uint32_t, 30> defaults{
        1130430464u, 1104150528u, 1141276672u, 1u, 1u,
        1127481344u, 1099956224u, 1132042650u, 0u, 0u,
        1131413504u, 1099956224u, 1132042650u, 0u, 0u,
        1123024896u, 1099956224u, 1132042650u, 0u, 0u,
        1127481344u, 1099956224u, 1139802112u, 0u, 0u,
        1130430464u, 1088421888u, 1133674496u, 1u, 1u};
    for (std::size_t index = 0; index < defaults.size(); ++index)
        state.At(0x44u + index * 4u) = defaults[index];
    state.At(0xc0u) = 0u;
    state.At(0xbcu) = controller_address + 0x44u;
    runtime_.LogInitialization(true);
    return result;
}

}  // namespace pal4
