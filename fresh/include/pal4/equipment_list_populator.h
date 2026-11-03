#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "pal4/equipment_window.h"

namespace pal4 {

struct EquipmentCategoryRecord {
    std::uint32_t category{};
    std::uint32_t inventory_key{};
};

struct EquipmentInventorySnapshot {
    std::uint8_t flag{};
    std::vector<std::uint32_t> keys;
};

class EquipmentInventoryHelpers final {
public:
    [[nodiscard]] static bool MatchesCategory(
        std::uint32_t category, const EquipmentCategoryRecord& record);
    [[nodiscard]] static EquipmentInventorySnapshot CopyInventoryData(
        const EquipmentInventorySnapshot& source);
};

struct EquipmentListRow {
    std::uint32_t item{};
    std::uint32_t item_id{};
    std::string name;
    std::string level_text;
    bool active_player_equipped{};
    bool disabled{};
};

class IEquipmentListRuntime {
public:
    virtual ~IEquipmentListRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ActivePlayerCount() = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual EquipmentInventorySnapshot InventoryData() = 0;
    [[nodiscard]] virtual std::vector<EquipmentCategoryRecord>
        CategoryRecords() = 0;
    [[nodiscard]] virtual std::uint32_t InventoryItem(
        std::uint32_t key, std::uint32_t kind) = 0;
    [[nodiscard]] virtual std::uint32_t ItemCount(
        std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t ItemAt(
        std::uint32_t key, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t Player(
        std::uint32_t character) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerLevel(
        std::uint32_t player, std::uint32_t field) = 0;
    [[nodiscard]] virtual std::string ItemText(
        std::uint32_t address) = 0;
    virtual void AppendRow(std::uint32_t list,
                           const EquipmentListRow& row) = 0;
};

class EquipmentListRuntimeStub : public IEquipmentListRuntime {
public:
    std::uint32_t ActivePlayerCount() override { return 0; }
    void SetVisible(std::uint32_t, bool) override {}
    EquipmentInventorySnapshot InventoryData() override { return {}; }
    std::vector<EquipmentCategoryRecord> CategoryRecords() override {
        return {};
    }
    std::uint32_t InventoryItem(std::uint32_t, std::uint32_t) override {
        return 0;
    }
    std::uint32_t ItemCount(std::uint32_t) override { return 0; }
    std::uint32_t ItemAt(std::uint32_t, std::uint32_t) override { return 0; }
    std::uint32_t Player(std::uint32_t) override { return 0; }
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    std::uint32_t PlayerLevel(std::uint32_t, std::uint32_t) override {
        return 0;
    }
    std::string ItemText(std::uint32_t) override { return {}; }
    void AppendRow(std::uint32_t, const EquipmentListRow&) override {}
};

class EquipmentListPopulator final {
public:
    static void Populate(EquipmentWindowState& state,
                         std::uint32_t category,
                         IEquipmentListRuntime& runtime);
};

}  // namespace pal4
