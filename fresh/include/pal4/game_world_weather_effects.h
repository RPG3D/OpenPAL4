#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct GameWorldEffectHandle {
    std::uint32_t first = 0u;
    std::uint32_t second = 0u;
    bool operator==(const GameWorldEffectHandle&) const = default;
};

class IGameWorldWeatherRuntime {
public:
    virtual ~IGameWorldWeatherRuntime() = default;
    virtual void GetActorControl() = 0;
    [[nodiscard]] virtual bool NeedsEffectUpdate(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t UpdateEffectContent(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetEffectManager() = 0;
    [[nodiscard]] virtual GameWorldEffectHandle CreateEffect(
        std::uint32_t manager, std::string_view name,
        std::uint32_t position_address, std::uint32_t first_flag,
        std::uint32_t second_flag, GameWorldEffectHandle& temporary) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ResetTemporary(
        GameWorldEffectHandle& temporary) = 0;
    [[nodiscard]] virtual std::uint32_t GetCameraManager() = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCamera(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t UpdateEffectProperties(
        std::uint32_t handle, const std::array<std::uint32_t, 3>& position,
        std::uint8_t flag) = 0;
};

class GameWorldWeatherRuntimeStub final : public IGameWorldWeatherRuntime {
public:
    void GetActorControl() override {}
    [[nodiscard]] bool NeedsEffectUpdate(std::uint32_t) override {
        return false;
    }
    [[nodiscard]] std::uint8_t UpdateEffectContent(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t GetEffectManager() override { return 0u; }
    [[nodiscard]] GameWorldEffectHandle CreateEffect(
        std::uint32_t, std::string_view, std::uint32_t,
        std::uint32_t, std::uint32_t,
        GameWorldEffectHandle&) override { return {}; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ResetTemporary(
        GameWorldEffectHandle&) override { return 0u; }
    [[nodiscard]] std::uint32_t GetCameraManager() override { return 0u; }
    [[nodiscard]] std::uint32_t ActiveCamera(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t UpdateEffectProperties(
        std::uint32_t, const std::array<std::uint32_t, 3>&,
        std::uint8_t) override { return 0u; }
};

class GameWorldWeatherEffects final {
public:
    static constexpr std::uint32_t kPositionAddress = 0x84b1dcu;
    static constexpr std::array<std::string_view, 10> kEffectNames = {
        "w_rain_S", "w_rain_L", "w_snow_S", "w_fog_S", "w_fog_M",
        "w_fog_L", "w_sandStorm", "w_snowStorm", "w_flower_Rain",
        "w_fog_l3",
    };

    // IDA 0x51a2d0.
    [[nodiscard]] static std::uint32_t Select(
        std::uint32_t world, std::int32_t weather_type,
        IGameWorldWeatherRuntime& runtime);
    // IDA 0x51a740.
    [[nodiscard]] static std::uint8_t UpdateExisting(
        std::uint32_t world, IGameWorldWeatherRuntime& runtime);
    // IDA 0x51a780; the original two arguments are unused.
    [[nodiscard]] static std::uint8_t SyncCameraPosition(
        std::uint32_t world, std::uint32_t first, std::uint32_t second,
        IGameWorldWeatherRuntime& runtime);
};

}  // namespace pal4
