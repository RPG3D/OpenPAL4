#pragma once

#include <cstdint>

namespace pal4 {

class IScriptRandomIntegerRuntime {
public:
    virtual ~IScriptRandomIntegerRuntime() = default;
    [[nodiscard]] virtual std::int32_t Rand() = 0;
};

class IScriptWorldMiscRuntime : public IScriptRandomIntegerRuntime {
public:
    virtual ~IScriptWorldMiscRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::int32_t SelectWeatherEffect(
        std::uint32_t arena, std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t MinimapManager() = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t TemperatureManager() = 0;
    [[nodiscard]] virtual std::uint8_t SetTemperatureEnabled(
        std::uint32_t manager, std::uint32_t enabled) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    virtual void FlushMainCameraTailYaw(std::uint32_t actor) = 0;
};

class ScriptWorldMisc final {
public:
    // IDA 0x5d3fb0..0x5d4050.
    [[nodiscard]] static std::int32_t SelectWeather(
        std::int32_t value, IScriptWorldMiscRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetMinimapState(
        std::uint32_t state, IScriptWorldMiscRuntime& runtime);
    [[nodiscard]] static std::int32_t RandomInRange(
        std::int32_t minimum, std::int32_t maximum,
        IScriptRandomIntegerRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetTemperatureState(
        std::int32_t value, IScriptWorldMiscRuntime& runtime);
    static void FlushTailYaw(IScriptWorldMiscRuntime& runtime);
};

}  // namespace pal4
