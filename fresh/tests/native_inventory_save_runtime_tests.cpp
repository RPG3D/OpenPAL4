#include "pal4/native_inventory_save_runtime.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_inventory_equipment_reader.h"
#include "pal4/native_game_save_archive_reader.h"
#include <windows.h>
#include <array>
#include <algorithm>

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

std::uint32_t ReadWord(const std::span<const std::byte> bytes,
                       const std::size_t offset) {
    Check(offset + 4u <= bytes.size(), "inventory save word in range");
    std::uint32_t value{};
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

std::size_t CheckHeader(const std::span<const std::byte> bytes,
                        const std::uint32_t money) {
    constexpr std::string_view marker = "IY_SERIALIZE_FLAG";
    Check(ReadWord(bytes, 0u) == marker.size() &&
              std::memcmp(bytes.data() + 4u, marker.data(),
                          marker.size()) == 0,
          "inventory marker matches original stream");
    const auto config_offset = 4u + marker.size();
    pal4::BinaryXmlConfig config;
    std::size_t config_bytes{};
    const bool parsed = config.ParsePrefix(
              std::span(reinterpret_cast<const std::uint8_t*>(
                            bytes.data() + config_offset),
                        bytes.size() - config_offset), config_bytes);
    Check(parsed &&
              config.Root().name == "INVENTORY" &&
              config.Root().attributes.size() == 1u &&
              config.Root().attributes[0].name == "money" &&
              config.Root().attributes[0].type == 1u &&
              config.Root().attributes[0].ToInt(0u) == money,
          "inventory XML records current money");
    return config_offset + config_bytes;
}

void EmptyInventory() {
    pal4::NativeInventorySession inventory;
    const auto object = inventory.Start();
    *reinterpret_cast<std::uint32_t*>(
        static_cast<std::uintptr_t>(object + 0x134u)) = 12345u;
    pal4::NativeGameSaveArchiveBuffer archive;
    pal4::NativeInventorySaveRuntime runtime(inventory, archive);
    Check(runtime.Serialize() == 1u, "serialize empty native inventory");
    const std::span bytes(archive.Bytes());
    auto offset = CheckHeader(bytes, 12345u);
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        Check(ReadWord(bytes, offset) == 0u,
              "empty inventory category count is zero");
        offset += 4u;
    }
    Check(ReadWord(bytes, offset) == 1u,
          "equipment group writes its base field before slots");
    offset += 4u;
    constexpr std::string_view flag = "EC_SERIALIZE_FLAG";
    for (std::uint32_t index = 0u; index < 7u; ++index) {
        Check(ReadWord(bytes, offset) == flag.size() &&
                  std::memcmp(bytes.data() + offset + 4u,
                              flag.data(), flag.size()) == 0,
              "equipment slot has original serialization flag");
        offset += 4u + flag.size();
        Check(ReadWord(bytes, offset) == 0u,
              "empty equipment slot count is zero");
        offset += 4u;
    }
    Check(ReadWord(bytes, offset) == 0u &&
              ReadWord(bytes, offset + 4u) == 0u,
          "equipment recent array and tree are empty");
    offset += 8u;
    Check(ReadWord(bytes, offset) == 1u,
          "item collection writes its base field before slots");
    offset += 4u;
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        Check(ReadWord(bytes, offset) == 0u,
              "base item slot is empty");
        offset += 8u;
    }
    Check(ReadWord(bytes, offset) == 0u &&
              ReadWord(bytes, offset + 4u) == 0u &&
              offset + 8u == bytes.size(),
          "empty inventory section ends after recent IDs and tree");
    inventory.Close();
    Check(inventory.OutstandingAllocations() == 0u,
          "empty inventory save retains no allocation");
}

