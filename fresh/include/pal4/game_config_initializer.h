#pragma once

#include "pal4/game_render_system.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

using GameConfigWords = std::array<std::uint32_t, 12>;
using ConfigDocument = std::array<std::uint32_t, 15>;

class IGameConfigLogger {
public:
    virtual ~IGameConfigLogger() = default;
    [[nodiscard]] virtual std::uint32_t InitializeLogger() = 0;
    virtual void AppendLog(std::uint32_t logger, std::string_view format,
                           std::string_view source_file,
                           std::uint32_t source_line) = 0;
};

class IGameConfigRuntime : public IGameConfigLogger {
public:
    ~IGameConfigRuntime() override = default;
    virtual void GetTimerCaps(std::uint32_t& minimum_period,
                              std::uint32_t& maximum_period,
                              std::uint32_t byte_count) = 0;
    virtual void BeginTimePeriod(std::uint32_t period) = 0;
    virtual void InitializeDocument(ConfigDocument& document) = 0;
    virtual void LoadConfig(std::string_view filename,
                            ConfigDocument& document) = 0;
    [[nodiscard]] virtual std::uint32_t FindValue(
        ConfigDocument& document, std::string_view key) = 0;
    [[nodiscard]] virtual std::uint32_t ValueToInt(
        std::uint32_t token, std::uint32_t& output) = 0;
    virtual void DestroyDocument(ConfigDocument& document) = 0;
};

class GameConfigInitializer final {
public:
    GameConfigInitializer(GameConfigWords& config, GameRenderState& state)
        : config_(config), state_(state) {}

    [[nodiscard]] std::uint32_t Initialize(std::uint32_t config_source,
                                           IGameConfigRuntime& runtime);
    [[nodiscard]] std::uint32_t SetInputDevice(std::uint32_t selector,
                                               std::uint32_t callback,
                                               IGameConfigLogger& logger);

private:
    GameConfigWords& config_;
    GameRenderState& state_;
};

}  // namespace pal4
