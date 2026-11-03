#include "pal4/native_player_slot_collection_runtime.h"
#include "pal4/indexed_collection_reset.h"
#include "pal4/native_player_collection_values.h"
#include "pal4/native_game_database.h"
#include "pal4/native_inventory_session.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Address(const void* value) {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(value));
}

struct Inventory final : pal4::IInventoryItemSlotsRuntime {
    std::vector<std::uint32_t> ids;
    std::uint32_t destination{};
    bool reject = false;

    std::uint32_t ReadDword(std::uint32_t) override { return 0u; }
    std::uint32_t GetGameDatabase() override { return 0u; }
    std::uint32_t ProcessGameDatabaseArray(std::uint32_t,
                                           std::uint32_t) override {
        return 0u;
    }
    std::uint32_t GetInventory() override {
        if (reject) throw std::logic_error("inventory unavailable");
        return 0x6000u;
    }
    std::uint32_t FindInventoryEntry(std::uint32_t array,
                                     std::uint32_t id,
                                     std::uint32_t category) override {
        Check(array == 0x607cu && category == 7u,
              "original inventory category lookup");
        ids.push_back(id);
        return 0u;
    }
    std::uint32_t ProcessEntry(std::uint32_t, std::uint32_t,
                               std::uint32_t) override {
        return 0u;
    }
    void InsertEntry(std::uint32_t target, std::uint32_t entry) override {
        Check(entry == 0u && (destination == 0u || target == destination),
              "missing inventory record does not add a result");
        destination = target;
    }
};

struct Values final : pal4::ISortedValueProcessRuntime {
    std::vector<std::uint32_t> values;
    std::uint32_t ReadDword(std::uint32_t address) override {
        return *reinterpret_cast<const std::uint32_t*>(address);
    }
    void Process(std::uint32_t context, std::uint32_t value) override {
        auto& result = *reinterpret_cast<pal4::PlayerManagerResultArray*>(
            context);
        Check(result.end + 4u <= result.capacity_end,
              "temporary result has capacity");
        *reinterpret_cast<std::uint32_t*>(result.end) = value;
        result.end += 4u;
        values.push_back(value);
    }
};

struct Script final : pal4::INativePlayerCollectionScriptRuntime {
    std::uint32_t expected_script{};
    std::vector<std::uint32_t> fades;
    std::uint32_t finalizations{};
    bool reject{};
    void HandleFade(std::uint32_t script, std::uint32_t value,
                    std::uint32_t marker) override {
        Check(script == expected_script && marker == script,
              "collection fade targets player script");
        if (reject) throw std::runtime_error("fade service failure");
        fades.push_back(value);
    }
    void FinalizeScript(std::uint32_t script) override {
        Check(script == expected_script,
              "collection finalizes player script");
        ++finalizations;
    }
};

void Run() {
    std::array<std::uint32_t, 709u> player{};
    const auto object = Address(player.data());
    pal4::InventoryItemSlotsState slots{};
    slots.ids = {1u, 2u, 3u, 4u, 5u, 6u};
    std::memcpy(reinterpret_cast<void*>(object + 0x920u), &slots,
                sizeof(slots));
    std::array<std::uint32_t, 2u> source{101u, 102u};
    auto collection = pal4::IndexedCollectionResetState{};
    collection.values[0].begin = Address(source.data());
    collection.values[0].end = collection.values[0].begin + 4u;
    collection.key_values[5].begin = Address(source.data() + 1u);
    collection.key_values[5].end = collection.key_values[5].begin + 4u;
    std::memcpy(reinterpret_cast<void*>(object + 0x938u), &collection,
                sizeof(collection));

    pal4::NativeBaseObjectMemory memory;
    Inventory inventory;
    Values indexed;
    Values fixed;
    Script script;
    script.expected_script = object + 0x654u;
    pal4::NativePlayerSlotCollectionRuntime runtime(
        memory, inventory, indexed, fixed, script);
    (void)pal4::PlayerManagerSlotCollectionProcessor::Process(
        object, 0x5au, runtime);
    Check(inventory.ids == std::vector<std::uint32_t>{1u, 2u, 3u, 4u, 5u, 6u} &&
              indexed.values == std::vector<std::uint32_t>{101u} &&
              fixed.values == std::vector<std::uint32_t>{102u} &&
              script.fades == std::vector<std::uint32_t>{101u, 102u} &&
              script.finalizations == 1u &&
              memory.OutstandingAllocations() == 0u,
          "native adapter reuses inventory and indexed collection stages");

    inventory.reject = true;
    bool failed = false;
    try {
        (void)pal4::PlayerManagerSlotCollectionProcessor::Process(
            object, 0u, runtime);
    } catch (const std::logic_error&) {
        failed = true;
    }
    Check(failed && memory.OutstandingAllocations() == 0u &&
              script.finalizations == 1u,
          "missing inventory service frees temporary collection storage");
}

}  // namespace