void OriginalEquipment(const char* root) {
    pal4::NativeGameDatabase database;
    Check(database.Load(root), "load original equipment database");
    pal4::NativeInventorySession inventory;
    inventory.BindDatabase(database);
    const auto object = inventory.Start();
    const auto sword = inventory.ProcessEquipment(object + 0x7cu, 3501u);
    const auto armor = inventory.ProcessEquipment(object + 0x7cu, 3586u);
    Check(sword != 0u && armor != 0u,
          "populate native inventory with original equipment");
    pal4::NativeGameSaveArchiveBuffer archive;
    pal4::NativeInventorySaveRuntime runtime(inventory, archive);
    Check(runtime.Serialize() == 1u,
          "serialize populated original equipment inventory");
    const std::span bytes(archive.Bytes());
    auto offset = CheckHeader(bytes, inventory.ReadDword(object + 0x134u)) +
                  3u * 4u;
    Check(ReadWord(bytes, offset) == inventory.ReadDword(object + 0x80u),
          "populated equipment group writes its base field");
    offset += 4u;
    constexpr std::string_view flag = "EC_SERIALIZE_FLAG";
    Check(ReadWord(bytes, offset) == flag.size() &&
              std::memcmp(bytes.data() + offset + 4u,
                          flag.data(), flag.size()) == 0,
          "populated first equipment slot starts with original flag");
    offset += 4u + flag.size();
    Check(ReadWord(bytes, offset) == 1u,
          "first equipment slot contains sword snapshot");
    offset += 4u;
    Check(ReadWord(bytes, offset) == inventory.ReadDword(sword + 0x190u) &&
              ReadWord(bytes, offset + 4u) == 3501u,
          "sword sequence and GameDB ID are serialized in source order");
    inventory.Close();
    Check(inventory.OutstandingAllocations() == 0u,
          "populated inventory save retains no allocation");
}

void OriginalInventoryLayout(const char* root) {
    std::ifstream input(std::filesystem::path(root) / "save" / "0.dat",
                        std::ios::binary);
    Check(static_cast<bool>(input), "open original save inventory section");
    const std::vector<std::uint8_t> source{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
    const std::span bytes(reinterpret_cast<const std::byte*>(source.data()),
                          source.size());
    auto offset = 4u + ReadWord(bytes, 0u) + 52u;
    constexpr std::string_view game_flag = "GA_SERIALIZE_FLAG";
    Check(ReadWord(bytes, offset) == game_flag.size(),
          "original GameData flag precedes Inventory");
    offset += 4u + game_flag.size();
    Check(ReadWord(bytes, offset) == 0u,
          "original GameData native records are empty");
    offset += 4u;
    pal4::BinaryXmlConfig config;
    std::size_t size{};
    Check(config.ParsePrefix(std::span(source).subspan(offset), size),
          "parse original GameData config");
    offset += size;
    constexpr std::string_view inventory_flag = "IY_SERIALIZE_FLAG";
    Check(ReadWord(bytes, offset) == inventory_flag.size() &&
              std::memcmp(bytes.data() + offset + 4u,
                          inventory_flag.data(), inventory_flag.size()) == 0,
          "original Inventory flag follows GameData");
    offset += 4u + inventory_flag.size();
    Check(config.ParsePrefix(std::span(source).subspan(offset), size) &&
              config.Root().name == "INVENTORY" &&
              config.Root().FindAttribute("money") != nullptr,
          "parse original Inventory money config");
    offset += size;
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        const auto count = ReadWord(bytes, offset);
        offset += 4u + static_cast<std::size_t>(count) * 4u;
        Check(offset <= bytes.size(), "original category span fits save");
    }
    Check(ReadWord(bytes, offset) == 8u,
          "original equipment group base field precedes seven slots");
    offset += 4u;
    constexpr std::string_view equipment_flag = "EC_SERIALIZE_FLAG";
    for (std::uint32_t slot = 0u; slot < 7u; ++slot) {
        Check(ReadWord(bytes, offset) == equipment_flag.size() &&
                  std::memcmp(bytes.data() + offset + 4u,
                              equipment_flag.data(), equipment_flag.size()) == 0,
              "original equipment slot flag matches codec");
        offset += 4u + equipment_flag.size();
        const auto count = ReadWord(bytes, offset);
        offset += 4u;
        for (std::uint32_t item = 0u; item < count; ++item) {
            offset += 4u + 8u;
            for (std::uint32_t text = 0u; text < 13u; ++text) {
                const auto length = ReadWord(bytes, offset);
                offset += 4u + length;
                Check(offset <= bytes.size(),
                      "original equipment text fits save");
            }
            offset += 10u * 4u + 0x90u;
            Check(offset <= bytes.size(),
                  "original equipment body fits save");
        }
    }
    const auto equipment_recent = ReadWord(bytes, offset);
    offset += 4u + static_cast<std::size_t>(equipment_recent) * 4u;
    Check(ReadWord(bytes, offset) == 0u,
          "original equipment temporary tree is empty");
    offset += 4u;
    Check(ReadWord(bytes, offset) != 0u,
          "original base item field follows equipment region");
    offset += 4u;
    for (std::uint32_t slot = 0u; slot < 5u; ++slot) {
        const auto count = ReadWord(bytes, offset);
        offset += 4u + static_cast<std::size_t>(count) * 12u + 4u;
        Check(offset <= bytes.size(), "original item slot fits save");
    }
    const auto item_recent = ReadWord(bytes, offset);
    offset += 4u + static_cast<std::size_t>(item_recent) * 4u;
    Check(ReadWord(bytes, offset) == 0u && offset + 4u < bytes.size(),
          "original Inventory ends with an empty tree before next section");
    offset += 4u;
    const auto next_length = ReadWord(bytes, offset);
    constexpr std::string_view next_flag = "P_MGR_SERIALIZE_FLAG";
    Check(next_length == next_flag.size() &&
              offset + 4u + next_length <= bytes.size() &&
              std::memcmp(bytes.data() + offset + 4u,
                          next_flag.data(), next_flag.size()) == 0,
          "original Inventory ends at the PlayerManager section marker");
}


