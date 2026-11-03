#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct TemperatureManagerRegistration {
    std::uint8_t guard = 0;
    std::uint32_t instance = 0;
};

class ITemperatureManagerRuntime {
public:
    virtual ~ITemperatureManagerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destructor) = 0;
    [[nodiscard]] virtual std::uint32_t FloorAttribute() = 0;
    [[nodiscard]] virtual std::uint32_t RgbaToPixel(
        std::uint32_t rgba, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::int32_t AddFloorColor(
        std::uint32_t floor, std::uint32_t pixel,
        std::uint32_t context, std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::int32_t ProcessStateEvent(
        std::uint32_t event_key, std::uint8_t state) = 0;
    [[nodiscard]] virtual std::int32_t Ftol(double value) = 0;
    [[nodiscard]] virtual std::int32_t FpuCompareStatus(
        float value, float threshold) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t index) = 0;
    [[nodiscard]] virtual std::int32_t PlayerMaximum(
        std::uint32_t component, std::uint32_t kind) = 0;
    [[nodiscard]] virtual std::int32_t PlayerValue(
        std::uint32_t component, std::uint32_t property) = 0;
    virtual void SetPlayerValue(std::uint32_t component,
                                std::uint32_t property,
                                std::int32_t value) = 0;
    [[nodiscard]] virtual std::array<float, 2> ProjectToScreen(
        const std::array<float, 3>& position) = 0;
    [[nodiscard]] virtual std::array<float, 2> ScreenToUi(
        const std::array<float, 2>& position) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeFloatingText(
        std::uint32_t memory) = 0;
    virtual void SetFloatingText(std::uint32_t object,
                                 std::string_view text,
                                 std::string_view style, float alpha) = 0;
    virtual void SetFloatingTextPosition(
        std::uint32_t object, const std::array<float, 2>& position) = 0;
    virtual void SetFloatingTextScale(std::uint32_t object,
                                      float horizontal, float vertical) = 0;
    virtual void SetFloatingTextLifetime(
        std::uint32_t object, float lifetime) = 0;
    [[nodiscard]] virtual std::int32_t AppendFloatingText(
        std::uint32_t player_list, std::uint32_t text) = 0;
    virtual void AddDebugText(std::string_view text, std::int32_t x,
                              std::int32_t y, std::uint32_t color) = 0;
    [[nodiscard]] virtual std::uint32_t FindTemperatureWindow(
        std::uint32_t name_pointer) = 0;
    [[nodiscard]] virtual std::int32_t SetWindowPosition(
        std::uint32_t frame, float temperature) = 0;
};

class TemperatureManagerRuntimeStub : public ITemperatureManagerRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t SingletonManager() override { return 0; }
    [[nodiscard]] std::uint32_t RegisterSingleton(
        std::uint32_t, std::string_view, std::uint32_t,
        std::uint32_t, std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t FloorAttribute() override { return 0; }
    [[nodiscard]] std::uint32_t RgbaToPixel(
        std::uint32_t, std::uint32_t) override { return 0; }
    [[nodiscard]] std::int32_t AddFloorColor(
        std::uint32_t, std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0; }
    [[nodiscard]] std::int32_t ProcessStateEvent(
        std::uint32_t, std::uint8_t) override { return 0; }
    [[nodiscard]] std::int32_t Ftol(double) override { return 0; }
    [[nodiscard]] std::int32_t FpuCompareStatus(
        float, float) override { return 0; }
    [[nodiscard]] std::uint32_t PlayerByIndex(
        std::uint32_t) override { return 0; }
    [[nodiscard]] std::int32_t PlayerMaximum(
        std::uint32_t, std::uint32_t) override { return 0; }
    [[nodiscard]] std::int32_t PlayerValue(
        std::uint32_t, std::uint32_t) override { return 0; }
    void SetPlayerValue(std::uint32_t, std::uint32_t,
                        std::int32_t) override {}
    [[nodiscard]] std::array<float, 2> ProjectToScreen(
        const std::array<float, 3>&) override { return {}; }
    [[nodiscard]] std::array<float, 2> ScreenToUi(
        const std::array<float, 2>&) override { return {}; }
    [[nodiscard]] std::uint32_t InitializeFloatingText(
        std::uint32_t) override { return 0; }
    void SetFloatingText(std::uint32_t, std::string_view,
                         std::string_view, float) override {}
    void SetFloatingTextPosition(
        std::uint32_t, const std::array<float, 2>&) override {}
    void SetFloatingTextScale(std::uint32_t, float, float) override {}
    void SetFloatingTextLifetime(std::uint32_t, float) override {}
    [[nodiscard]] std::int32_t AppendFloatingText(
        std::uint32_t, std::uint32_t) override { return 0; }
    void AddDebugText(std::string_view, std::int32_t,
                      std::int32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FindTemperatureWindow(
        std::uint32_t) override { return 0; }
    [[nodiscard]] std::int32_t SetWindowPosition(
        std::uint32_t, float) override { return 0; }
};

class TemperatureManager final {
public:
    static constexpr std::uint32_t kObjectSize = 44;
    static constexpr std::uint32_t kSingletonId = 1015;
    static constexpr std::string_view kSingletonName = "palTemperatureGame";
    static constexpr std::uint32_t kFactory = 0x5b45b0;
    static constexpr std::uint32_t kDestructor = 0x5fc6c0;
    static constexpr std::uint32_t kFloorColorCallback = 0x5b4900;
    static constexpr std::uint32_t kEventKey = 0x8b135c;
    static constexpr std::uint32_t kWindowNamePointer = 0x8b1360;

    // IDA 0x5b4570..0x5b4610: guarded registration and construction.
    [[nodiscard]] static std::uint32_t Register(
        TemperatureManagerRegistration& state,
        ITemperatureManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        ITemperatureManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, ITemperatureManagerRuntime& runtime);
    // IDA 0x5b4700..0x5b4890: temperature and UI lifecycle.
    [[nodiscard]] static std::int32_t Update(
        std::uint32_t object, float elapsed,
        ITemperatureManagerRuntime& runtime);
    [[nodiscard]] static std::int32_t SetState(
        std::uint32_t object, std::uint8_t enabled,
        ITemperatureManagerRuntime& runtime);
    [[nodiscard]] static std::int32_t Reset(
        std::uint32_t object, ITemperatureManagerRuntime& runtime);
    // IDA 0x5b48c0/0x5b48f0/0x5b4900.
    [[nodiscard]] static std::int32_t RegisterFloorColors(
        std::uint32_t object, ITemperatureManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t FloorAttributeThunk(
        ITemperatureManagerRuntime& runtime);
    [[nodiscard]] static std::int32_t SelectFloorColor(
        std::uint32_t object, std::uint32_t pixel,
        ITemperatureManagerRuntime& runtime);
    // IDA 0x5b4920..0x5b4e60: player ticks, floating text, diagnostics and UI.
    [[nodiscard]] static std::int32_t UpdatePlayers(
        std::uint32_t object, float elapsed,
        ITemperatureManagerRuntime& runtime);
    [[nodiscard]] static std::int32_t EmitHealthPopup(
        std::uint32_t player, std::int8_t change,
        ITemperatureManagerRuntime& runtime);
    static void TraceTemperature(std::uint32_t object,
                                 ITemperatureManagerRuntime& runtime);
    [[nodiscard]] static std::int32_t SyncWindow(
        std::uint32_t object, ITemperatureManagerRuntime& runtime);
};

}  // namespace pal4
