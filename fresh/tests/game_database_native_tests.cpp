#include "pal4/native_game_database.h"
#include "pal4/native_player_role_load_runtime.h"
#include "pal4/game_resource_manager.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/native_role_record_image.h"
#include "pal4/native_upgrade_record_catalog.h"
#include "pal4/native_scene_record_catalog.h"
#include "pal4/native_player_event_runtime.h"
#include "pal4/combat_event_data_processor.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_equipment_record_catalog.h"
#include "pal4/native_player_script_refresh_runtime.h"
#include "pal4/player_manager_loader.h"
#include "pal4/player_manager_slot_data_copier.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>

namespace {
void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct RoleLoadHost final : pal4::PlayerManagerLoadRuntimeStub {
    std::uint32_t manager{};
    std::uint32_t actors{};
    std::uint32_t vectors{};
    std::uint32_t postprocess{};
    std::uint32_t cleanup{};

    std::uint32_t Log(std::uint32_t line) override {
        Check(line == 0x2bu || line == 0x49u,
              "native role loader log boundary");
        return line;
    }
    std::uint32_t GameDB() override { return 0x9000u; }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return *reinterpret_cast<const std::uint32_t*>(
            static_cast<std::uintptr_t>(address));
    }
    void CallLoadActor(std::uint32_t slot, std::uint32_t virtual_slot,
                       std::uint32_t name, std::uint32_t model,
                       std::uint32_t enabled) override {
        constexpr std::string_view models[]{"101", "103", "106", "105"};
        const auto index = (slot - manager) /
            pal4::PlayerManagerLoader::kSlotBytes;
        Check(index < 4u &&
                  slot == manager +
                      index * pal4::PlayerManagerLoader::kSlotBytes &&
                  virtual_slot == 0x30u && enabled == 1u &&
                  name == pal4::PlayerManagerLoader::kNames[index] &&
                  std::string_view(reinterpret_cast<const char*>(
                      static_cast<std::uintptr_t>(model))) == models[index],
              "original Role model reaches actor loader");
        ++actors;
        *reinterpret_cast<std::uint32_t*>(slot + 0x14u) = 0x6000u + index;
    }
    void CallVectorMethod(std::uint32_t, std::uint32_t,
                          pal4::PlayerManagerVector&,
                          std::uint32_t option) override {
        Check(option == 0u, "native role vector option");
        ++vectors;
    }
    void ProcessLoadedData(std::uint32_t address) override {
        Check(address == manager, "native role postprocess manager");
        ++postprocess;
    }
    void CleanupScripts() override { ++cleanup; }
};

struct TransformRuntime final : pal4::GameResourceManagerRuntimeStub {
    std::uint32_t frames{};
    std::uint32_t positions{};
    std::uint32_t ClumpFrame(std::uint32_t clump) override {
        Check(clump >= 0x6000u && clump < 0x6004u,
              "Role transform uses loaded clump");
        ++frames;
        return 0x7000u;
    }
    std::uint32_t CalculateCameraPosition(std::uint32_t* destination,
                                          std::uint32_t*,
                                          std::uint32_t frame) override {
        Check(frame == 0x7000u, "Role transform frame forwarding");
        ++positions;
        return static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(destination));
    }
};
}

