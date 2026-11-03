#include "pal4/game_world_weather_effects.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Event {
    std::string name;
    std::uint32_t address = 0u;
    std::uint32_t value = 0u;
    std::string text;
    bool operator==(const Event&) const = default;
};

struct Runtime final : pal4::IGameWorldWeatherRuntime {
    std::vector<Event> events;
    std::map<std::uint32_t, bool> update_needed;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::array<std::uint32_t, 3>> positions;
    std::uint32_t camera = 0u;
    bool mutate_position_on_first = false;

    void GetActorControl() override { events.push_back({"actor"}); }
    bool NeedsEffectUpdate(std::uint32_t address) override {
        events.push_back({"needs", address});
        return update_needed[address];
    }
    std::uint8_t UpdateEffectContent(std::uint32_t address) override {
        events.push_back({"update", address});
        return 0x5au;
    }
    std::uint32_t GetEffectManager() override {
        events.push_back({"manager"});
        return 0x9000u;
    }
    pal4::GameWorldEffectHandle CreateEffect(
        std::uint32_t manager, std::string_view name,
        std::uint32_t position_address, std::uint32_t first_flag,
        std::uint32_t second_flag,
        pal4::GameWorldEffectHandle& temporary) override {
        Check(manager == 0x9000u && position_address == 0x84b1dcu &&
                  first_flag == 1u && second_flag == 0u &&
                  temporary == pal4::GameWorldEffectHandle{},
              "effect manager, shared position and creation flags follow IDA");
        events.push_back({"create", manager, position_address,
                          std::string(name)});
        temporary = {0x555u, 0x666u};
        return {0x111u, 0x222u};
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        events.push_back({"write", address, value});
    }
    std::uint32_t ResetTemporary(
        pal4::GameWorldEffectHandle& temporary) override {
        Check(temporary == pal4::GameWorldEffectHandle{0x555u, 0x666u},
              "temporary handle is reset after copied effect words");
        events.push_back({"reset"});
        return 0x777u;
    }
    std::uint32_t GetCameraManager() override {
        events.push_back({"camera-manager"});
        return 0x8000u;
    }
    std::uint32_t ActiveCamera(std::uint32_t manager) override {
        Check(manager == 0x8000u,
              "active camera lookup receives camera manager");
        events.push_back({"active-camera", manager});
        return camera;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back({"read", address});
        return words[address];
    }
    std::uint8_t UpdateEffectProperties(
        std::uint32_t handle,
        const std::array<std::uint32_t, 3>& position,
        std::uint8_t flag) override {
        events.push_back({"properties", handle, flag});
        positions.push_back(position);
        if (mutate_position_on_first && handle == 0x5198u) {
            ++words[camera + 116u];
            ++words[camera + 120u];
            ++words[camera + 124u];
        }
        return handle == 0x51a0u ? 0x66u : 0x44u;
    }
};

void AllWeatherTypes() {
    constexpr std::array<std::string_view, 10> expected_names = {
        "w_rain_S", "w_rain_L", "w_snow_S", "w_fog_S", "w_fog_M",
        "w_fog_L", "w_sandStorm", "w_snowStorm", "w_flower_Rain",
        "w_fog_l3",
    };
    for (std::int32_t type = 0; type < 10; ++type) {
        Runtime runtime;
        const auto result = pal4::GameWorldWeatherEffects::Select(
            0x5000u, type, runtime);
        Check(result == 0x777u && runtime.events == std::vector<Event>{
                  {"actor"},
                  {"needs", 0x5198u},
                  {"needs", 0x51a0u},
                  {"manager"},
                  {"create", 0x9000u, 0x84b1dcu,
                   std::string(expected_names[static_cast<std::size_t>(type)])},
                  {"write", 0x51a0u, 0x111u},
                  {"write", 0x51a4u, 0x222u},
                  {"reset"},
              },
              "each weather type selects exact name and shared handle flow");
    }
}

void ExistingHandleBranches() {
    for (int mask = 0; mask != 4; ++mask) {
        Runtime runtime;
        runtime.update_needed[0x5198u] = (mask & 1) != 0;
        runtime.update_needed[0x51a0u] = (mask & 2) != 0;
        Check(pal4::GameWorldWeatherEffects::Select(
                  0x5000u, 6, runtime) == 0x777u,
              "weather selection returns temporary reset result");
        std::vector<Event> expected{{"actor"}, {"needs", 0x5198u}};
        if ((mask & 1) != 0) expected.push_back({"update", 0x5198u});
        expected.push_back({"needs", 0x51a0u});
        if ((mask & 2) != 0) expected.push_back({"update", 0x51a0u});
        expected.insert(expected.end(), {
            {"manager"},
            {"create", 0x9000u, 0x84b1dcu, "w_sandStorm"},
            {"write", 0x51a0u, 0x111u},
            {"write", 0x51a4u, 0x222u},
            {"reset"},
        });
        Check(runtime.events == expected,
              "each old handle is conditionally updated in original order");
    }
}

