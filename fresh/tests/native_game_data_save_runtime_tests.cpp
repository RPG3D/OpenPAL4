#include "pal4/dialog_manager_lifecycle.h"
#include "pal4/native_game_timer_queue.h"
#include <windows.h>
#include <array>
#include "pal4/native_game_data_persistence_runtime.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_inventory_load_runtime.h"
#include "pal4/native_inventory_session.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Word(const std::span<const std::byte> bytes,
                   const std::size_t offset) {
    Check(offset + 4u <= bytes.size(), "save word is in range");
    std::uint32_t value{};
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

}  // namespace

int main(const int argc, char** argv) {
    pal4::NativePalGameIVTextureStorage game([](std::uint32_t) {});
    const auto game_address = game.Construct();
    pal4::DialogManagerState global_data;
    *reinterpret_cast<std::uint32_t*>(global_data.ImageData() + 0x18u) = 0x28u;
    game.SetGameDataEntry(12, 77u);
    game.SetGameDataEntry(69, 1u);
    *reinterpret_cast<std::uint32_t*>(
        static_cast<std::uintptr_t>(game_address + 0x164u)) = 0x80000001u;
    pal4::NativeGameSaveArchiveBuffer archive;
    bool timer_queried = false;
    pal4::NativeGameDataPersistenceRuntime runtime(
        game, global_data, archive, 0x1234u,
        [&](const std::uint32_t manager) {
            Check(manager == 0x1234u, "timer uses bound UI manager");
            timer_queried = true;
            return 42u;
        });
    Check(runtime.Serialize() == 1u && timer_queried &&
              game.GameDataEntry(70) == 42u,
          "GameData serializer captures current timer duration");

    const std::span bytes(archive.Bytes());
    constexpr std::string_view marker = "GA_SERIALIZE_FLAG";
    Check(Word(bytes, 0u) == marker.size() &&
              std::memcmp(bytes.data() + 4u, marker.data(), marker.size()) == 0,
          "GameData section starts with original marker");
    const auto count_offset = 4u + marker.size();
    Check(Word(bytes, count_offset) == 0u,
          "GameData native record collection is empty");
    pal4::BinaryXmlConfig config;
    std::size_t consumed{};
    Check(config.ParsePrefix(std::span(
                                 reinterpret_cast<const std::uint8_t*>(
                                     bytes.data() + count_offset + 4u),
                                 bytes.size() - count_offset - 4u),
                             consumed) &&
              consumed == bytes.size() - count_offset - 4u &&
              config.Root().name == "gameData" &&
              config.Root().attributes.size() == 75u,
          "live GameData serializes a complete binary XML node");
    const auto& root = config.Root();
    Check(root.FindAttribute("ElapsedTime")->ToInt(0u) == 0u &&
              root.FindAttribute("MVar")->ToInt(0u) == 0u &&
              root.FindAttribute("MovieFlag")->ToInt(0u) == 0x28u &&
              root.FindAttribute("dataInt12")->ToInt(0u) == 77u &&
              root.FindAttribute("dataInt69")->ToInt(0u) == 1u &&
              root.FindAttribute("dataInt70")->ToInt(0u) == 42u &&
              std::get<std::string>(root.FindAttribute("TUTORIAL")->value)
                      .size() == 128u,
          "live values and tutorial bits are present");

    const auto fixture_root = std::filesystem::path("H:/PAL4/objs/game-data-restore") /
        std::to_string(GetCurrentProcessId());
    std::filesystem::create_directories(fixture_root / "save");
    const auto write_fixture = [&](int slot, std::span<const std::byte> data) {
        std::ofstream output(fixture_root / "save" / (std::to_string(slot) + ".dat"), std::ios::binary);
        output.write(reinterpret_cast<const char*>(data.data()), data.size());
        Check(static_cast<bool>(output), "write GameData restore fixture under objs");
    };
    write_fixture(0, bytes);
    std::array<std::uint32_t, 0x224u / 4u> manager{};
    pal4::NativeGameTimerQueue timers([](std::uint32_t, std::uint32_t) {});
    manager[0x1a0u / 4u] = timers.Construct();
    const auto manager_address = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(manager.data()));
    pal4::NativePalGameIVTextureStorage restored([](std::uint32_t) {});
    const auto restored_address = restored.Construct({&timers, manager_address, {}});
    pal4::DialogManagerState restored_globals;
    *reinterpret_cast<std::uint32_t*>(restored_globals.ImageData() + 0x18u) = 0x80000000u;
    alignas(4) std::array<std::byte, 0x658u> pet{};
    pal4::NativeGameSaveArchiveReader input;
    Check(input.OpenSlot(fixture_root, 0), "open native GameData section");
    pal4::NativeGameDataPersistenceRuntime load(restored, restored_globals, input, pet);
    const auto restore_and_collect = [&] {
        const auto ok = load.Deserialize();
        // Original timer removal clears callbacks; the next nonzero tick
        // unlinks their nodes. Count includes these pending removals.
        timers.Update(0.001f);
        return ok;
    };
    const auto restored_ok = restore_and_collect();
    if (!restored_ok || input.Remaining() || timers.Count() != 1u)
        std::cerr << "GameData restore result=" << restored_ok << " remaining=" << input.Remaining()
                  << " timers=" << timers.Count() << " error=" << input.LastError() << '\n';
    Check(restored_ok && input.Remaining() == 0u && timers.Count() == 1u,
            "shared GameData deserializer restores native owner and replaces timer once");
    for (std::uint32_t i = 0u; i < 71u; ++i) {
        const auto actual = *reinterpret_cast<const std::uint32_t*>(static_cast<std::uintptr_t>(restored_address + 8u + i * 4u));
        Check(actual == root.FindAttribute("dataInt" + std::to_string(i))->ToInt(99u),
                "all 71 GameData integers restore from saved XML");
    }
    Check(*reinterpret_cast<const std::uint32_t*>(restored_globals.ImageData() + 0x18u) == 0x80000028u &&
            *reinterpret_cast<const std::uint32_t*>(pet.data() + 0x654u) == 1u &&
            *reinterpret_cast<const std::uint32_t*>(static_cast<std::uintptr_t>(restored_address + 0x164u)) == 0x80000001u,
            "GameData ORs movie flags, restores pet visibility and tutorial bits");
    for (std::uint32_t i = 0u; i < 47u; ++i) {
        const auto local = i < 5u ? i + 21u : i + 22u;
        Check(*reinterpret_cast<const std::uint32_t*>(static_cast<std::uintptr_t>(restored.GlobalAddress(0x90dec4u + i * 4u))) ==
                root.FindAttribute("dataInt" + std::to_string(local))->ToInt(99u),
                "47 shared game globals restore through native mapped addresses");
    }
    pal4::NativeGameSaveArchiveBuffer invalid;
    Check(invalid.WriteMarker("WRONG_GAME_DATA_MARKER") && invalid.WriteConfig(root), "bad marker fixture");
    write_fixture(1, invalid.Bytes());
    restored.SetGameDataEntry(12, 123456u);
    Check(input.OpenSlot(fixture_root, 1) && !restore_and_collect() && restored.GameDataEntry(12) == 123456u && timers.Count() == 1u,
            "bad GameData marker does not initialize or alter native state");
    pal4::NativeGameSaveArchiveBuffer records;
    const std::array<std::uint32_t,4u> record_values{42u,0xffffffffu,42u,0u};
    const std::uint32_t nonempty_count = record_values.size();
    Check(records.WriteMarker("GA_SERIALIZE_FLAG") &&
            records.WriteBytes(std::as_bytes(std::span(&nonempty_count, 1u))) &&
            records.WriteBytes(std::as_bytes(std::span(record_values))) && records.WriteConfig(root),
            "nonempty record fixture");
    write_fixture(2, records.Bytes());
    Check(input.OpenSlot(fixture_root, 2), "open record boundary fixture");
    Check(restore_and_collect() && input.Remaining()==0u && restored.GameDataEntry(12)==77u && timers.Count()==1u,
          "temporary record set consumes duplicates and signed-bit values before XML");
    // Keep the archive's minimum header-sized file envelope while truncating
    // the declared record list itself before it can reach any XML.
    std::vector<std::byte> truncated_records(records.Bytes().begin(),records.Bytes().begin()+64u);
    const std::int32_t truncated_count=100;
    std::memcpy(truncated_records.data()+count_offset,&truncated_count,sizeof(truncated_count));
    write_fixture(4,truncated_records);
    Check(input.OpenSlot(fixture_root,4),"open truncated temporary record set");
    bool records_rejected = false;
    try { (void)load.Deserialize(); } catch (const std::runtime_error&) { records_rejected = true; }
    timers.Update(0.001f);
    Check(records_rejected && timers.Count()==1u,
          "truncated record list throws and releases local set storage");
    pal4::NativeGameSaveArchiveBuffer negative_records;
    const std::int32_t negative_count=-1;
    Check(negative_records.WriteMarker(marker) &&
          negative_records.WriteBytes(std::as_bytes(std::span(&negative_count,1u))) &&
          negative_records.WriteConfig(root),"negative signed record count fixture");
    write_fixture(5,negative_records.Bytes());
    Check(input.OpenSlot(fixture_root,5) && restore_and_collect() && input.Remaining()==0u,
          "original signed loop consumes no records for negative count");
    pal4::NativeGameSaveArchiveBuffer wrong_version;
    const auto legacy_version = pal4::BinaryXmlConfig::Serialize(root, 0u, 0u);
    const std::uint32_t zero_count = 0u;
    Check(legacy_version && wrong_version.WriteMarker("GA_SERIALIZE_FLAG") &&
            wrong_version.WriteBytes(std::as_bytes(std::span(&zero_count, 1u))) &&
            wrong_version.WriteBytes(std::as_bytes(std::span(*legacy_version))), "wrong XML version fixture");
    write_fixture(3, wrong_version.Bytes());
    Check(input.OpenSlot(fixture_root, 3) && !restore_and_collect() && timers.Count() == 1u,
            "wrong XML version fails with normal native cleanup");
    Check(input.OpenSlot(fixture_root, 0) && restore_and_collect() && restored.GameDataEntry(12) == 77u,
            "same native runtime recovers after marker, record and XML failures");
    input.Close();
    if (argc > 1) {
        std::ifstream schema_input(std::filesystem::path(argv[1]) / "save" / "0.dat",
                            std::ios::binary);
        Check(static_cast<bool>(schema_input), "open original save schema");
        const std::vector<std::uint8_t> original{
            std::istreambuf_iterator<char>(schema_input),
            std::istreambuf_iterator<char>()};
        const auto prefix_marker_size =
            Word(std::as_bytes(std::span(original)), 0u);
        auto offset = 4u + prefix_marker_size + 52u;
        const auto section_marker_size =
            Word(std::as_bytes(std::span(original)), offset);
        offset += 4u + section_marker_size;
        Check(Word(std::as_bytes(std::span(original)), offset) == 0u,
              "original GameData native record collection is empty");
        offset += 4u;
        pal4::BinaryXmlConfig original_config;
        std::size_t original_size{};
        Check(original_config.ParsePrefix(
                  std::span(original).subspan(offset), original_size) &&
                  original_config.Root().attributes.size() ==
                      root.attributes.size(),
              "original and live GameData attribute counts match");
        for (std::size_t i = 0u; i < root.attributes.size(); ++i)
            Check(root.attributes[i].name ==
                      original_config.Root().attributes[i].name &&
                      root.attributes[i].type ==
                          original_config.Root().attributes[i].type,
                  "original and live GameData schema order matches");
        Check(input.OpenSlot(argv[1], 0) && input.ReadMarker().has_value(), "open original continuous restore input");
        pal4::GameSaveHeader original_header;
        Check(input.ReadHeader(original_header) && restore_and_collect() && timers.Count() == 1u,
                "original slot zero GameData restores through shared native runtime");
        for (std::uint32_t i = 0u; i < 71u; ++i) {
            const auto actual = *reinterpret_cast<const std::uint32_t*>(static_cast<std::uintptr_t>(restored_address + 8u + i * 4u));
            Check(actual == original_config.Root().FindAttribute("dataInt" + std::to_string(i))->ToInt(99u),
                    "original GameData values match restored native fields");
        }
        Check(input.Position() == offset + original_size,
              "GameData restore leaves stream exactly at Inventory");
        pal4::NativeGameDatabase database;
        Check(database.Load(argv[1]), "load database for continuous Inventory restore");
        pal4::NativeInventorySession inventory;
        inventory.BindDatabase(database);
        (void)inventory.Start();
        pal4::NativeInventoryLoadRuntime inventory_load(inventory, input);
        Check(inventory_load.Deserialize() == 1u &&
                  input.ReadMarker() == "P_MGR_SERIALIZE_FLAG",
              "one original stream restores GameData then Inventory to PlayerManager");
        inventory.Close();
        Check(inventory.OutstandingAllocations() == 0u,
              "continuous restore releases inventory allocations");

    }
    restored.Destroy();
    timers.Update(0.001f);
    Check(timers.Count() == 0u && restored.CoreAllocationCount() == 0u,
            "restored native game releases replacement timer and core storage");
    game.Destroy();
    std::cout << "PASS: native GameData persistence and original restore runtime\n";
}
