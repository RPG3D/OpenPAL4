#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/equipment_window.h"

namespace pal4 {

using EquipmentStatSnapshot = std::array<std::int32_t, 84>;

struct EquipmentImageColour {
    float red;
    float green;
    float blue;
    float alpha;
};

class IEquipmentItemDetailsRuntime {
public:
    virtual ~IEquipmentItemDetailsRuntime() = default;
    virtual void SetText(std::uint32_t window,
                         std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string ReadText(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    [[nodiscard]] virtual std::string RenderItemAttributes(
        std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t Player(
        std::uint32_t character) = 0;
    [[nodiscard]] virtual std::int32_t PlayerLevel(
        std::uint32_t player, std::uint32_t field) = 0;
    virtual void SetImageColour(std::uint32_t image,
                                EquipmentImageColour colour) = 0;
    virtual void RefreshPlayer(std::uint32_t player) = 0;
    [[nodiscard]] virtual EquipmentStatSnapshot ReadStats(
        std::uint32_t player) = 0;
    virtual void InitializeProjectedStats() = 0;
    [[nodiscard]] virtual std::uint32_t EquipmentSlot(
        std::uint32_t player, std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t RemovePreviewObject(
        std::uint32_t id) = 0;
};

class EquipmentItemDetailsRuntimeStub : public IEquipmentItemDetailsRuntime {
public:
    void SetText(std::uint32_t, std::string_view) override {}
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    std::string ReadText(std::uint32_t) override { return {}; }
    std::string ConvertToUtf8(std::string_view text) override {
        return std::string(text);
    }
    std::string RenderItemAttributes(std::uint32_t) override { return {}; }
    std::uint32_t Player(std::uint32_t) override { return 0; }
    std::int32_t PlayerLevel(std::uint32_t, std::uint32_t) override {
        return 0;
    }
    void SetImageColour(std::uint32_t, EquipmentImageColour) override {}
    void RefreshPlayer(std::uint32_t) override {}
    EquipmentStatSnapshot ReadStats(std::uint32_t) override { return {}; }
    void InitializeProjectedStats() override {}
    std::uint32_t EquipmentSlot(std::uint32_t, std::uint32_t) override {
        return 6;
    }
    std::uint32_t RemovePreviewObject(std::uint32_t) override { return 0; }
};

class EquipmentItemDetails final {
public:
    static void Update(EquipmentWindowState& state, std::uint32_t item,
                       IEquipmentItemDetailsRuntime& runtime);
    [[nodiscard]] static std::uint32_t Clear(
        EquipmentWindowState& state,
        IEquipmentItemDetailsRuntime& runtime);
    [[nodiscard]] static std::string FormatStat(std::int32_t original,
                                                 std::int32_t projected);
};

}  // namespace pal4
