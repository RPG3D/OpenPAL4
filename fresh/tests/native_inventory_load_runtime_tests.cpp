#include "pal4/native_inventory_load_runtime.h"
#include "pal4/native_inventory_save_runtime.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_property_record_catalog.h"
#include <windows.h>

#include <algorithm>
#include <bit>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
std::uint32_t Word(const void* data) {
    std::uint32_t value{};
    std::memcpy(&value, data, sizeof(value));
    return value;
}
std::filesystem::path FixtureRoot() {
    return std::filesystem::path("H:/PAL4/objs/inventory-load") /
        std::to_string(GetCurrentProcessId());
}
void WriteFixture(const std::filesystem::path& root, std::span<const std::byte> bytes) {
    std::filesystem::create_directories(root / "save");
    std::ofstream file(root / "save" / "0.dat", std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    Check(static_cast<bool>(file), "write inventory fixture under objs");
}
void EmptyAndFailures() {
    pal4::NativeInventorySession source;
    const auto source_object = source.Start();
    *reinterpret_cast<std::uint32_t*>(source_object + 0x134u) = 67890u;
    pal4::NativeGameSaveArchiveBuffer buffer;
    pal4::NativeInventorySaveRuntime save(source, buffer);
    Check(save.Serialize() == 1u, "generate empty Inventory section");
    const auto root = FixtureRoot();
    WriteFixture(root, buffer.Bytes());
    pal4::NativeInventorySession inventory;
    const auto object = inventory.Start();
    pal4::NativeGameSaveArchiveReader input;
    Check(input.OpenSlot(root, 0), "open Inventory fixture");
    pal4::NativeInventoryLoadRuntime load(inventory, input);
    Check(load.Deserialize() == 1u && input.Remaining() == 0u &&
              inventory.ReadDword(object + 0x134u) == 67890u,
          "restore empty inventory money and consume exact section");
    input.Close();
    auto bad_flag = buffer.Bytes();
    bad_flag[4u] = std::byte{'X'};
    WriteFixture(root, bad_flag);
    Check(input.OpenSlot(root, 0) && load.Deserialize() == 0u &&
              inventory.ReadDword(object + 0x134u) == 67890u,
          "bad Inventory flag returns zero before reset");
    input.Close();
    // Legacy XML version parses, but the original archive contract rejects it.
    pal4::NativeGameSaveArchiveBuffer old_xml;
    Check(old_xml.WriteMarker("IY_SERIALIZE_FLAG"), "write flag");
    pal4::BinaryXmlNode node{"INVENTORY"};
    const auto xml = pal4::BinaryXmlConfig::Serialize(node, 0u, 0u);
    Check(xml && old_xml.WriteBytes(std::as_bytes(std::span(*xml))), "write old XML");
    std::vector<std::byte> old_bytes = old_xml.Bytes();
    old_bytes.resize((std::max)(old_bytes.size(), std::size_t{80u}));
    WriteFixture(root, old_bytes);
    Check(input.OpenSlot(root, 0) && load.Deserialize() == 0u &&
              inventory.ReadDword(object + 0x134u) == 0u,
          "legacy version fails after original reset sequence");
    input.Close();
    WriteFixture(root, std::span(buffer.Bytes()).first(buffer.Bytes().size() - 2u));
    Check(input.OpenSlot(root, 0), "open truncated Inventory");
    bool failed = false;
    try { (void)load.Deserialize(); }
    catch (const std::runtime_error&) { failed = true; }
    Check(failed, "truncated Inventory fails explicitly");
    input.Close();
    WriteFixture(root, buffer.Bytes());
    Check(input.OpenSlot(root, 0) && load.Deserialize() == 1u,
          "reuse same runtime after marker, XML and stream failures");
    input.Close();
    inventory.Close();
    source.Close();
    Check(inventory.OutstandingAllocations() == 0u && source.OutstandingAllocations() == 0u,
          "empty restore and failures release all owned storage");
}

void PropertyCatalog(const pal4::NativeGameDatabase& database) {
    pal4::NativePropertyRecordCatalog catalog(database);
    const auto& table = database.Tables().at("Property");
    Check(catalog.Size() == table.rows.size() && catalog.Address(0xffffffffu) == 0u,
          "complete native Property catalog and missing lookup");
    // Independent IDA offsets for every scalar and text field in the image.
    constexpr std::pair<std::string_view, std::uint32_t> fields[]{
        {"ID", 0u}, {"TYPE", 4u}, {"AI_CMD_TYPE", 8u}, {"TARGET", 12u},
        {"PA_NAME", 16u}, {"PA_DESCRIPTION", 32u}, {"PA_PROPERTY", 48u},
        {"PA_ICON", 64u}, {"PA_MODEL", 80u}, {"PA_TEXTURE", 96u},
        {"PA_ATTACHED_EFFECT", 112u}, {"ePT_TIANHE_CAN_USE", 128u},
        {"ePT_LINSHA_CAN_USE", 132u}, {"ePT_MENGLI_CAN_USE", 136u},
        {"ePT_ZIYING_CAN_USE", 140u}, {"PA_PRICE", 144u},
        {"PA_ATTACHED_SKILL", 148u}, {"PRIZE_LEVEL", 152u},
        {"PA_ATK_TYPE", 156u}, {"CANDROP", 160u}, {"PA_ATK_DELAY", 164u}};
    const auto id_field = std::find_if(table.fields.begin(), table.fields.end(),
        [](const auto& field) { return field.name == "ID"; });
    Check(id_field != table.fields.end(), "Property ID field exists");
    const auto id_column = static_cast<std::size_t>(id_field - table.fields.begin());
    for (const auto& row : table.rows) {
        const auto address = catalog.Address(std::get<std::uint32_t>(row[id_column]));
        Check(address != 0u, "every original Property row resolves");
        for (const auto& [name, offset] : fields) {
            const auto field = std::find_if(table.fields.begin(), table.fields.end(),
                [&](const auto& value) { return value.name == name; });
            Check(field != table.fields.end(), "original Property field exists");
            const auto& value = row[static_cast<std::size_t>(field - table.fields.begin())];
            const auto* image = reinterpret_cast<const std::byte*>(address + offset);
            if (const auto* text = std::get_if<std::string>(&value)) {
                const auto pointer = Word(image + 4u);
                const auto length = Word(image + 8u);
                Check(length == text->size() && (length == 0u ||
                          (pointer != 0u && std::memcmp(reinterpret_cast<void*>(pointer),
                              text->data(), length) == 0 &&
                           *reinterpret_cast<const char*>(pointer + length) == 0)),
                      "all seven original Property text values and terminators match");
            } else {
                const auto expected = std::holds_alternative<float>(value) ?
                    std::bit_cast<std::uint32_t>(std::get<float>(value)) :
                    std::get<std::uint32_t>(value);
                Check(Word(image) == expected, "all original Property scalar values match");
            }
        }
    }
}

std::uint64_t ToInventory(pal4::NativeGameSaveArchiveReader& input) {
    Check(input.ReadMarker().has_value(), "read original save flag");
    pal4::GameSaveHeader header{};
    Check(input.ReadHeader(header) && input.ReadMarker() == "GA_SERIALIZE_FLAG",
          "read original header and GameData marker");
    std::uint32_t count{};
    Check(input.ReadBytes(std::as_writable_bytes(std::span(&count, 1u))) && count == 0u,
          "original GameData native records empty");
    pal4::BinaryXmlConfig config;
    Check(input.ReadConfig(config), "read original GameData XML");
    return input.Position();
}
void OriginalInventory(const char* root) {
    pal4::NativeGameDatabase database;
    Check(database.Load(root), "load original game database");
    PropertyCatalog(database);
    pal4::NativeInventorySession inventory;
    inventory.BindDatabase(database);
    const auto object = inventory.Start();
    pal4::NativeGameSaveArchiveReader input;
    Check(input.OpenSlot(root, 0), "open original save");
    pal4::NativeInventoryLoadRuntime load(inventory, input);
    std::size_t allocations{};
    for (int repeat = 0; repeat < 2; ++repeat) {
        if (repeat != 0) Check(input.OpenSlot(root, 0), "reopen original save");
        const auto begin = ToInventory(input);
        Check(load.Deserialize() == 1u, "restore full original Inventory");
        const auto end = input.Position();
        Check(input.ReadMarker() == "P_MGR_SERIALIZE_FLAG",
              "Inventory ends exactly at PlayerManager marker");
        pal4::NativeGameSaveArchiveBuffer output;
        pal4::NativeInventorySaveRuntime save(inventory, output);
        Check(save.Serialize() == 1u, "reserialize full restored Inventory");
        std::ifstream file(std::filesystem::path(root) / "save" / "0.dat", std::ios::binary);
        file.seekg(begin);
        std::vector<std::byte> bytes(static_cast<std::size_t>(end - begin));
        file.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
        Check(static_cast<bool>(file) && bytes == output.Bytes(),
              "entire original Inventory roundtrips byte-for-byte");
        std::uint32_t items{};
        for (std::uint32_t slot = 0u; slot < 5u; ++slot) {
            const auto member = object + 8u + slot * 16u;
            for (auto cursor = inventory.ReadDword(member + 4u);
                 cursor != inventory.ReadDword(member + 8u); cursor += 12u) {
                const auto record = inventory.ReadDword(cursor);
                Check(record != 0u && inventory.ReadDword(record + 4u) == slot,
                      "restored ordinary item points at full Property record for its category");
                ++items;
            }
        }
        Check(items > 0u, "original save restores nonempty ordinary items");
        if (repeat == 0) allocations = inventory.OutstandingAllocations();
        else Check(inventory.OutstandingAllocations() == allocations,
                   "repeat full restore reuses buffers without leaking records or texts");
        input.Close();
    }
    // Unknown ordinary-item IDs follow the existing codec: consume the record,
    // skip insertion and return the final slot reader's zero result.
    pal4::NativeInventorySession empty;
    (void)empty.Start();
    pal4::NativeGameSaveArchiveBuffer missing;
    pal4::NativeInventorySaveRuntime empty_save(empty, missing);
    Check(empty_save.Serialize() == 1u, "generate missing-ID inventory");
    auto missing_bytes = missing.Bytes();
    const auto record_offset = missing_bytes.size() - 16u; // fifth slot count
    constexpr std::uint32_t one = 1u;
    std::memcpy(missing_bytes.data() + record_offset, &one, sizeof(one));
    const std::array<std::uint32_t, 3u> record{123u, 2u, 0xffffffffu};
    const auto record_bytes = std::as_bytes(std::span(record));
    missing_bytes.insert(missing_bytes.begin() + record_offset + 4u,
                         record_bytes.begin(), record_bytes.end());
    const auto fixture = FixtureRoot();
    WriteFixture(fixture, missing_bytes);
    Check(input.OpenSlot(fixture, 0) && load.Deserialize() == 0u && input.Remaining() == 0u,
          "missing Property ID consumes record without inventing a database object");
    Check(inventory.ReadDword(object + 8u + 4u * 16u + 4u) ==
              inventory.ReadDword(object + 8u + 4u * 16u + 8u),
          "missing Property ID leaves final ordinary slot empty");
    input.Close();
    // Recent count overflows are rejected before allocation/zero fill.
    auto oversized = missing.Bytes();
    constexpr std::uint32_t too_many = 0x40000000u;
    std::memcpy(oversized.data() + oversized.size() - 8u, &too_many, sizeof(too_many));
    WriteFixture(fixture, oversized);
    Check(input.OpenSlot(fixture, 0), "open overflowed ordinary-item recent count");
    bool overflow = false;
    try { (void)load.Deserialize(); }
    catch (const std::length_error&) { overflow = true; }
    Check(overflow, "ordinary-item recent count overflow rejected");
    input.Close();
    empty.Close();
    inventory.Close();
    Check(inventory.OutstandingAllocations() == 0u, "full restored inventory releases nested storage");
}
}  // namespace

int main(const int argc, char** argv) {
    EmptyAndFailures();
    if (argc > 1) OriginalInventory(argv[1]);
    std::cout << "PASS: native Inventory restoration\n";
}
