#include "pal4/equipment_list_populator.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::EquipmentListRuntimeStub {
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, bool>> visibility;
    std::vector<pal4::EquipmentListRow> rows;
    std::vector<std::uint32_t> keys;
    std::vector<pal4::EquipmentCategoryRecord> records;
    std::unordered_map<std::uint32_t, std::uint32_t> item_by_key;
    std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> items_by_key;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::string> names;
    std::uint32_t current_player = 0xa000u;
    std::uint32_t active_count = 2u;

    std::uint32_t ActivePlayerCount() override {
        calls.emplace_back("active-count");
        return active_count;
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        visibility.emplace_back(window, visible);
        calls.emplace_back("visible");
    }
    pal4::EquipmentInventorySnapshot InventoryData() override {
        calls.emplace_back("inventory-data");
        return {0x5au, keys};
    }
    std::vector<pal4::EquipmentCategoryRecord> CategoryRecords() override {
        calls.emplace_back("category-records");
        return records;
    }
    std::uint32_t InventoryItem(std::uint32_t key,
                                std::uint32_t kind) override {
        Check(kind == 7u, "category five lookup kind");
        calls.emplace_back("lookup-item");
        return item_by_key.at(key);
    }
    std::uint32_t ItemCount(std::uint32_t key) override {
        calls.emplace_back("item-count");
        return static_cast<std::uint32_t>(items_by_key.at(key).size());
    }
    std::uint32_t ItemAt(std::uint32_t key,
                         std::uint32_t index) override {
        calls.emplace_back("item-at");
        return items_by_key.at(key).at(index);
    }
    std::uint32_t Player(std::uint32_t character) override {
        calls.emplace_back("player");
        if (character == 0u) return 0x9000u;
        if (character == 1u) return current_player;
        return 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto it = words.find(address);
        return it == words.end() ? 0u : it->second;
    }
    std::uint32_t PlayerLevel(std::uint32_t player,
                              std::uint32_t field) override {
        Check(player == 0xa000u && field == 4u, "player level virtual call");
        calls.emplace_back("level");
        return 10u;
    }
    std::string ItemText(std::uint32_t address) override {
        calls.emplace_back("text");
        return names.at(address);
    }
    void AppendRow(std::uint32_t list,
                   const pal4::EquipmentListRow& row) override {
        Check(list == 0x4000u, "list target");
        calls.emplace_back("append");
        rows.push_back(row);
    }
};

pal4::EquipmentWindowState State(std::uint32_t character = 1u) {
    pal4::EquipmentWindowState state{};
    state.Write(0x40u, 0x4000u);
    state.Write(0x48u, 0x4800u);
    state.Write(0x4cu, 0x4c00u);
    state.Write(0xf4u, character);
    return state;
}

void AddItem(Runtime& runtime, std::uint32_t item,
             std::uint32_t id, std::uint32_t required,
             const std::string& name) {
    runtime.words[item + 0x190u] = id;
    runtime.words[item + 0x16cu] = required;
    runtime.words[item + 0x9cu] = item + 0x500u;
    runtime.names[item + 0x500u] = name;
}

Runtime MakeRuntime() {
    Runtime runtime;
    runtime.words[0x9000u + 0xb08u] = 0u;
    runtime.words[0xa000u + 0xb08u] = 1u;
    for (std::uint32_t slot = 0u; slot < 6u; ++slot) {
        runtime.words[0x9000u + 0x920u + slot * 4u] = 0xffffffffu;
        runtime.words[0xa000u + 0x920u + slot * 4u] = 0xffffffffu;
    }
    runtime.words[0xa000u + 0x920u] = 100u;
    runtime.words[0x9000u + 0x920u] = 105u;
    AddItem(runtime, 0x20000u, 100u, 5u, "Equipped");
    AddItem(runtime, 0x21000u, 101u, 3u, "Available three");
    AddItem(runtime, 0x22000u, 102u, 1u, "Available one");
    AddItem(runtime, 0x23000u, 103u, 20u, "Locked");
    AddItem(runtime, 0x24000u, 104u, 0u, "No level");
    AddItem(runtime, 0x25000u, 105u, 2u, "Inactive equip");
    runtime.keys = {10u, 11u, 12u, 13u, 14u, 15u, 16u};
    runtime.item_by_key = {{10u, 0x21000u}, {11u, 0x23000u},
                           {12u, 0x20000u}, {13u, 0x24000u},
                           {14u, 0x22000u}, {15u, 0x25000u},
                           {16u, 0u}};
    runtime.records = {{2u, 77u}, {3u, 88u}, {2u, 99u}};
    runtime.items_by_key = {{77u, {0x23000u, 0x21000u}},
                            {88u, {0x20000u}},
                            {99u, {0x24000u}}};
    return runtime;
}

