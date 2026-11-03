#include "pal4/temperature_manager.h"
#include "pal4/temperature_floor_selection.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <string>

namespace pal4 {
namespace {

float ReadFloat(const std::uint32_t address,
                ITemperatureManagerRuntime& runtime) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

void WriteFloat(const std::uint32_t address, const float value,
                ITemperatureManagerRuntime& runtime) {
    runtime.WriteWord(address, std::bit_cast<std::uint32_t>(value));
}

}  // namespace

std::uint32_t TemperatureManager::Register(
    TemperatureManagerRegistration& state,
    ITemperatureManagerRuntime& runtime) {
    if ((state.guard & 1u) != 0) return state.instance;
    state.guard = static_cast<std::uint8_t>(state.guard | 1u);
    state.instance = runtime.RegisterSingleton(
        runtime.SingletonManager(), kSingletonName, kSingletonId,
        kFactory, kDestructor);
    return state.instance;
}

std::uint32_t TemperatureManager::Create(
    ITemperatureManagerRuntime& runtime) {
    const auto object = runtime.Allocate(kObjectSize);
    return object == 0 ? 0 : Construct(object, runtime);
}

std::uint32_t TemperatureManager::Construct(
    const std::uint32_t object, ITemperatureManagerRuntime& runtime) {
    static_cast<void>(Reset(object, runtime));
    runtime.WriteWord(object + 16,
                      runtime.ReadWord(FloorAttributeThunk(runtime) + 20));
    const auto& kRgba = kTemperatureFloorRgba;
    for (std::uint32_t index = 0; index < kRgba.size(); ++index) {
        runtime.WriteWord(object + index * 4,
                          runtime.RgbaToPixel(kRgba[index], 1280));
    }
    static_cast<void>(RegisterFloorColors(object, runtime));
    return object;
}

std::int32_t TemperatureManager::Update(
    const std::uint32_t object, const float elapsed,
    ITemperatureManagerRuntime& runtime) {
    const auto enabled = runtime.ReadByte(object + 40);
    if (enabled == 0) return 0;
    TraceTemperature(object, runtime);
    const auto state = runtime.ReadWord(object + 20);
    if (state == 4) return 4;

    auto temperature = ReadFloat(object + 24, runtime);
    const auto change = elapsed * ReadFloat(object + 28, runtime);
    switch (state) {
    case 0:
    case 1: temperature = std::max(0.0f, temperature - change); break;
    case 2: temperature = std::max(0.0f, temperature - change * 2.0f); break;
    case 3: temperature = std::min(1.0f, temperature + change * 2.0f); break;
    default: break;
    }
    WriteFloat(object + 24, temperature, runtime);
    static_cast<void>(SyncWindow(object, runtime));
    const auto band = temperature < 0.125f ? 0u :
                      temperature < 0.5f ? 1u :
                      temperature < 0.875f ? 2u : 3u;
    runtime.WriteWord(object + 32, band);
    return UpdatePlayers(object, elapsed, runtime);
}

std::int32_t TemperatureManager::SetState(
    const std::uint32_t object, const std::uint8_t enabled,
    ITemperatureManagerRuntime& runtime) {
    auto result = runtime.ProcessStateEvent(kEventKey, enabled);
    if (enabled != 0) {
        static_cast<void>(Reset(object, runtime));
        result = SyncWindow(object, runtime);
    }
    runtime.WriteByte(object + 40, enabled);
    return result;
}

std::int32_t TemperatureManager::Reset(
    const std::uint32_t object, ITemperatureManagerRuntime& runtime) {
    runtime.WriteWord(object + 20, 4);
    WriteFloat(object + 24, 0.5f, runtime);
    WriteFloat(object + 28, 0.0625f, runtime);
    runtime.WriteWord(object + 32, 2);
    WriteFloat(object + 36, 0.0f, runtime);
    runtime.WriteByte(object + 40, 0);
    return 0;
}

std::int32_t TemperatureManager::RegisterFloorColors(
    const std::uint32_t object, ITemperatureManagerRuntime& runtime) {
    std::int32_t result = 0;
    for (std::uint32_t index = 0; index < 5; ++index) {
        result = runtime.AddFloorColor(
            FloorAttributeThunk(runtime), runtime.ReadWord(object + index * 4),
            object, kFloorColorCallback);
    }
    return result;
}

std::uint32_t TemperatureManager::FloorAttributeThunk(
    ITemperatureManagerRuntime& runtime) {
    return runtime.FloorAttribute();
}

std::int32_t TemperatureManager::SelectFloorColor(
    const std::uint32_t object, const std::uint32_t pixel,
    ITemperatureManagerRuntime& runtime) {
    std::array<std::uint32_t,5> colors{};
    for (std::uint32_t index=0; index<colors.size(); ++index)
        colors[index]=runtime.ReadWord(object+index*4);
    auto selected=runtime.ReadWord(object+20);
    const auto result=SelectTemperatureFloorColor(colors,pixel,selected);
    if (result<5) runtime.WriteWord(object+20,selected);
    return static_cast<std::int32_t>(result);
}

std::int32_t TemperatureManager::UpdatePlayers(
    const std::uint32_t object, const float elapsed,
    ITemperatureManagerRuntime& runtime) {
    const auto timer = ReadFloat(object + 36, runtime) + elapsed;
    WriteFloat(object + 36, timer, runtime);
    if (timer < 3.0f) return runtime.FpuCompareStatus(timer, 3.0f);
    WriteFloat(object + 36, timer - 3.0f, runtime);

    float fraction = 0.0f;
    switch (runtime.ReadWord(object + 20)) {
    case 0:
    case 3: fraction = -0.1f; break;
    case 1: fraction = -0.05f; break;
    case 2: fraction = 0.05f; break;
    default: break;
    }
    for (std::uint32_t index = 0; index < 4; ++index) {
        const auto player = runtime.PlayerByIndex(index);
        if (player == 0 || runtime.ReadWord(player + 0xb08) == 0) continue;
        const auto component = player + 0x654;
        const auto maximum_health = runtime.PlayerMaximum(component, 0);
        const auto current_health = runtime.PlayerValue(component, 7);
        auto health = runtime.Ftol(
            static_cast<double>(current_health) +
            static_cast<double>(maximum_health) * fraction);
        if (health <= 0) health = 1;
        else if (health > maximum_health) health = maximum_health;
        runtime.SetPlayerValue(component, 7, health);

        static_cast<void>(EmitHealthPopup(
            player, static_cast<std::int8_t>(runtime.Ftol(fraction)),
            runtime));
        const auto maximum_mana = runtime.PlayerMaximum(component, 2);
        const auto current_mana = runtime.PlayerValue(component, 9);
        auto mana = runtime.Ftol(
            static_cast<double>(current_mana) +
            static_cast<double>(maximum_mana) * fraction);
        if (mana <= 0) mana = 0;
        else if (mana > maximum_mana) mana = maximum_mana;
        runtime.SetPlayerValue(component, 9, mana);
    }
    return 4;
}

std::int32_t TemperatureManager::EmitHealthPopup(
    const std::uint32_t player, const std::int8_t change,
    ITemperatureManagerRuntime& runtime) {
    const std::array<float, 3> world = {
        ReadFloat(player + 172, runtime),
        ReadFloat(player + 176, runtime) +
            ReadFloat(player + 244, runtime) + 20.0f,
        ReadFloat(player + 180, runtime),
    };
    const auto screen = runtime.ProjectToScreen(world);
    const auto ui = runtime.ScreenToUi(screen);
    const auto memory = runtime.Allocate(72);
    if (memory == 0) return 0;
    const auto text = runtime.InitializeFloatingText(memory);
    runtime.SetFloatingText(text, std::to_string(change),
                            change >= 0 ? "HPinc" : "RoleHPdec", 1.0f);
    runtime.SetFloatingTextPosition(text, ui);
    runtime.SetFloatingTextScale(text, 0.5f, 0.5f);
    runtime.SetFloatingTextLifetime(text, 40.0f);
    return runtime.AppendFloatingText(player + 2728, text);
}

void TemperatureManager::TraceTemperature(
    const std::uint32_t object, ITemperatureManagerRuntime& runtime) {
    char buffer[64]{};
    std::snprintf(buffer, sizeof(buffer), "Temperature: %f",
                  static_cast<double>(ReadFloat(object + 24, runtime)));
    std::string line = buffer;
    runtime.AddDebugText(line, 0, 50, 65280);
    constexpr std::array<std::string_view, 5> kFloorNames = {
        "floorColour: Snow", "floorColour: Ice", "floorColour: Freeze",
        "floorColour: Hot", "floorColour: Normal"};
    const auto floor = runtime.ReadWord(object + 20);
    if (floor < kFloorNames.size()) line = kFloorNames[floor];
    runtime.AddDebugText(line, 0, 60, 65280);
    constexpr std::array<std::string_view, 4> kRangeNames = {
        "TempRange:\tFreeze", "TempRange:\tCold",
        "TempRange:\tCozy", "TempRange:\tHot"};
    const auto range = runtime.ReadWord(object + 32);
    if (range < kRangeNames.size()) line = kRangeNames[range];
    runtime.AddDebugText(line, 0, 70, 65280);
}

std::int32_t TemperatureManager::SyncWindow(
    const std::uint32_t object, ITemperatureManagerRuntime& runtime) {
    const auto frame = runtime.FindTemperatureWindow(kWindowNamePointer);
    if (frame == 0) return 0;
    return runtime.SetWindowPosition(frame, ReadFloat(object + 24, runtime));
}

}  // namespace pal4
