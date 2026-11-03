#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptEquipmentAcquisitionRuntime {
public:
    virtual ~IScriptEquipmentAcquisitionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateHintWindow(
        std::uint32_t frames) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint8_t HasEquipment(
        std::uint32_t table, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t FindEquipment(
        std::uint32_t table, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t Inventory() = 0;
    virtual void AddToInventory(std::uint32_t collection,
                                std::uint32_t id) = 0;
    virtual void InitializeText(std::uint32_t buffer) = 0;
    [[nodiscard]] virtual std::string_view EquipmentName(
        std::uint32_t record) = 0;
    [[nodiscard]] virtual std::string_view GameString(
        std::uint32_t table, std::uint32_t id) = 0;
    virtual void FormatText(std::string_view format, std::string_view name,
                            std::uint32_t count) = 0;
    virtual void AppendText() = 0;
    [[nodiscard]] virtual std::string_view HintText() = 0;
    virtual void ShowHint(std::uint32_t window, std::string_view text,
                          float x, float y, float scale,
                          std::uint32_t mode, std::uint32_t enabled) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    virtual void PlaySound(std::uint32_t audio, std::uint8_t flag,
                           std::string_view path, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint8_t CleanupText(
        std::uint32_t buffer) noexcept = 0;
};

class ScriptEquipmentAcquisition final {
public:
    // IDA 0x5d52f0, giAddEquipment.
    [[nodiscard]] static std::uint8_t Add(
        std::uint32_t id, std::uint8_t show_hint,
        IScriptEquipmentAcquisitionRuntime& runtime);
};

}  // namespace pal4