void InvalidTypesAndStub() {
    Runtime runtime;
    Check(pal4::GameWorldWeatherEffects::Select(
              0x5000u, -1, runtime) == 0xffffffffu &&
              runtime.events == std::vector<Event>{{"actor"}},
          "negative weather type returns original bits after actor lookup");
    runtime.events.clear();
    Check(pal4::GameWorldWeatherEffects::Select(
              0x5000u, 10, runtime) == 10u &&
              runtime.events == std::vector<Event>{{"actor"}},
          "out-of-range weather type skips all effect work");

    pal4::GameWorldWeatherRuntimeStub stub;
    Check(pal4::GameWorldWeatherEffects::Select(
              0x5000u, 0, stub) == 0u,
          "native effect stub links and returns its empty handle result");
}

void UpdateExistingHandles() {
    for (int mask = 0; mask != 4; ++mask) {
        Runtime runtime;
        runtime.update_needed[0x5198u] = (mask & 1) != 0;
        runtime.update_needed[0x51a0u] = (mask & 2) != 0;
        const auto result = pal4::GameWorldWeatherEffects::UpdateExisting(
            0x5000u, runtime);
        std::vector<Event> expected{{"needs", 0x5198u}};
        if ((mask & 1) != 0) expected.push_back({"update", 0x5198u});
        expected.push_back({"needs", 0x51a0u});
        if ((mask & 2) != 0) expected.push_back({"update", 0x51a0u});
        Check(result == ((mask & 2) != 0 ? 0x5au : 0u) &&
                  runtime.events == expected,
              "existing effects update independently; second result wins");
    }
}

void SyncCameraPosition() {
    Runtime absent;
    Check(pal4::GameWorldWeatherEffects::SyncCameraPosition(
              0x5000u, 0x11u, 0x22u, absent) == 0u &&
              absent.events == std::vector<Event>{
                  {"camera-manager"}, {"active-camera", 0x8000u}},
          "missing active camera returns before effect handle checks");

    for (int mask = 0; mask != 4; ++mask) {
        Runtime runtime;
        runtime.camera = 0xa000u;
        runtime.words[0xa074u] = 0x111u;
        runtime.words[0xa078u] = 0x222u;
        runtime.words[0xa07cu] = 0x333u;
        runtime.update_needed[0x5198u] = (mask & 1) != 0;
        runtime.update_needed[0x51a0u] = (mask & 2) != 0;
        const auto result = pal4::GameWorldWeatherEffects::SyncCameraPosition(
            0x5000u, 0x11u, 0x22u, runtime);
        std::vector<Event> expected{
            {"camera-manager"}, {"active-camera", 0x8000u},
            {"needs", 0x5198u},
        };
        const auto append_update = [&expected](std::uint32_t handle) {
            expected.insert(expected.end(), {
                {"read", 0xa074u}, {"read", 0xa078u},
                {"read", 0xa07cu}, {"properties", handle, 0u},
            });
        };
        if ((mask & 1) != 0) append_update(0x5198u);
        expected.push_back({"needs", 0x51a0u});
        if ((mask & 2) != 0) append_update(0x51a0u);
        Check(result == ((mask & 2) != 0 ? 0x66u : 0u) &&
                  runtime.events == expected &&
                  runtime.positions == std::vector<std::array<std::uint32_t, 3>>(
                      static_cast<std::size_t>((mask & 1) != 0) +
                          static_cast<std::size_t>((mask & 2) != 0),
                      {0x111u, 0x222u, 0x333u}),
              "camera sync respects both gates and forwards three raw words");
    }

    Runtime changing;
    changing.camera = 0xa000u;
    changing.words[0xa074u] = 1u;
    changing.words[0xa078u] = 2u;
    changing.words[0xa07cu] = 3u;
    changing.update_needed[0x5198u] = true;
    changing.update_needed[0x51a0u] = true;
    changing.mutate_position_on_first = true;
    Check(pal4::GameWorldWeatherEffects::SyncCameraPosition(
              0x5000u, 0u, 0u, changing) == 0x66u &&
              changing.positions ==
                  std::vector<std::array<std::uint32_t, 3>>{
                      {1u, 2u, 3u}, {2u, 3u, 4u}},
          "second effect rereads camera position after first callback");
}

}  // namespace

int main() {
    try {
        AllWeatherTypes();
        ExistingHandleBranches();
        InvalidTypesAndStub();
        UpdateExistingHandles();
        SyncCameraPosition();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