namespace {
pal4::NativeGameDbTable Table(std::string name, std::string field,
    std::initializer_list<std::pair<std::uint32_t, std::uint32_t>> rows) {
    pal4::NativeGameDbTable table;
    table.name = std::move(name);
    table.fields = {{"ID", 1u}, {std::move(field), 1u}};
    for (const auto& [key, value] : rows) table.rows.push_back({key, value});
    return table;
}
void NativeValues() {
    auto magic_table = Table("Magic", "ATTACHED_SKILL", {{101u,1u},{102u,2u},{103u,99u},{104u,0u}});
    auto stunt_table = Table("Stunt", "ATTACHED_SKILL", {{201u,1u},{202u,2u}});
    auto skill_table = Table("Skill", "EXIST_TIME", {{0u,0xffffffffu},{1u,0xffffffffu},{2u,0u}});
    pal4::NativePlayerCollectionCatalog catalog(magic_table, stunt_table, skill_table);
    Check(catalog.MagicSkill(104u) == 0u && catalog.SkillDuration(0u) == 0xffffffffu &&
        !catalog.MagicSkill(999u), "typed projections preserve zero and distinguish missing records");
    pal4::NativeBaseObjectMemory memory;
    pal4::NativePlayerCollectionValues magic(catalog, memory, pal4::NativePlayerCollectionValues::Kind::Magic);
    pal4::NativePlayerCollectionValues stunt(catalog, memory, pal4::NativePlayerCollectionValues::Kind::Stunt);
    pal4::PlayerManagerResultArray result{};
    const auto context = Address(&result);
    for (auto id : {999u, 102u, 103u}) magic.Process(context, id);
    stunt.Process(context, 202u);
    Check(result.begin == 0u, "missing primary/skill and finite skills leave empty result unallocated");
    magic.Process(context, 104u);
    magic.Process(context, 101u);
    stunt.Process(context, 201u);
    Check((result.end - result.begin) / 4u == 3u && memory.ReadDword(result.begin) == 0u &&
        memory.ReadDword(result.begin + 4u) == 1u && memory.ReadDword(result.begin + 8u) == 1u,
        "Magic and Stunt append passive skill IDs in source order without deduplication");
    (void)memory.Free(result.begin);

    std::array<std::uint32_t,709u> player{};
    const auto object = Address(player.data());
    std::array<std::uint32_t,40u> values;
    values.fill(101u);
    pal4::IndexedCollectionResetState collections{};
    collections.values[0].begin = Address(values.data());
    collections.values[0].end = collections.values[0].begin + sizeof(values);
    std::array<std::uint32_t,1u> fixed{201u};
    collections.key_values[5].begin = Address(fixed.data());
    collections.key_values[5].end = collections.key_values[5].begin + sizeof(fixed);
    std::memcpy(reinterpret_cast<void*>(object + 0x938u), &collections, sizeof(collections));
    pal4::NativeInventorySession inventory;
    (void)inventory.Start();
    Script script;
    script.expected_script = object + 0x654u;
    pal4::NativePlayerSlotCollectionRuntime runtime(memory, inventory, magic, stunt, script);
    (void)pal4::PlayerManagerSlotCollectionProcessor::Process(object, 0u, runtime);
    Check(script.fades == std::vector<std::uint32_t>(41u,1u) && script.finalizations == 1u &&
        memory.OutstandingAllocations() == 0u,
        "real database projection grows beyond32 results and forwards all41 skills before cleanup");
    script.reject = true;
    bool threw = false;
    try { (void)pal4::PlayerManagerSlotCollectionProcessor::Process(object, 0u, runtime); }
    catch (const std::runtime_error&) { threw = true; }
    Check(threw && memory.OutstandingAllocations() == 0u && script.finalizations == 1u,
        "throwing fade service frees relocated result storage and stops finalization");
    auto malformed = skill_table;
    malformed.fields[1].type = 2u;
    threw = false;
    try { pal4::NativePlayerCollectionCatalog invalid(magic_table, stunt_table, malformed); }
    catch (const std::runtime_error&) { threw = true; }
    Check(threw, "invalid source schema is rejected");
    malformed = skill_table;
    malformed.rows.push_back(malformed.rows.front());
    threw = false;
    try { pal4::NativePlayerCollectionCatalog invalid(magic_table, stunt_table, malformed); }
    catch (const std::runtime_error&) { threw = true; }
    Check(threw, "duplicate source keys are rejected rather than silently selecting a row");
    inventory.Close();
}
void OriginalValues(const char* root) {
    pal4::NativeGameDatabase database;
    Check(database.Load(root), "load original collection database");
    pal4::NativePlayerCollectionCatalog catalog(database);
    pal4::NativeBaseObjectMemory memory;
    pal4::PlayerManagerResultArray result{};
    std::vector<std::uint32_t> expected;
    for (const auto kind : {pal4::NativePlayerCollectionValues::Kind::Magic,
                            pal4::NativePlayerCollectionValues::Kind::Stunt}) {
        const auto& table = database.Tables().at(kind == pal4::NativePlayerCollectionValues::Kind::Magic ? "Magic" : "Stunt");
        const auto index = [&](std::string_view name) {
            for (std::size_t i = 0; i < table.fields.size(); ++i) if (table.fields[i].name == name) return i;
            throw std::runtime_error("missing original field");
        };
        const auto id_field = index("ID"), attached_field = index("ATTACHED_SKILL");
        pal4::NativePlayerCollectionValues values(catalog, memory, kind);
        for (const auto& row : table.rows) {
            const auto id = std::get<std::uint32_t>(row[id_field]);
            const auto skill = std::get<std::uint32_t>(row[attached_field]);
            const auto duration = catalog.SkillDuration(skill);
            if (duration && *duration == 0xffffffffu) expected.push_back(skill);
            values.Process(Address(&result), id);
        }
    }
    Check(!expected.empty() && result.end - result.begin == expected.size() * 4u &&
        std::memcmp(reinterpret_cast<const void*>(result.begin), expected.data(), expected.size() * 4u) == 0,
        "all original Magic/Stunt rows collect exact passive skill sequence");
    (void)memory.Free(result.begin);
    Check(memory.OutstandingAllocations() == 0u, "original database collection releases all result growth");
    result = {};
    pal4::NativeInventorySession inventory;
    inventory.BindDatabase(database);
    const auto inventory_address = inventory.Start();
    const auto sword = inventory.ProcessEquipment(inventory_address + 0x7cu, 3501u);
    // Only the owned snapshot is changed; original database/assets stay intact.
    memory.WriteDword(sword + 0x180u, expected.front());
    pal4::NativePlayerCollectionInventory equipment(catalog, memory, inventory);
    for (unsigned i = 0u; i < 40u; ++i) equipment.InsertEntry(Address(&result), sword);
    Check(result.end - result.begin == 160u && memory.ReadDword(result.begin) == expected.front() &&
        memory.ReadDword(result.end - 4u) == expected.front(),
        "native equipment collector reuses original helper and grows owned skill result storage");
    const auto previous_end = result.end;
    memory.WriteDword(sword + 0x180u, 0xffffffffu);
    equipment.InsertEntry(Address(&result), sword);
    equipment.InsertEntry(Address(&result), 0u);
    Check(result.end == previous_end, "equipment without a matching skill and null entries are skipped");
    (void)memory.Free(result.begin);
    Check(memory.OutstandingAllocations() == 0u, "equipment collection uses result allocator without leaked growth");
    inventory.Close();
    std::cout << "Original passive collection entries=" << expected.size() << '\n';
}
}
int main(int argc, char** argv) {
    Run();
    NativeValues();
    if (argc > 1) OriginalValues(argv[1]);
    std::cout << "PASS: native player collection adapter\n";
}