void TestInventoryCategory() {
    auto state = State();
    auto runtime = MakeRuntime();
    pal4::EquipmentListPopulator::Populate(state, 5u, runtime);
    Check(runtime.visibility == std::vector<std::pair<std::uint32_t, bool>>{
              {0x4800u, true}, {0x4c00u, true}},
          "multiple players show both navigation buttons");
    Check(runtime.rows.size() == 5u, "one inactive equipped item filtered");
    const std::vector<std::uint32_t> expected = {100u, 102u, 101u, 103u,
                                                  104u};
    for (std::size_t i = 0u; i < expected.size(); ++i)
        Check(runtime.rows[i].item_id == expected[i],
              "equipped, available, locked, no-level order");
    Check(runtime.rows[0].active_player_equipped &&
              !runtime.rows[0].disabled &&
              runtime.rows[0].name == "Equipped" &&
              runtime.rows[0].level_text == "5",
          "equipped row fields");
    Check(runtime.rows[3].disabled && runtime.rows[3].level_text == "20",
          "locked row shows required level and disabled flag");
    Check(runtime.rows[4].disabled && runtime.rows[4].level_text == "--",
          "zero requirement uses disabled dash row");
    Check(runtime.calls[0] == "active-count" &&
              runtime.calls[1] == "visible" &&
              runtime.calls[2] == "visible" &&
              runtime.calls[3] == "inventory-data",
          "visibility precedes inventory enumeration");
    for (const auto& call : runtime.calls)
        Check(call != "category-records", "category five skips GameDB scan");
}

void TestDatabaseCategory() {
    auto state = State();
    auto runtime = MakeRuntime();
    runtime.active_count = 1u;
    pal4::EquipmentListPopulator::Populate(state, 2u, runtime);
    Check(runtime.visibility == std::vector<std::pair<std::uint32_t, bool>>{
              {0x4800u, false}, {0x4c00u, false}},
          "single player hides navigation buttons");
    Check(runtime.rows.size() == 3u &&
              runtime.rows[0].item_id == 101u &&
              runtime.rows[1].item_id == 103u &&
              runtime.rows[2].item_id == 104u,
          "matching database records expand inventory entries");
    for (const auto& call : runtime.calls)
        Check(call != "inventory-data", "database category skips full copy");
}

void TestRecentSpecialGroupOrder() {
    auto state=State();auto runtime=MakeRuntime();
    AddItem(runtime,0x26000u,106u,4u,"Second equipped");
    AddItem(runtime,0x27000u,107u,0u,"Second unavailable");
    runtime.words[0xa000u+0x924u]=106u;
    runtime.keys.push_back(17u);runtime.keys.push_back(18u);
    runtime.item_by_key[17u]=0x26000u;runtime.item_by_key[18u]=0x27000u;
    pal4::EquipmentListPopulator::Populate(state,5u,runtime);
    const std::vector<std::uint32_t> recent{106u,100u,102u,101u,103u,107u,104u};
    Check(runtime.rows.size()==recent.size(),"recent special groups have both entries");
    for(std::size_t i=0;i<recent.size();++i)Check(runtime.rows[i].item_id==recent[i],
        "recent equipped and no-level groups reverse insertion order; available/locked retain level sort");
    runtime.rows.clear();runtime.records={{2u,77u}};
    runtime.items_by_key[77u]={0x20000u,0x26000u,0x24000u,0x27000u};
    pal4::EquipmentListPopulator::Populate(state,2u,runtime);
    const std::vector<std::uint32_t> normal{100u,106u,104u,107u};
    Check(runtime.rows.size()==normal.size(),"ordinary category special groups have both entries");
    for(std::size_t i=0;i<normal.size();++i)Check(runtime.rows[i].item_id==normal[i],
        "ordinary category equipped and no-level groups preserve source order");
}

void TestSentinelAndMissingPlayer() {
    auto state = State(4u);
    auto runtime = MakeRuntime();
    pal4::EquipmentListPopulator::Populate(state, 5u, runtime);
    Check(runtime.calls.empty() && runtime.rows.empty(),
          "character sentinel exits before services");

    state = State();
    runtime = MakeRuntime();
    runtime.current_player = 0u;
    pal4::EquipmentListPopulator::Populate(state, 5u, runtime);
    Check(runtime.rows.empty(), "missing current player aborts population");
    for (const auto& call : runtime.calls)
        Check(call != "append", "missing player leaves list empty");
}

void TestInventoryHelpers() {
    const pal4::EquipmentCategoryRecord record{2u, 77u};
    Check(pal4::EquipmentInventoryHelpers::MatchesCategory(2u, record) &&
              !pal4::EquipmentInventoryHelpers::MatchesCategory(5u, record),
          "record second word equals requested category");
    const pal4::EquipmentInventorySnapshot source{0xa5u, {10u, 20u, 30u}};
    auto copy = pal4::EquipmentInventoryHelpers::CopyInventoryData(source);
    Check(copy.flag == 0xa5u && copy.keys == source.keys,
          "copy preserves one-byte flag and all inventory keys");
    copy.keys[0] = 99u;
    Check(source.keys[0] == 10u, "copy has independent storage");
    const auto empty = pal4::EquipmentInventoryHelpers::CopyInventoryData(
        {0x5au, {}});
    Check(empty.flag == 0x5au && empty.keys.empty(),
          "empty source copies flag without keys");
}

}  // namespace

int main() {
    TestInventoryCategory();
    TestDatabaseCategory();
    TestRecentSpecialGroupOrder();
    TestSentinelAndMissingPlayer();
    TestInventoryHelpers();
    std::cout << "Equipment list populator tests passed\n";
}
