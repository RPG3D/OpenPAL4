#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct GlobalDataSettingsState {
    std::array<std::uint32_t, 5> words{};
    std::uint8_t byte_14 = 0;
};

enum class GlobalDataAudioCollection : std::uint8_t {
    First,
    KindOne,
    KindTwo,
};

class IGlobalDataSettingsSaveRuntime {
public:
    virtual ~IGlobalDataSettingsSaveRuntime() = default;
    virtual void WriteBytes(std::uint32_t archive, const void* data,
                            std::size_t length) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    [[nodiscard]] virtual float AudioLevel(
        std::uint32_t manager, std::uint32_t index) = 0;
};

class IGlobalDataScaleRuntime {
public:
    virtual ~IGlobalDataScaleRuntime() = default;
    virtual void SetAtomicPluginFloatData(float value) = 0;
    [[nodiscard]] virtual bool HasEffectSystem() = 0;
    [[nodiscard]] virtual std::int32_t SetEffectSystemSquaredValue(float value) = 0;
};

class IGlobalDataSettingsRuntime : public IGlobalDataSettingsSaveRuntime, public IGlobalDataScaleRuntime {
public:
    virtual void ReadBytes(std::uint32_t archive, void* data,
                           std::size_t length) = 0;
    virtual void SetAudioLevel(std::uint32_t manager,
                               std::uint32_t index, float value) = 0;
    [[nodiscard]] virtual std::size_t AudioStreamCount(
        std::uint32_t manager,
        GlobalDataAudioCollection collection) = 0;
    [[nodiscard]] virtual std::uint32_t AudioStreamAt(
        std::uint32_t manager,
        GlobalDataAudioCollection collection,
        std::size_t index) = 0;
    [[nodiscard]] virtual std::uint32_t AudioStreamKind(
        std::uint32_t stream) = 0;
    virtual void SetStreamVolume(std::uint32_t stream,
                                 float value) = 0;
    virtual void RefreshStream(std::uint32_t stream) = 0;
};

class GlobalDataSettings final {
public:
    [[nodiscard]] static constexpr GlobalDataSettingsState DefaultState() {return {{0u,2u,1u,1u,1u},1u};}
    [[nodiscard]] static constexpr float ScaleDistance(std::uint32_t option) {return option==0u?1800.0f:1200.0f;}
    // IDA 0x5cce70 / 0x5ccf00 / 0x5cd050 / 0x5cd150.
    [[nodiscard]] static std::uint8_t Save(
        const GlobalDataSettingsState& state, std::uint32_t archive,
        IGlobalDataSettingsSaveRuntime& runtime);
    [[nodiscard]] static std::uint8_t Load(
        GlobalDataSettingsState& state, std::uint32_t archive,
        IGlobalDataSettingsRuntime& runtime);
    [[nodiscard]] static std::int32_t ResetDefaults(
        GlobalDataSettingsState& state,
        IGlobalDataSettingsRuntime& runtime);
    [[nodiscard]] static std::int32_t ApplyScaleOption(
        GlobalDataSettingsState& state, std::uint32_t option,
        IGlobalDataScaleRuntime& runtime);
};

}  // namespace pal4