void WriteFixture(const std::filesystem::path& root,
                  const std::span<const std::byte> bytes) {
    std::filesystem::create_directories(root / "save");
    std::ofstream output(root / "save" / "0.dat", std::ios::binary);
    output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    Check(static_cast<bool>(output), "write equipment fixture under objs");
}

std::filesystem::path FixtureRoot() {
    return std::filesystem::path("H:/PAL4/objs/inventory-equipment-restore") /
           std::to_string(GetCurrentProcessId());
}

void GeneratedEquipmentRestore() {
    pal4::NativeGameSaveArchiveBuffer source;
    const auto word = [&](const std::uint32_t value) {
        Check(source.WriteBytes(std::as_bytes(std::span(&value, 1u))), "write word");
    };
    word(11u);
    std::size_t truncation{};
    for (std::uint32_t slot = 0u; slot < 7u; ++slot) {
        Check(source.WriteMarker("EC_SERIALIZE_FLAG"), "write slot flag");
        word(slot == 0u ? 2u : 1u);
        for (std::uint32_t entry = 0u; entry < (slot == 0u ? 2u : 1u); ++entry) {
            word(slot * 2u + entry + 1u); // sequence
            word(3500u + slot); // database ID
            word(slot); // type
            for (std::uint32_t text = 0u; text < 13u; ++text) {
                const auto value = std::string("field-") + std::to_string(text);
                Check(source.WriteMarker(value), "write equipment text");
                if (slot == 0u && entry == 0u && text == 2u)
                    truncation = source.Bytes().size() - 2u;
            }
            for (std::uint32_t field = 0u; field < 10u; ++field)
                word(100u + field);
            for (std::uint32_t field = 0u; field < 36u; ++field)
                word(field == 0u ? 7u : 200u + field);
        }
    }
    word(3u); word(1u); word(2u); word(3u); // recent IDs
    word(0u); // tree
    const auto root = FixtureRoot();
    WriteFixture(root, source.Bytes());
    pal4::NativeInventorySession inventory;
    const auto object = inventory.Start();
    pal4::NativeGameSaveArchiveReader input;
    std::size_t allocations{};
    for (int repeat = 0; repeat < 2; ++repeat) {
        Check(input.OpenSlot(root, 0), "open generated equipment stream");
        Check(pal4::NativeInventoryEquipmentReader::Deserialize(inventory, input) == 1u &&
                  input.Remaining() == 0u,
              "restore all seven populated slots to exact end");
        Check(inventory.ReadDword(object + 0x80u) == 11u,
              "restore next equipment sequence");
        const auto restored = inventory.FindInventoryEntry(object + 0x7cu, 2u, 7u);
        Check(restored != 0u && inventory.ReadDword(restored + 0x90u) == 3500u,
              "restored sequence resolves through existing inventory lookup");
        pal4::NativeGameSaveArchiveBuffer output;
        pal4::NativeInventorySaveRuntime save(inventory, output);
        Check(save.Serialize() == 1u, "reserialize restored equipment");
        const auto begin = CheckHeader(output.Bytes(), inventory.ReadDword(object + 0x134u)) + 12u;
        Check(begin + source.Bytes().size() <= output.Bytes().size() &&
                  std::equal(source.Bytes().begin(), source.Bytes().end(),
                             output.Bytes().begin() + begin),
              "all sequences, stats and 13 text fields roundtrip byte-for-byte");
        if (repeat == 0) allocations = inventory.OutstandingAllocations();
        else Check(inventory.OutstandingAllocations() == allocations,
                   "repeat restore replaces records and texts without leaks");
        input.Close();
    }
    const auto preserved = inventory.FindInventoryEntry(object + 0x7cu, 2u, 7u);
    auto bad_flag = source.Bytes();
    bad_flag[8u] = std::byte{'X'};
    WriteFixture(root, bad_flag);
    Check(input.OpenSlot(root, 0), "open bad equipment flag");
    bool bad_flag_failed = false;
    try { (void)pal4::NativeInventoryEquipmentReader::Deserialize(inventory, input); }
    catch (const std::runtime_error&) { bad_flag_failed = true; }
    Check(bad_flag_failed && inventory.FindInventoryEntry(object + 0x7cu, 2u, 7u) == preserved &&
              inventory.OutstandingAllocations() == allocations,
          "bad first-slot flag preserves existing records and allocations");
    input.Close();
    auto oversized = source.Bytes();
    constexpr std::uint32_t overflowing_count = 0x40000000u;
    std::memcpy(oversized.data() + oversized.size() - 20u,
                &overflowing_count, sizeof(overflowing_count));
    WriteFixture(root, oversized);
    Check(input.OpenSlot(root, 0), "open overflowed recent-array count");
    bool overflow_failed = false;
    try { (void)pal4::NativeInventoryEquipmentReader::Deserialize(inventory, input); }
    catch (const std::length_error&) { overflow_failed = true; }
    Check(overflow_failed && inventory.OutstandingAllocations() == allocations,
          "overflowed recent-array count rejects before writing beyond storage");
    input.Close();
    // A failure inside a new record releases its text and record image.
    pal4::NativeInventorySession partial;
    (void)partial.Start();
    WriteFixture(root, std::span(source.Bytes()).first(truncation));
    Check(input.OpenSlot(root, 0), "open truncated equipment");
    bool failed = false;
    try { (void)pal4::NativeInventoryEquipmentReader::Deserialize(partial, input); }
    catch (const std::runtime_error&) { failed = true; }
    Check(failed && partial.OutstandingAllocations() == 1u,
          "truncated equipment frees in-flight record and text");
    input.Close();
    WriteFixture(root, source.Bytes());
    Check(input.OpenSlot(root, 0) &&
              pal4::NativeInventoryEquipmentReader::Deserialize(partial, input) == 1u,
          "same inventory recovers after failed restore");
    input.Close();
    partial.Close();
    inventory.Close();
    Check(partial.OutstandingAllocations() == 0u &&
              inventory.OutstandingAllocations() == 0u,
          "restored native equipment owner frees all persistent allocations");
}