int main(int argc, char** argv) {
    if (argc != 2) return EXIT_FAILURE;
    pal4::NativeGameDatabase database;
    if (!database.Load(argv[1])) {
        std::cerr << "FAIL: " << database.LastError() << '\n';
        return EXIT_FAILURE;
    }
    Check(database.BytesConsumed() == database.BytesTotal(),
          "database stream reaches EOF");
    Check(database.BytesTotal() == 647273u, "original database byte count");
    Check(database.Properties().size() == 28u, "original property count");
    Check(database.Tables().size() == 28u, "original table count");
    const auto& equipment = database.Tables().at("Equip");
    Check(equipment.fields.size() > 1u &&
              equipment.fields[0].name == "ID" &&
              equipment.fields[1].name == "TYPE",
          "equipment ID and category schema");
    for (const auto [id, category] :
         {std::pair{3501u, 0u}, std::pair{3586u, 4u}}) {
        bool found = false;
        for (const auto& row : equipment.rows) {
            if (std::get<std::uint32_t>(row[0]) != id) continue;
            Check(std::get<std::uint32_t>(row[1]) == category,
                  "new-game equipment category");
            found = true;
            break;
        }
        Check(found, "new-game equipment ID exists");
    }
    pal4::NativeEquipmentRecordCatalog equipment_records(database);
    const auto sword = equipment_records.Address(3501u);
    const auto armor = equipment_records.Address(3586u);
    const auto word_at = [](const std::uint32_t address,
                            const std::uint32_t offset) {
        return *reinterpret_cast<const std::uint32_t*>(
            static_cast<std::uintptr_t>(address + offset));
    };
    Check(sword != 0u && armor != 0u &&
              equipment_records.Address(0u) == 0u &&
              equipment_records.Type(3501u) == 0u &&
              equipment_records.Type(3586u) == 4u &&
              equipment_records.ClassForType(0u) == 0u &&
              equipment_records.ClassForType(4u) == 2u &&
              equipment_records.ClassForType(6u) == 4u &&
              word_at(sword, 144u) == 3501u &&
              word_at(armor, 144u) == 3586u &&
              word_at(sword, 16u) == 10u &&
              word_at(armor, 20u) == 5u &&
              std::string_view(reinterpret_cast<const char*>(
                  static_cast<std::uintptr_t>(word_at(sword, 220u)))) ==
                  "WTA" &&
              std::string_view(reinterpret_cast<const char*>(
                  static_cast<std::uintptr_t>(word_at(armor, 220u)))) ==
                  "YF",
          "original new-game equipment uses IDA Equip record layout");

    std::size_t entries = 0u;
    std::size_t fields = 0u;
    std::size_t rows = 0u;
    for (const auto& [name, property] : database.Properties()) {
        Check(name == property.name, "property keyed by name");
        entries += property.entries.size();
    }
    for (const auto& [name, table] : database.Tables()) {
        Check(name == table.name, "table keyed by name");
        fields += table.fields.size();
        rows += table.rows.size();
        for (const auto& row : table.rows) {
            Check(row.size() == table.fields.size(), "row matches schema");
            for (std::size_t i = 0; i < row.size(); ++i) {
                const auto type = table.fields[i].type;
                Check((type == 1u && std::holds_alternative<std::uint32_t>(row[i])) ||
                          (type == 2u && std::holds_alternative<std::string>(row[i])) ||
                          (type == 3u && std::holds_alternative<float>(row[i])),
                      "row value matches field type");
            }
        }
    }
    Check(entries == 247u && fields == 537u && rows == 4465u,
          "original database record counts");
    const auto& scene_table = database.Tables().at("Scene");
    Check(scene_table.fields.size() == 6u &&
              scene_table.rows.size() == 292u &&
              scene_table.fields[0u].name == "ID" &&
              scene_table.fields[1u].name == "SCENE" &&
              scene_table.fields[2u].name == "SECTION" &&
              scene_table.fields[3u].name == "NAME" &&
              scene_table.fields[4u].name == "TYPE" &&
              scene_table.fields[5u].name == "EARTHBALL",
          "original Scene table schema and row count");
    pal4::NativeSceneRecordCatalog scene_records(database);
    Check(scene_records.Records().size() == scene_table.rows.size(),
          "native Scene catalog owns every original row");
    const auto scene = std::find_if(
        scene_records.Records().begin(), scene_records.Records().end(),
        [](const pal4::GameArenaSceneRecord& record) {
            return record.world == "Q01" && record.scene == "N01";
        });
    Check(scene != scene_records.Records().end() &&
              scene->actor_type == 1u && scene->address != 0u,
          "Q01/N01 resolves to original Scene type 1");
    const auto index = static_cast<std::size_t>(
        scene - scene_records.Records().begin());
    const auto& image = scene_records.Image(index);
    Check(image[0u] == 3u && image[13u] == 1u && image[14u] == 0u &&
              image[2u] != 0u && image[6u] != 0u &&
              std::string_view(reinterpret_cast<const char*>(
                  static_cast<std::uintptr_t>(image[2u]))) == "Q01" &&
              std::string_view(reinterpret_cast<const char*>(
                  static_cast<std::uintptr_t>(image[6u]))) == "N01" &&
              reinterpret_cast<std::uintptr_t>(image.data()) == scene->address,
          "native Scene image preserves IDA field offsets and owned text");
    const auto& upgrades = database.Tables().at("UpgradeData");
    Check(upgrades.fields.size() == 39u && upgrades.rows.size() == 396u &&
              upgrades.fields[0].name == "ID" &&
              upgrades.fields[1].name == "ROLE_ID" &&
              upgrades.fields[2].name == "CA_LEVEL" &&
              upgrades.fields[3].name == "CA_EXPERIENCE" &&
              upgrades.fields[4].name == "CA_MAX_HP" &&
              upgrades.fields[38].name == "CA_PAY_FOR_SPARE",
          "original UpgradeData schema for event state processing");
    for (std::size_t index = 0; index < upgrades.fields.size(); ++index)
        Check(upgrades.fields[index].type == (index < 23u ? 1u : 3u),
              "UpgradeData scalar layout matches x86 event record");
    std::array<std::size_t, 4u> upgrades_per_role{};
    for (const auto& row : upgrades.rows) {
        const auto role_id = std::get<std::uint32_t>(row[1]);
        Check(role_id >= 1001u && role_id <= 1004u,
              "UpgradeData row references a player Role");
        ++upgrades_per_role[role_id - 1001u];
    }
    Check(upgrades_per_role == std::array<std::size_t, 4u>{99u, 99u, 99u, 99u},
          "original UpgradeData has 99 levels for each player Role");
    pal4::NativeUpgradeRecordCatalog upgrade_records(database);
    Check(upgrade_records.Count() == upgrades.rows.size() &&
              upgrade_records.Address() != 0u,
          "native UpgradeData catalog owns the original contiguous records");
    for (std::size_t index = 0u; index < upgrade_records.Count(); ++index) {
        const auto& row = upgrades.rows[index];
        const auto& record = upgrade_records.Record(index);
        Check(record[0u] == 7u &&
                  record[1u] == std::get<std::uint32_t>(row[4u]) &&
                  record[13u] == std::get<std::uint32_t>(row[17u]) &&
                  record[31u] == std::get<std::uint32_t>(row[12u]) &&
                  record[9u] == std::bit_cast<std::uint32_t>(
                      std::get<float>(row[35u])) &&
                  record[19u] == std::bit_cast<std::uint32_t>(
                      std::get<float>(row[29u])) &&
                  record[25u] == std::bit_cast<std::uint32_t>(
                      std::get<float>(row[23u])) &&
                  record[36u] == std::get<std::uint32_t>(row[0u]) &&
                  record[37u] == std::get<std::uint32_t>(row[1u]) &&
                  record[38u] == std::get<std::uint32_t>(row[2u]) &&
                  record[39u] == std::get<std::uint32_t>(row[3u]) &&
                  reinterpret_cast<std::uintptr_t>(&record) ==
                      upgrade_records.Address() + index * 160u,
              "native UpgradeData record preserves RaceStats and event keys");
    }
    pal4::NativeBaseObjectMemory event_memory;
    pal4::NativeInventorySession event_inventory;
    pal4::NativePlayerScriptRefreshRuntime event_refresh(
        event_memory, event_inventory);
    pal4::NativePlayerEventRuntime event_runtime(
        upgrade_records, event_memory, event_refresh);
    std::array<std::uint32_t, 70u> event_target{};
    event_target[0u] = 0x12345678u;
    const auto event_target_address = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(event_target.data()));
    Check(pal4::CombatEventDataProcessor::Process(
              event_target_address, 9999u, 1u, event_runtime) == 0 &&
              event_target[0u] == 0x12345678u && event_target[1u] == 0u,
          "unmatched native event keeps the actor payload intact");
    const auto match = std::find_if(
        upgrades.rows.begin(), upgrades.rows.end(), [](const auto& row) {
            return std::get<std::uint32_t>(row[1u]) == 1001u &&
                   std::get<std::uint32_t>(row[2u]) == 1u;
        });
    Check(match != upgrades.rows.end(),
          "original UpgradeData has the first player level");
    Check(pal4::CombatEventDataProcessor::Process(
              event_target_address, 1001u, 1u, event_runtime) == 0 &&
              event_target[0u] == 0x12345678u &&
              event_target[1u] == std::get<std::uint32_t>((*match)[4u]) &&
              event_target[13u] == std::get<std::uint32_t>((*match)[17u]) &&
              event_target[31u] == std::get<std::uint32_t>((*match)[12u]),
          "matching native event copies first upgrade payload and preserves tag");
    const auto& roles = database.Tables().at("Role");
    Check(roles.fields.size() == 50u && roles.rows.size() == 4u,
          "original player Role table shape");
    constexpr std::string_view models[]{"101", "103", "106", "105"};
    constexpr std::uint32_t hit_points[]{90u, 68u, 45u, 81u};
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto role = database.FindRoleByPlayerIndex(index);
        Check(role && role->id == 1001u + index &&
                  role->model == models[index] &&
                  role->icon == models[index] &&
                  !role->name.empty() && role->fields.size() == 50u &&
                  std::get<std::uint32_t>(role->fields[0]) == role->id,
              "player index resolves original Role record and model");
        std::array<std::uint32_t, 709u> player_slot{};
        pal4::NativePlayerSlotCopyRuntime copy_runtime;
        const auto destination = static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(player_slot.data()));
        std::uint32_t source_model_pointer{};
        {
            auto image = pal4::NativeRoleRecordImage::FromDatabase(
                database, index);
            Check(image && image->Data()[56u] == index &&
                      image->Data()[57u] == role->id &&
                      image->Data()[1u] == hit_points[index] &&
                      image->Data()[25u] ==
                          std::bit_cast<std::uint32_t>(
                              std::get<float>(role->fields[29u])),
                  "original Role row fills 320-byte record fields");
            source_model_pointer = image->Data()[45u];
            Check(pal4::PlayerManagerSlotDataCopier::Copy(
                      destination, image->Address(), copy_runtime) ==
                      destination &&
                      player_slot[(0x658u + 4u) / 4u] == hit_points[index] &&
                      player_slot[(0x9f8u + 80u) / 4u] == index &&
                      player_slot[(0x9f8u + 84u) / 4u] == role->id,
                  "rewritten player slot copier accepts original Role image");
        }
        const auto model_pointer = player_slot[(0x9f8u + 36u) / 4u];
        const auto model_length = player_slot[(0x9f8u + 40u) / 4u];
        Check(model_pointer != 0u && model_pointer != source_model_pointer &&
                  std::string_view(reinterpret_cast<const char*>(
                      static_cast<std::uintptr_t>(model_pointer)),
                      model_length) == models[index],
              "player slot owns model string after Role image destruction");
        const auto sound_pointer = player_slot[(0x9f8u + 100u) / 4u];
        const auto sound_length = player_slot[(0x9f8u + 104u) / 4u];
        Check(sound_pointer != 0u &&
                  std::string_view(reinterpret_cast<const char*>(
                      static_cast<std::uintptr_t>(sound_pointer)),
                      sound_length) ==
                      std::get<std::string>(role->fields[47u]) &&
                  player_slot[(0x9f8u + 160u + 4u) / 4u] == 0u,
              "player slot receives wounded sound and absent fifth sound");
    }
    Check(!database.FindRoleByPlayerIndex(4u) &&
              !pal4::NativeRoleRecordImage::FromDatabase(database, 4u),
          "Role lookup rejects out-of-range player index");
    std::array<std::uint32_t, 709u * 4u> manager_slots{};
    for (std::size_t index = 0u; index < 4u; ++index) {
        const auto slot = index * pal4::PlayerManagerLoader::kSlotBytes;
        manager_slots[(slot + 0x1f0u) / 4u] = 0xffffffffu;
        for (std::size_t word = 40u; word <= 45u; ++word)
            manager_slots[slot / 4u + word] = 0xffffffffu;
        manager_slots[(slot + 0x654u + 83u * 4u) / 4u] = 0xffffffffu;
        manager_slots[(slot + 0x654u + 167u * 4u) / 4u] = 0xffffffffu;
    }
    RoleLoadHost role_host;
    role_host.manager = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(manager_slots.data()));
    pal4::NativePlayerSlotCopyRuntime role_copier;
    TransformRuntime transforms;
    pal4::NativeRoleRecordCatalog role_records(database);
    Check(role_records.Address(4u) == 0u &&
              role_records.Address(0u) != role_records.Address(1u) &&
              role_records.Owns(role_records.Address(0u)) &&
              !role_records.Owns(0u),
          "shared original Role catalog owns four distinct x86 records");
    pal4::NativePlayerRoleLoadRuntime role_runtime(
        role_records, role_host, role_copier, nullptr, &transforms);
    Check(pal4::PlayerManagerLoader::Load(role_host.manager,
                                          role_runtime) == 0x49u &&
              role_host.actors == 4u && role_host.vectors == 0u &&
              transforms.frames == 8u && transforms.positions == 4u &&
              role_host.postprocess == 1u && role_host.cleanup == 1u,
          "rewritten player loader consumes four original Role records");
    for (std::size_t index = 0u; index < 4u; ++index) {
        const auto slot = index * pal4::PlayerManagerLoader::kSlotBytes;
        const auto model = manager_slots[(slot + 0xa1cu) / 4u];
        const auto length = manager_slots[(slot + 0xa20u) / 4u];
        Check(manager_slots[(slot + 0x1f0u) / 4u] == 0u &&
                  manager_slots[(slot + 40u * 4u) / 4u] == 0u &&
                  manager_slots[(slot + 45u * 4u) / 4u] == 0u &&
                  manager_slots[(slot + 0x654u + 83u * 4u) / 4u] == index &&
                  manager_slots[(slot + 0x654u + 167u * 4u) / 4u] == index,
              "Role load writes state flag and both script index fields");
        Check(model != 0u &&
                  std::string_view(reinterpret_cast<const char*>(
                      static_cast<std::uintptr_t>(model)), length) ==
                      models[index],
              "all player model strings survive four-slot load");
    }
    const auto& game_strings = database.Tables().at("GameString");
    Check(game_strings.rows.size() == 315u &&
              game_strings.fields.size() == 2u &&
              game_strings.fields[0].name == "ID" &&
              game_strings.fields[1].name == "STRING",
          "original GameString schema for UI startup");
    for (const auto [id, bytes] : {
             std::pair{309u, 2u}, std::pair{310u, 36u},
             std::pair{311u, 11u}}) {
        const auto value = database.FindTableTextById("GameString", id);
        Check(value && value->size() == bytes,
              "original UI startup string by database ID");
    }
    Check(!database.FindTableTextById("GameString", 9999u) &&
              !database.FindTableTextById("MissingTable", 309u),
          "missing table text has no value");
    Check(database.Load(argv[1]) && database.Properties().size() == 28u &&
              database.Tables().size() == 28u &&
              database.BytesConsumed() == database.BytesTotal() &&
              database.FindRoleByPlayerIndex(0u) &&
              database.FindRoleByPlayerIndex(0u)->model == "101",
          "repeat load resets runtime state");
    std::cout << "PASS: bytes=" << database.BytesConsumed()
              << " properties=" << database.Properties().size()
              << " entries=" << entries
              << " tables=" << database.Tables().size()
              << " fields=" << fields << " rows=" << rows << '\n';
    Check(!database.Load(std::filesystem::path(argv[1]) / "missing") &&
              database.Properties().empty() && database.Tables().empty() &&
              !database.FindRoleByPlayerIndex(0u) &&
              !database.LastError().empty(),
          "failed load clears previous database records");
    Check(scene->world == "Q01" && scene->scene == "N01" &&
              scene_records.Image(index)[0u] == 3u &&
              std::string_view(reinterpret_cast<const char*>(
                  static_cast<std::uintptr_t>(
                      scene_records.Image(index)[6u]))) == "N01",
          "Scene catalog remains valid after source database reload");
    for (const auto* member : {"shopdb.db", "combatDb.db"}) {
        if (!database.Load(argv[1], member)) {
            std::cerr << "FAIL: " << member << ": " << database.LastError()
                      << '\n';
            return EXIT_FAILURE;
        }
        Check(database.BytesConsumed() == database.BytesTotal(),
              "secondary database reaches EOF");
        std::size_t field_count = 0u;
        std::size_t row_count = 0u;
        for (const auto& [name, table] : database.Tables()) {
            Check(name == table.name, "secondary table keyed by name");
            field_count += table.fields.size();
            row_count += table.rows.size();
            for (const auto& row : table.rows) {
                Check(row.size() == table.fields.size(),
                      "secondary row matches schema");
                for (std::size_t i = 0; i < row.size(); ++i) {
                    const auto type = table.fields[i].type;
                    Check((type == 1u && std::holds_alternative<std::uint32_t>(row[i])) ||
                              (type == 2u && std::holds_alternative<std::string>(row[i])) ||
                              (type == 3u && std::holds_alternative<float>(row[i])),
                          "secondary value matches field type");
                }
            }
        }
        const bool shop = std::string_view(member) == "shopdb.db";
        Check(database.BytesTotal() == (shop ? 35706u : 9126u) &&
                  database.Properties().size() == (shop ? 2u : 0u) &&
                  database.Tables().size() == (shop ? 2u : 1u) &&
                  field_count == (shop ? 10u : 2u) &&
                  row_count == (shop ? 899u : 229u),
              "original secondary database record counts");
        if (shop)
            Check(database.Tables().contains("ShopProperty") &&
                      database.Tables().contains("Good") &&
                      database.Properties().contains("ShopType") &&
                      database.Properties().contains("GoodsType"),
                  "shop records satisfy IDA table and property bindings");
        std::cout << "PASS: " << member << " bytes=" << database.BytesTotal()
                  << " properties=" << database.Properties().size()
                  << " tables=" << database.Tables().size()
                  << " fields=" << field_count << " rows=" << row_count
                  << '\n';
    }
}
