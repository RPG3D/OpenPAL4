#include "pal4/equipment_list_populator.h"
#include "pal4/equipment_item_property_sort.h"


namespace pal4 {

bool EquipmentInventoryHelpers::MatchesCategory(
    std::uint32_t category, const EquipmentCategoryRecord& record) {
    return record.category == category;
}

EquipmentInventorySnapshot EquipmentInventoryHelpers::CopyInventoryData(
    const EquipmentInventorySnapshot& source) {
    return {source.flag, source.keys};
}

namespace {

constexpr std::uint32_t kItemIdOffset = 0x190u;
constexpr std::uint32_t kRequiredLevelOffset = 0x168u;

bool HasItemEquipped(std::uint32_t player, std::uint32_t item_id,
                     IEquipmentListRuntime& runtime) {
    for (std::uint32_t slot = 0u; slot < 6u; ++slot)
        if (runtime.ReadWord(player + 0x920u + slot * 4u) == item_id)
            return true;
    return false;
}

bool PassesInactivePlayerCondition(std::uint32_t item,
                                    IEquipmentListRuntime& runtime) {
    const auto item_id = runtime.ReadWord(item + kItemIdOffset);
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto player = runtime.Player(index);
        if (player != 0u &&
            runtime.ReadWord(player + 0xb08u) == 0u &&
            HasItemEquipped(player, item_id, runtime))
            return false;
    }
    return true;
}

bool ActivePlayerHasItem(std::uint32_t item,
                          IEquipmentListRuntime& runtime) {
    const auto item_id = runtime.ReadWord(item + kItemIdOffset);
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto player = runtime.Player(index);
        if (player != 0u &&
            runtime.ReadWord(player + 0xb08u) != 0u &&
            HasItemEquipped(player, item_id, runtime))
            return true;
    }
    return false;
}

using Items = std::vector<std::uint32_t>;

void SortByRequiredLevel(Items& items, std::uint32_t character,
                         IEquipmentListRuntime& runtime) {
    class PropertyRuntime final : public EquipmentItemPropertyRuntimeStub {
    public:
        explicit PropertyRuntime(IEquipmentListRuntime& source) : source_(source) {}
        std::uint32_t ReadWord(std::uint32_t address) override {
            return source_.ReadWord(address);
        }
    private:
        IEquipmentListRuntime& source_;
    } properties(runtime);
    (void)EquipmentItemPropertySort::Sort(items, character, properties);
}

bool AppendItem(Items& equipped, Items& available, Items& locked,
                Items& no_level, std::uint32_t item,
                std::uint32_t character, IEquipmentListRuntime& runtime,
                bool recent) {
    if (item == 0u || !PassesInactivePlayerCondition(item, runtime))
        return true;
    const auto player = runtime.Player(character);
    if (player == 0u) return false;

    const auto required = runtime.ReadWord(
        item + kRequiredLevelOffset + character * 4u);
    if (required == 0u) {
        // IDA45095B/450988 insert these two recent groups at begin;
        // ordinary category enumeration appends them at end instead.
        no_level.insert(recent ? no_level.begin() : no_level.end(), item);
    } else if (HasItemEquipped(player,
                               runtime.ReadWord(item + kItemIdOffset), runtime)) {
        equipped.insert(recent ? equipped.begin() : equipped.end(), item);
    } else if (required > runtime.PlayerLevel(player, 4u)) {
        locked.push_back(item);
    } else {
        available.push_back(item);
    }
    return true;
}

EquipmentListRow BuildRow(std::uint32_t item, std::uint32_t character,
                          bool no_level, IEquipmentListRuntime& runtime) {
    EquipmentListRow row;
    row.item = item;
    row.item_id = runtime.ReadWord(item + kItemIdOffset);
    const auto name = runtime.ReadWord(item + 0x9cu);
    if (name != 0u) row.name = runtime.ItemText(name);
    row.active_player_equipped = ActivePlayerHasItem(item, runtime);
    const auto required = runtime.ReadWord(
        item + kRequiredLevelOffset + character * 4u);
    row.level_text = no_level ? "--" : std::to_string(required);
    if (no_level) {
        row.disabled = true;
    } else {
        const auto player = runtime.Player(character);
        row.disabled = character >= 4u || required == 0u ||
                       player == 0u ||
                       runtime.PlayerLevel(player, 4u) < required;
    }
    return row;
}

}  // namespace

void EquipmentListPopulator::Populate(
    EquipmentWindowState& state, std::uint32_t category,
    IEquipmentListRuntime& runtime) {
    const auto character = state.Read(0xf4u);
    if (character == 4u) return;

    const bool multiple_players = runtime.ActivePlayerCount() > 1u;
    runtime.SetVisible(state.Read(0x48u), multiple_players);
    runtime.SetVisible(state.Read(0x4cu), multiple_players);

    Items equipped;
    Items available;
    Items locked;
    Items no_level;
    if (category == 5u) {
        const auto snapshot = EquipmentInventoryHelpers::CopyInventoryData(
            runtime.InventoryData());
        for (const auto key : snapshot.keys)
            if (!AppendItem(equipped, available, locked, no_level,
                            runtime.InventoryItem(key, 7u), character, runtime, true))
                return;
    } else {
        for (const auto& record : runtime.CategoryRecords()) {
            if (!EquipmentInventoryHelpers::MatchesCategory(category, record))
                continue;
            const auto count = runtime.ItemCount(record.inventory_key);
            for (std::uint32_t index = 0u; index < count; ++index)
                if (!AppendItem(equipped, available, locked, no_level,
                                runtime.ItemAt(record.inventory_key, index),
                                character, runtime, false))
                    return;
        }
    }

    SortByRequiredLevel(available, character, runtime);
    SortByRequiredLevel(locked, character, runtime);
    const auto list = state.Read(0x40u);
    for (const auto item : equipped)
        runtime.AppendRow(list, BuildRow(item, character, false, runtime));
    for (const auto item : available)
        runtime.AppendRow(list, BuildRow(item, character, false, runtime));
    for (const auto item : locked)
        runtime.AppendRow(list, BuildRow(item, character, false, runtime));
    for (const auto item : no_level)
        runtime.AppendRow(list, BuildRow(item, character, true, runtime));
}

}  // namespace pal4
