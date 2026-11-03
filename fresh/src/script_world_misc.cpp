#include "pal4/script_world_misc.h"

#include <bit>
#include <limits>
#include <stdexcept>

namespace pal4 {

std::int32_t ScriptWorldMisc::SelectWeather(
    std::int32_t value, IScriptWorldMiscRuntime& runtime) {
    return runtime.SelectWeatherEffect(runtime.ArenaManager(), value);
}

std::uint32_t ScriptWorldMisc::SetMinimapState(
    std::uint32_t state, IScriptWorldMiscRuntime& runtime) {
    if (state > 1u) return state;
    const auto minimap = runtime.MinimapManager();
    runtime.WriteWord(minimap + 0x50u, state == 0u ? 2u : 1u);
    return minimap;
}

std::int32_t ScriptWorldMisc::RandomInRange(
    std::int32_t minimum, std::int32_t maximum,
    IScriptRandomIntegerRuntime& runtime) {
    const auto random = runtime.Rand();
    const auto width = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(maximum) -
        static_cast<std::uint32_t>(minimum));
    if (width == 0 ||
        (random == std::numeric_limits<std::int32_t>::min() && width == -1))
        throw std::domain_error("invalid native random interval");
    return std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(minimum) +
        static_cast<std::uint32_t>(random % width));
}

std::uint8_t ScriptWorldMisc::SetTemperatureState(
    std::int32_t value, IScriptWorldMiscRuntime& runtime) {
    const auto enabled = value != 0 ? 1u : 0u;
    return runtime.SetTemperatureEnabled(runtime.TemperatureManager(), enabled);
}

void ScriptWorldMisc::FlushTailYaw(IScriptWorldMiscRuntime& runtime) {
    runtime.FlushMainCameraTailYaw(runtime.ActorControl());
}

}  // namespace pal4
