#include "pal4/script_world_misc.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptWorldMiscRuntime {
    std::int32_t random = 37;
    std::vector<std::string> calls;
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena-manager");
        return 0x1000u;
    }
    std::int32_t SelectWeatherEffect(std::uint32_t arena,
                                     std::int32_t value) override {
        Check(arena == 0x1000u && value == -3,
              "weather effect operands");
        calls.emplace_back("select-weather");
        return -9;
    }
    std::uint32_t MinimapManager() override {
        calls.emplace_back("minimap-manager");
        return 0x2000u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x2050u, "minimap state offset");
        calls.push_back("minimap-state:" + std::to_string(value));
    }
    std::int32_t Rand() override {
        calls.emplace_back("rand");
        return random;
    }
    std::uint32_t TemperatureManager() override {
        calls.emplace_back("temperature-manager");
        return 0x3000u;
    }
    std::uint8_t SetTemperatureEnabled(std::uint32_t manager,
                                        std::uint32_t enabled) override {
        Check(manager == 0x3000u && enabled <= 1u,
              "temperature state operands");
        calls.push_back("temperature-state:" + std::to_string(enabled));
        return 0xa5u;
    }
    std::uint32_t ActorControl() override {
        calls.emplace_back("actor-control");
        return 0x4000u;
    }
    void FlushMainCameraTailYaw(std::uint32_t actor) override {
        Check(actor == 0x4000u, "tail yaw actor");
        calls.emplace_back("flush-yaw");
    }
};

void WeatherAndMinimap() {
    Runtime weather;
    Check(pal4::ScriptWorldMisc::SelectWeather(-3, weather) == -9 &&
          weather.calls == std::vector<std::string>{
              "arena-manager", "select-weather"},
          "weather wrapper forwards signed value and result");

    for (const auto [state, value] :
         {std::pair{0u, 2u}, std::pair{1u, 1u}}) {
        Runtime runtime;
        Check(pal4::ScriptWorldMisc::SetMinimapState(state, runtime) ==
                  0x2000u &&
              runtime.calls == std::vector<std::string>{
                  "minimap-manager", "minimap-state:" + std::to_string(value)},
              "minimap recognized state writes +0x50 and returns manager");
    }
    for (const auto state : {2u, 0xffffffffu}) {
        Runtime runtime;
        Check(pal4::ScriptWorldMisc::SetMinimapState(state, runtime) == state &&
              runtime.calls.empty(),
              "other minimap state returns input without manager access");
    }
}

void RandomRange() {
    Runtime runtime;
    Check(pal4::ScriptWorldMisc::RandomInRange(10, 20, runtime) == 17 &&
          runtime.calls == std::vector<std::string>{"rand"},
          "signed remainder, upper endpoint excluded");
    runtime.calls.clear();
    Check(pal4::ScriptWorldMisc::RandomInRange(20, 10, runtime) == 27 &&
          runtime.calls == std::vector<std::string>{"rand"},
          "negative interval follows native signed idiv");
    runtime.random = -37;
    Check(pal4::ScriptWorldMisc::RandomInRange(10, 20, runtime) == 3,
          "negative random source keeps signed remainder");

    runtime.calls.clear();
    try {
        (void)pal4::ScriptWorldMisc::RandomInRange(10, 10, runtime);
        throw std::runtime_error("missing zero-width failure");
    } catch (const std::domain_error&) {
        Check(runtime.calls == std::vector<std::string>{"rand"},
              "random source runs before invalid division is detected");
    }
}

void TemperatureAndYaw() {
    Runtime off;
    Check(pal4::ScriptWorldMisc::SetTemperatureState(0, off) == 0xa5u &&
          off.calls == std::vector<std::string>{
              "temperature-manager", "temperature-state:0"},
          "zero disables temperature manager");
    Runtime on;
    Check(pal4::ScriptWorldMisc::SetTemperatureState(-1, on) == 0xa5u &&
          on.calls == std::vector<std::string>{
              "temperature-manager", "temperature-state:1"},
          "nonzero input normalizes to enabled state");
    Runtime yaw;
    pal4::ScriptWorldMisc::FlushTailYaw(yaw);
    Check(yaw.calls == std::vector<std::string>{
              "actor-control", "flush-yaw"},
          "yaw wrapper forwards actor-control singleton");
}

}  // namespace

int main() {
    try {
        WeatherAndMinimap();
        RandomRange();
        TemperatureAndYaw();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