void OriginalEquipmentRestore(const char* root) {
    pal4::NativeGameSaveArchiveReader input;
    Check(input.OpenSlot(root, 0), "open original continuous save stream");
    Check(input.ReadMarker().has_value(), "read original save flag");
    pal4::GameSaveHeader header{};
    Check(input.ReadHeader(header) && input.ReadMarker() == "GA_SERIALIZE_FLAG",
          "read original prefix before GameData");
    std::uint32_t count{};
    Check(input.ReadBytes(std::as_writable_bytes(std::span(&count, 1u))) && count == 0u,
          "original GameData record count is zero");
    pal4::BinaryXmlConfig document;
    Check(input.ReadConfig(document) && input.ReadMarker() == "IY_SERIALIZE_FLAG" &&
              input.ReadConfig(document), "read GameData and Inventory XML");
    for (int slot = 0; slot < 3; ++slot) {
        Check(input.ReadBytes(std::as_writable_bytes(std::span(&count, 1u))),
              "read original category count");
        std::vector<std::byte> category(count * 4u);
        Check(input.ReadBytes(category), "read original category IDs");
    }
    const auto begin = input.Position();
    pal4::NativeGameDatabase database;
    Check(database.Load(root), "load original database for restored inventory");
    pal4::NativeInventorySession inventory;
    inventory.BindDatabase(database);
    const auto object = inventory.Start();
    Check(pal4::NativeInventoryEquipmentReader::Deserialize(inventory, input) == 1u,
          "restore actual original equipment into native inventory");
    const auto end = input.Position();
    Check(inventory.ReadDword(object + 0x80u) == 8u,
          "original equipment next-sequence restored");
    pal4::NativeGameSaveArchiveBuffer output;
    pal4::NativeInventorySaveRuntime save(inventory, output);
    Check(save.Serialize() == 1u, "serialize restored original equipment");
    const auto output_begin = CheckHeader(output.Bytes(), inventory.ReadDword(object + 0x134u)) + 12u;
    std::ifstream original(std::filesystem::path(root) / "save" / "0.dat", std::ios::binary);
    original.seekg(begin);
    std::vector<std::byte> equipment(static_cast<std::size_t>(end - begin));
    original.read(reinterpret_cast<char*>(equipment.data()), equipment.size());
    Check(static_cast<bool>(original) &&
              output_begin + equipment.size() <= output.Bytes().size() &&
              std::equal(equipment.begin(), equipment.end(), output.Bytes().begin() + output_begin),
          "actual original equipment section roundtrips byte-for-byte");
    Check(input.ReadBytes(std::as_writable_bytes(std::span(&count, 1u))) && count != 0u,
          "continuous stream stops at ordinary item base field");
    const auto added = inventory.ProcessEquipment(object + 0x7cu, 3501u);
    Check(added != 0u && inventory.ReadDword(added + 0x190u) == 8u &&
              inventory.FindInventoryEntry(object + 0x7cu, 8u, 7u) == added,
          "restored inventory continues adding original equipment with next sequence");
    bool rebind_failed = false;
    try { inventory.BindDatabase(database); }
    catch (const std::logic_error&) { rebind_failed = true; }
    Check(rebind_failed, "database replacement rejects live restored equipment");
    inventory.Close();
    Check(inventory.OutstandingAllocations() == 0u, "original restored equipment fully released");
}

}  // namespace

int main(const int argc, char** argv) {
    EmptyInventory();
    GeneratedEquipmentRestore();
    if (argc > 1) {
        OriginalEquipment(argv[1]);
        OriginalEquipmentRestore(argv[1]);
        OriginalInventoryLayout(argv[1]);
    }
    std::cout << "PASS: native inventory save runtime\n";
}
