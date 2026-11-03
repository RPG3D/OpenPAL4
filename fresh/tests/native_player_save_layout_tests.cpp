#include "pal4/binary_xml_config.h"
#include "pal4/native_arena_config_save_runtime.h"
#include "pal4/native_arena_cache_save_section.h"
#include "pal4/native_arena_cache_owner.h"
#include "pal4/game_data_resource_names.h"
#include "pal4/native_host_server_save_runtime.h"
#include "pal4/native_player_basic_field_schema.h"
#include "pal4/native_player_basic_save_runtime.h"
#include "pal4/native_player_basic_load_runtime.h"
#include "pal4/native_player_script_property_runtime.h"
#include "pal4/native_player_load_fields.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/native_xml_attribute_text.h"
#include "pal4/native_player_equipment_save_text.h"
#include "pal4/native_inventory_session.h"
#include "pal4/native_player_field_schema.h"
#include "pal4/native_player_save_runtime.h"
#include "pal4/native_player_manager_save_runtime.h"
#include "pal4/player_manager_object.h"

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Word(const std::span<const std::uint8_t> bytes,
                   const std::size_t offset) {
    Check(offset + 4u <= bytes.size(), "player save word in range");
    std::uint32_t value{};
    std::memcpy(&value, bytes.data() + offset, 4u);
    return value;
}

std::size_t MarkerEnd(const std::span<const std::uint8_t> bytes,
                      const std::size_t offset, const std::string_view marker) {
    Check(Word(bytes, offset) == marker.size() &&
              offset + 4u + marker.size() <= bytes.size() &&
              std::memcmp(bytes.data() + offset + 4u, marker.data(),
                          marker.size()) == 0,
          "player save marker matches IDA order");
    return offset + 4u + marker.size();
}

std::size_t ConfigEnd(const std::span<const std::uint8_t> bytes,
                      const std::size_t offset, const std::string_view root,
                      pal4::BinaryXmlConfig& config) {
    std::size_t consumed{};
    Check(config.ParsePrefix(bytes.subspan(offset), consumed) &&
              config.Root().name == root && consumed != 0u,
          "player save config parses with expected root");
    return offset + consumed;
}

std::size_t FindMarker(const std::span<const std::uint8_t> bytes,
                       const std::string_view marker) {
    for (std::size_t offset = 0u; offset + 4u + marker.size() <= bytes.size();
         ++offset) {
        if (Word(bytes, offset) == marker.size() &&
            std::memcmp(bytes.data() + offset + 4u, marker.data(),
                        marker.size()) == 0)
            return offset;
    }
    Check(false, "player manager marker present in original save");
    return 0u;
}

void CheckLivePlayerSlot(pal4::NativeInventorySession& inventory,
                         const std::uint32_t sword_sequence,
                         const std::string& sword_name) {
    pal4::PlayerManagerSlotState image{};
    const auto fields = pal4::NativePlayerFieldSchema::Build();
    const std::string role_text = "live role text";
    for (const auto& field : fields) {
        auto* destination = reinterpret_cast<std::byte*>(&image) +
                            field.slot_offset;
        if (field.type == 3u && !field.name.starts_with("ERES_")) {
            const auto data = static_cast<std::uint32_t>(
                reinterpret_cast<std::uintptr_t>(role_text.data()));
            const auto length = static_cast<std::uint32_t>(role_text.size());
            std::memcpy(destination + 4u, &data, 4u);
            std::memcpy(destination + 8u, &length, 4u);
        } else {
            const std::uint32_t raw = field.type == 2u
                                          ? std::bit_cast<std::uint32_t>(1.5f)
                                          : field.name == "ERES_WEAPON"
                                                ? sword_sequence
                                                : field.type == 3u ? 0u : 1234u;
            std::memcpy(destination, &raw, 4u);
        }
    }
    const std::int32_t skills[]{42, -7};
    const auto begin = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(skills));
    const auto end = begin + sizeof(skills);
    std::memcpy(reinterpret_cast<std::byte*>(&image) + 0x9ecu,
                &begin, 4u);
    std::memcpy(reinterpret_cast<std::byte*>(&image) + 0x9f0u,
                &end, 4u);
    pal4::NativeGameSaveArchiveBuffer archive;
    pal4::NativePlayerSaveRuntime writer(archive);
    Check(writer.SerializeSlot(static_cast<std::uint32_t>(
              reinterpret_cast<std::uintptr_t>(&image)), inventory),
          "Player writer serializes a live slot");
    pal4::BinaryXmlConfig parsed;
    Check(parsed.Parse(std::span(
              reinterpret_cast<const std::uint8_t*>(archive.Bytes().data()),
              archive.Bytes().size())),
          "live Player document parses");
    for (const auto& field : fields) {
        const auto* value = parsed.Root().FindAttribute(field.name);
        Check(value && value->type == field.type,
              "live Player field name and type");
        if (field.name == "ERES_WEAPON") {
            Check(std::get<std::string>(value->value) ==
                      std::to_string(static_cast<std::int32_t>(sword_sequence)) +
                          " " + sword_name,
                  "live Player weapon uses inventory instance name");
        } else if (field.type == 3u) {
            Check(std::get<std::string>(value->value) ==
                      (field.name.starts_with("ERES_") ? "0..." : role_text),
                  "live Player text resolves by owning subobject");
        } else if (field.type == 2u) {
            Check(std::get<float>(value->value) == 1.5f,
                  "live Player float reads script field");
        } else {
            Check(std::get<std::int32_t>(value->value) == 1234,
                  "live Player integer reads mapped field");
        }
    }
    Check(parsed.Root().children.size() == 1u &&
              parsed.Root().children[0].children.size() == 2u &&
              parsed.Root().children[0].children[0].name == "42" &&
              parsed.Root().children[0].children[1].name == "-7",
          "live Player forbidden skills preserve slot order");
}

void CheckOriginal(const std::filesystem::path& root) {
    pal4::NativeGameDatabase database;
    Check(database.Load(root), "open original equipment database");
    pal4::NativeInventorySession inventory;
    inventory.BindDatabase(database);
    const auto inventory_address = inventory.Start();
    const auto sword = inventory.ProcessEquipment(
        inventory_address + 0x7cu, 3501u);
    Check(sword != 0u, "original sword inventory snapshot exists");
    const auto sword_sequence = inventory.ReadDword(sword + 400u);
    std::string sword_name;
    for (const auto& row : database.Tables().at("Equip").rows) {
        if (std::get<std::uint32_t>(row[0u]) == 3501u) {
            sword_name = std::get<std::string>(row[2u]);
            break;
        }
    }
    Check(!sword_name.empty(), "original sword name exists");
    Check(pal4::NativePlayerEquipmentSaveText::FormatFromInventory(
              inventory, static_cast<std::int32_t>(sword_sequence)) ==
              std::to_string(static_cast<std::int32_t>(sword_sequence)) +
                  " " + sword_name,
          "live inventory equipment name is saved by instance sequence");
    Check(pal4::NativePlayerEquipmentSaveText::FormatFromInventory(
              inventory, -1) == "-1...",
          "missing inventory instance retains original suffix");
    CheckLivePlayerSlot(inventory, sword_sequence, sword_name);
    pal4::PlayerManagerObjectState live_manager{};
    pal4::NativeGameSaveArchiveBuffer live_archive;
    pal4::NativePlayerManagerSaveRuntime live_writer(live_archive);
    Check(live_writer.Serialize(static_cast<std::uint32_t>(
              reinterpret_cast<std::uintptr_t>(&live_manager)), inventory),
          "live player manager serializes four slots");
    const auto live_bytes = std::span(
        reinterpret_cast<const std::uint8_t*>(live_archive.Bytes().data()),
        live_archive.Bytes().size());
    auto live_offset = MarkerEnd(live_bytes, 0u, "P_MGR_SERIALIZE_FLAG");
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        live_offset = MarkerEnd(live_bytes, live_offset,
                                "PR_SERIALIZE_FLAG");
        pal4::BinaryXmlConfig live_basic;
        pal4::BinaryXmlConfig live_player;
        live_offset = ConfigEnd(live_bytes, live_offset, "BASIC", live_basic);
        live_offset = ConfigEnd(live_bytes, live_offset, "Player", live_player);
        Check(live_basic.Root().attributes.size() == 84u &&
                  live_player.Root().attributes.size() == 114u,
              "live manager emits complete slot documents");
        live_offset = MarkerEnd(live_bytes, live_offset,
                                "MGC_HOST_SER_FLAG");
        for (std::uint32_t collection = 0u; collection < 11u;
             ++collection) {
            Check(Word(live_bytes, live_offset) == 0u,
                  "empty live HostServer collection count");
            live_offset += 4u;
        }
    }
    pal4::BinaryXmlConfig live_config;
    live_offset = ConfigEnd(live_bytes, live_offset, "P_MGR", live_config);
    Check(live_offset == live_bytes.size() &&
              live_config.Root().attributes.size() == 12u,
          "live manager section ends after P_MGR");
    inventory.Close();
    std::ifstream input(root / "save" / "0.dat", std::ios::binary);
    Check(static_cast<bool>(input), "open original player save");
    const std::vector<std::uint8_t> data{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
    const std::span bytes(data);
    const auto manager_section_start = FindMarker(bytes, "P_MGR_SERIALIZE_FLAG");
    auto offset = MarkerEnd(bytes, manager_section_start,
                            "P_MGR_SERIALIZE_FLAG");
    using Field = std::pair<std::string, std::uint32_t>;
    std::vector<Field> basic_schema;
    std::vector<Field> player_schema;
    const auto fields = pal4::NativePlayerBasicFieldSchema::Build();
    const auto player_fields = pal4::NativePlayerFieldSchema::Build();
    Check(fields.size() == 84u,
          "rewritten BASIC registry has original field count");
    Check(player_fields.size() == 114u,
          "rewritten Player registry has original field count");
    std::vector<std::uint32_t> script_offsets;
    for (const auto& field : player_fields) {
        if (field.slot_offset >= 0x654u + 85u * 4u &&
            field.slot_offset <= 0x654u + 168u * 4u)
            script_offsets.push_back(field.slot_offset);
    }
    std::sort(script_offsets.begin(), script_offsets.end());
    Check(script_offsets.size() == 84u,
          "Player has exactly 84 script-backed properties");
    for (std::uint32_t index = 0u; index < 84u; ++index)
        Check(script_offsets[index] == 0x654u + (85u + index) * 4u,
              "script getter offsets cover all 84 distinct words");
    for (std::uint32_t index = 0u; index < 8u; ++index) {
        const auto name = pal4::GameDataResourceNames::NameAt(
            0x8ee4f8u + index * 16u);
        Check(name.has_value(), "original combat field name exists");
        const auto found = std::find_if(player_fields.begin(),
                                        player_fields.end(), [&](const auto& field) {
                                            return field.name == *name;
                                        });
        Check(found != player_fields.end() &&
                  found->slot_offset == 0x654u + 0x158u + index * 4u,
              "Player combat stat uses script getter offset");
    }
    for (std::uint32_t index = 0u; index < 10u; ++index) {
        const auto name = pal4::GameDataResourceNames::NameAt(
            0x8ee3b8u + index * 16u);
        Check(name.has_value(), "original progression field name exists");
        const auto found = std::find_if(player_fields.begin(),
                                        player_fields.end(), [&](const auto& field) {
                                            return field.name == *name;
                                        });
        Check(found != player_fields.end() &&
                  found->slot_offset == 0x654u + 0x220u + index * 4u,
              "Player progression stat uses script getter offset");
    }
    for (std::uint32_t player = 0u; player < 4u; ++player) {
        offset = MarkerEnd(bytes, offset, "PR_SERIALIZE_FLAG");
        const auto basic_start = offset;
        pal4::BinaryXmlConfig basic;
        offset = ConfigEnd(bytes, offset, "BASIC", basic);
        pal4::PlayerManagerSlotState basic_image{};
        pal4::NativePlayerBasicLoadRuntime::Apply(static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(&basic_image)), basic.Root());
        for (std::size_t index = 0u; index < fields.size(); ++index) {
            const auto& field = fields[index];
            const auto& attribute = basic.Root().attributes[index];
            Check(field.name == attribute.name &&
                      field.type == attribute.type &&
                      field.slot_offset + 4u <=
                          pal4::PlayerManagerObject::kElementBytes,
                  "rewritten BASIC registry matches original name and type");
            std::uint32_t raw{};
            if (field.type == 2u) {
                const auto* number = std::get_if<float>(&attribute.value);
                Check(number != nullptr, "BASIC float attribute");
                raw = std::bit_cast<std::uint32_t>(*number);
            } else {
                const auto* number =
                    std::get_if<std::int32_t>(&attribute.value);
                Check(number != nullptr, "BASIC integer attribute");
                raw = std::bit_cast<std::uint32_t>(*number);
            }
            std::uint32_t restored{};
            std::memcpy(&restored, reinterpret_cast<const std::byte*>(&basic_image) +
                             field.slot_offset, sizeof(restored));
            Check(restored == raw, "native BASIC loader restores every original field");
        }
        pal4::NativeGameSaveArchiveBuffer basic_rebuilt;
        pal4::NativePlayerBasicSaveRuntime basic_writer(basic_rebuilt);
        Check(basic_writer.Serialize(static_cast<std::uint32_t>(
                  reinterpret_cast<std::uintptr_t>(&basic_image))) &&
                  basic_rebuilt.Bytes().size() == offset - basic_start &&
                  std::memcmp(basic_rebuilt.Bytes().data(),
                              bytes.data() + basic_start,
                              basic_rebuilt.Bytes().size()) == 0,
              "native BASIC writer matches original slot bytes");
        const auto player_start = offset;
        pal4::BinaryXmlConfig player_data;
        offset = ConfigEnd(bytes, offset, "Player", player_data);
        const auto restored_slot = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&basic_image));
        const auto restored_script = restored_slot + 0x654u;
        pal4::NativePlayerSlotCopyRuntime text_owner;
        pal4::NativePlayerLoadFields::Apply(restored_slot, player_data.Root(), text_owner);
        for (const auto& field : player_fields) {
            if (field.slot_offset < 0x9f8u && !field.name.starts_with("ERES_")) continue;
            const auto* attribute = player_data.Root().FindAttribute(field.name);
            Check(attribute != nullptr, "original Role/equipment/direct attribute present");
            const auto location = restored_slot + field.slot_offset;
            if (field.type == 3u && !field.name.starts_with("ERES_")) {
                const auto data = text_owner.ReadDword(location + 4u);
                const auto expected = std::get<std::string>(attribute->value);
                Check((data ? std::string(reinterpret_cast<const char*>(data)) : std::string{}) == expected &&
                          text_owner.ReadDword(location + 8u) == expected.size(),
                      "native loader restores original Role text and length");
            } else {
                const auto expected = field.name.starts_with("ERES_")
                    ? static_cast<std::uint32_t>(std::atoi(std::get<std::string>(attribute->value).c_str()))
                    : attribute->ToInt(0u);
                Check(text_owner.ReadDword(location) == expected,
                      "native loader restores original equipment IDs and Role/direct scalars");
            }
        }
        // Original43FE10 explicitly reapplies HP/MP after all properties.
        for (const auto [name, property] : {std::pair{"CA_HP", 7u}, std::pair{"CA_MP", 9u}}) {
            const auto* attribute = player_data.Root().FindAttribute(name);
            (void)pal4::NativePlayerScriptPropertyRuntime::Set(restored_script, 0x44u, property,
                attribute ? attribute->ToInt(0u) : 0u);
        }
        const auto bindings = pal4::NativePlayerFieldSchema::ScriptBindings();
        for (const auto& binding : bindings) {
            const auto field = std::find_if(player_fields.begin(), player_fields.end(), [&](const auto& item) {
                return item.name == binding.name;
            });
            Check(field != player_fields.end(), "script input has matching getter field");
            const auto* attribute = player_data.Root().FindAttribute(binding.name);
            Check(attribute != nullptr, "original script input field present");
            const auto expected = field->type == 2u
                ? std::bit_cast<std::uint32_t>(attribute->ToFloat(0.0f)) : attribute->ToInt(0u);
            std::uint32_t actual{};
            std::memcpy(&actual, reinterpret_cast<const std::byte*>(&basic_image) + field->slot_offset, 4u);
            if (actual != expected)
                std::cerr << "script input mismatch: " << field->name << " actual=" << actual << " expected=" << expected << '\n';
            Check(actual == expected, "native scalar script setter restores original Player attribute");
        }
        std::vector<Field> current_basic;
        std::vector<Field> current_player;
        for (const auto& attribute : basic.Root().attributes)
            current_basic.emplace_back(attribute.name, attribute.type);
        for (const auto& attribute : player_data.Root().attributes)
            current_player.emplace_back(attribute.name, attribute.type);
        for (std::size_t index = 0u; index < player_fields.size(); ++index)
            Check(player_fields[index].name ==
                      player_data.Root().attributes[index].name &&
                      player_fields[index].type ==
                          player_data.Root().attributes[index].type &&
                      player_fields[index].slot_offset <
                          pal4::PlayerManagerObject::kElementBytes,
                  "rewritten Player registry matches original name and type");
        for (const auto& attribute : player_data.Root().attributes) {
            if (!attribute.name.starts_with("ERES_")) continue;
            const auto* saved = std::get_if<std::string>(&attribute.value);
            Check(saved != nullptr, "original equipment field is text");
            std::int32_t id{};
            const auto parsed = std::from_chars(
                saved->data(), saved->data() + saved->size(), id);
            Check(parsed.ec == std::errc{} && parsed.ptr != saved->data(),
                  "original equipment text begins with signed ID");
            const auto suffix = std::string_view(parsed.ptr,
                static_cast<std::size_t>(saved->data() + saved->size() -
                                         parsed.ptr));
            const auto name = suffix == "..."
                                  ? std::nullopt
                                  : std::optional<std::string_view>{
                                        suffix.starts_with(' ') ?
                                            suffix.substr(1u) :
                                            std::string_view{}};
            Check((suffix == "..." || suffix.starts_with(' ')) &&
                      pal4::NativePlayerEquipmentSaveText::Format(id, name) ==
                          *saved,
                  "native equipment text matches original Player attribute");
        }
        Check(current_basic.size() == 84u &&
                  current_player.size() == 114u &&
                  player_data.Root().children.size() == 1u &&
                  player_data.Root().children[0].name == "ForbidenSkill",
              "original slot document schemas and forbidden-skill node");
        std::vector<std::int32_t> forbidden_skills;
        for (const auto& child : player_data.Root().children[0].children) {
            Check(child.attributes.empty() && child.children.empty(),
                  "original forbidden-skill entry is a leaf");
            forbidden_skills.push_back(std::stoi(child.name));
        }
        pal4::NativeGameSaveArchiveBuffer player_rebuilt;
        pal4::NativePlayerSaveRuntime player_writer(player_rebuilt);
        Check(player_writer.Serialize(
                  [&](const pal4::NativePlayerBasicField& field)
                      -> std::optional<pal4::BinaryXmlAttribute::Value> {
                      const auto* attribute =
                          player_data.Root().FindAttribute(field.name);
                      if (!attribute || attribute->type != field.type)
                          return std::nullopt;
                      return attribute->value;
                  }, forbidden_skills) &&
                  player_rebuilt.Bytes().size() == offset - player_start &&
                  std::memcmp(player_rebuilt.Bytes().data(),
                              bytes.data() + player_start,
                              player_rebuilt.Bytes().size()) == 0,
              "native Player document writer matches original slot bytes");
        if (player == 0u) {
            basic_schema = std::move(current_basic);
            player_schema = std::move(current_player);
        } else {
            Check(current_basic == basic_schema &&
                      current_player == player_schema,
                  "four original player slots share field schema");
        }
        const auto host_start = offset;
        offset = MarkerEnd(bytes, offset, "MGC_HOST_SER_FLAG");
        pal4::PlayerManagerSlotState image{};
        auto* host = reinterpret_cast<pal4::IndexedCollectionResetState*>(
            reinterpret_cast<std::byte*>(&image) + 0x938u);
        for (std::uint32_t index = 0u; index < 11u; ++index) {
            const auto count = Word(bytes, offset);
            const auto stride = index < 10u && (index & 1u) != 0u ? 8u : 4u;
            offset += 4u;
            const auto size = static_cast<std::size_t>(count) * stride;
            std::uint32_t* begin = nullptr;
            std::uint32_t* end = nullptr;
            if (index == 10u) {
                begin = &host->key_values[5u].begin;
                end = &host->key_values[5u].end;
            } else if ((index & 1u) != 0u) {
                begin = &host->key_values[index / 2u].begin;
                end = &host->key_values[index / 2u].end;
            } else {
                begin = &host->values[index / 2u].begin;
                end = &host->values[index / 2u].end;
            }
            if (count != 0u) {
                *begin = static_cast<std::uint32_t>(
                    reinterpret_cast<std::uintptr_t>(bytes.data() + offset));
                *end = *begin + static_cast<std::uint32_t>(size);
            }
            offset += size;
            Check(offset <= bytes.size(), "host-server collection fits save");
        }
        pal4::NativeGameSaveArchiveBuffer rebuilt;
        pal4::NativeHostServerSaveRuntime host_writer(rebuilt);
        Check(host_writer.Serialize(static_cast<std::uint32_t>(
                  reinterpret_cast<std::uintptr_t>(&image))) &&
                  rebuilt.Bytes().size() == offset - host_start &&
                  std::memcmp(rebuilt.Bytes().data(),
                              bytes.data() + host_start,
                              rebuilt.Bytes().size()) == 0,
              "native HostServer writer matches original slot bytes");
    }
    const auto manager_start = offset;
    pal4::BinaryXmlConfig manager;
    offset = ConfigEnd(bytes, offset, "P_MGR", manager);
    Check(manager.Root().attributes.size() == 12u &&
              manager.Root().FindAttribute("Leading") != nullptr &&
              manager.Root().FindAttribute("CameraAutoSeek") != nullptr,
          "manager config follows all four slots");
    pal4::PlayerManagerObjectState manager_image{};
    const auto* leading = manager.Root().FindAttribute("Leading");
    Check(leading->type == 1u && leading->ToInt(4u) < 4u,
          "original leading player index is valid");
    manager_image.Word(0x2c50u / 4u) = leading->ToInt(4u);
    const auto slot = manager_image.Word(0x2c50u / 4u);
    for (const auto& attribute : manager.Root().attributes) {
        std::uint32_t target = 0u;
        if (attribute.name == "Leading") continue;
        if (attribute.name == "CameraAutoSeek")
            target = slot * 0xb14u + 0xb0cu;
        else if (attribute.name.starts_with("Valid") &&
                 attribute.name.size() == 6u &&
                 attribute.name[5] >= '0' && attribute.name[5] <= '3')
            target = 0x2c54u + (attribute.name[5] - '0') * 4u;
        else {
            const auto position = attribute.name.starts_with("LeaderPos_");
            const auto rotation = attribute.name.starts_with("LeaderRot_");
            Check((position || rotation) && attribute.name.size() == 11u &&
                      attribute.name[10] >= 'X' &&
                      attribute.name[10] <= 'Z',
                  "original manager vector field name");
            const auto axis = attribute.name[10] == 'X' ? 0u :
                              attribute.name[10] == 'Y' ? 1u : 2u;
            target = slot * 0xb14u + (position ? 0xacu : 0xa0u) + axis * 4u;
        }
        const auto raw = attribute.type == 2u
                             ? std::bit_cast<std::uint32_t>(
                                   std::get<float>(attribute.value))
                             : std::bit_cast<std::uint32_t>(
                                   std::get<std::int32_t>(attribute.value));
        manager_image.Word(target / 4u) = raw;
    }
    pal4::NativeGameSaveArchiveBuffer manager_rebuilt;
    pal4::NativePlayerManagerSaveRuntime manager_writer(manager_rebuilt);
    Check(manager_writer.SerializeManagerDocument(static_cast<std::uint32_t>(
              reinterpret_cast<std::uintptr_t>(&manager_image))),
          "native P_MGR writer completes");
    if (manager_rebuilt.Bytes().size() != offset - manager_start ||
        std::memcmp(manager_rebuilt.Bytes().data(), bytes.data() + manager_start,
                    std::min(manager_rebuilt.Bytes().size(),
                             offset - manager_start)) != 0) {
        std::cerr << "original P_MGR order:";
        for (const auto& attribute : manager.Root().attributes)
            std::cerr << ' ' << attribute.name;
        std::cerr << '\n';
    }
    Check(manager_rebuilt.Bytes().size() == offset - manager_start &&
              std::memcmp(manager_rebuilt.Bytes().data(),
                          bytes.data() + manager_start,
                          manager_rebuilt.Bytes().size()) == 0,
          "native P_MGR writer matches original document bytes");
    const auto arena_start = FindMarker(bytes, "ARENA_SERIALIZE_FLAG");
    Check(arena_start >= offset + 4u,
          "original arena cache precedes arena config");
    std::size_t cache_consumed{};
    const auto cache_section = pal4::NativeArenaCacheSaveSection::ParsePrefix(
        bytes.subspan(offset, arena_start - offset), cache_consumed);
    Check(cache_section && cache_consumed == arena_start - offset &&
              cache_section->entries.size() == 3u &&
              cache_section->entries[0].names[0] == "Q01" &&
              cache_section->entries[0].names[1] == "N01" &&
              cache_section->entries[1].groups[0].size() == 36u &&
              cache_section->entries[1].minimap_header == 0u &&
              cache_section->entries[2].names[1] == "N03",
          "original ArenaCache section parses three scene snapshots");
    const auto& first_entry = cache_section->entries[0];
    const auto& second_entry = cache_section->entries[1];
    const auto& third_entry = cache_section->entries[2];
    const auto& first_child = first_entry.groups[0][0];
    Check(first_entry.groups[0].size() == 1u &&
              first_entry.groups[1].empty() && first_child.id == 8u &&
              first_child.document.Root().name == "soGOBEffect" &&
              first_child.document.Root().FindAttribute("valName") &&
              std::get<std::string>(first_child.document.Root()
                  .FindAttribute("valName")->value) == "effect001" &&
              first_child.document.Root().FindAttribute("valVisible") &&
              std::get<std::int32_t>(first_child.document.Root()
                  .FindAttribute("valVisible")->value) == 1 &&
              first_entry.minimap_header == 0xffff0000u &&
              second_entry.groups[0].size() == 36u &&
              second_entry.groups[1].empty() &&
              third_entry.groups[0].size() == 1u &&
              third_entry.groups[0][0].id == 7u &&
              third_entry.groups[1].empty(),
          "original ArenaCache child distribution and Q01/N01 effect fields");
    for (const auto& child : second_entry.groups[0]) {
        if (child.id != 7u) continue;
        const auto& root = child.document.Root();
        Check(root.name == "soGOBGetItem" && root.attributes.size() == 3u &&
                  std::get<std::int32_t>(root.FindAttribute("researchNum")->value) == 1 &&
                  std::get<std::string>(root.FindAttribute("valName")->value) == "item001" &&
                  std::get<std::int32_t>(root.FindAttribute("valVisible")->value) == 1,
              "original Q01/Q01 first GetItem snapshot fields");
        break;
    }
    for (const auto& child : second_entry.groups[0]) {
        if (child.id != 3u) continue;
        const auto& root = child.document.Root();
        Check(root.name == "soGOBSound" && root.attributes.size() == 1u &&
                  std::get<std::string>(root.FindAttribute("valName")->value) ==
                      "sound001",
              "original Q01/Q01 first Sound snapshot fields");
        break;
    }
    const auto first_machine = std::find_if(
        second_entry.groups[0].begin(), second_entry.groups[0].end(),
        [](const auto& child) { return child.id == 5u; });
    Check(first_machine != second_entry.groups[0].end(),
          "original Q01/Q01 contains a Machine snapshot");
    const auto& machine_root = first_machine->document.Root();
    Check(machine_root.name == "soGOBMachine" &&
              machine_root.attributes.size() == 13u &&
              std::get<std::string>(machine_root.FindAttribute("valName")->value) ==
                  "item101" &&
              std::get<std::string>(machine_root.FindAttribute("valActionName")->value) ==
                  "OM07" &&
              std::get<std::int32_t>(machine_root.FindAttribute("valVisible")->value) == 1 &&
              std::get<std::int32_t>(machine_root.FindAttribute("valHasTouched")->value) == 1 &&
              std::get<std::int32_t>(machine_root.FindAttribute("valHasDoAction")->value) == 1 &&
              machine_root.FindAttribute("valPosX")->type == 2u &&
              machine_root.FindAttribute("valRotY")->type == 2u,
          "original Q01/Q01 first Machine snapshot fields");
    pal4::NativeGameSaveArchiveBuffer cache_rebuilt;
    pal4::NativeArenaCacheSaveSection cache_writer(cache_rebuilt);
    Check(cache_writer.Serialize(*cache_section) &&
              cache_rebuilt.Bytes().size() == cache_consumed &&
              std::memcmp(cache_rebuilt.Bytes().data(), bytes.data() + offset,
                          cache_consumed) == 0,
          "ArenaCache structured writer matches original section bytes");
    pal4::NativeArenaCacheOwner loaded_cache;
    std::size_t loaded_cache_bytes = 0u;
    Check(loaded_cache.LoadSection(
              bytes.subspan(offset, arena_start - offset),
              loaded_cache_bytes) &&
              loaded_cache_bytes == cache_consumed,
          "ArenaCache::Load consumes original slot 0 cache section");
    pal4::NativeGameSaveArchiveBuffer loaded_cache_archive;
    Check(loaded_cache.SaveCurrent(cache_section->entries.back(),
                                   loaded_cache_archive) &&
              loaded_cache_archive.Bytes().size() == cache_consumed &&
              std::memcmp(loaded_cache_archive.Bytes().data(),
                          bytes.data() + offset, cache_consumed) == 0,
          "loaded original ArenaCache saves identical section bytes");
    loaded_cache.Destroy();
    Check(loaded_cache.OutstandingAllocations() == 0u,
          "original ArenaCache load releases three scene entries");
    pal4::BinaryXmlConfig arena_config;
    const auto arena_document = MarkerEnd(bytes, arena_start,
                                          "ARENA_SERIALIZE_FLAG");
    const auto arena_end = ConfigEnd(bytes, arena_document, "ARENA",
                                     arena_config);
    const auto& arena_attributes = arena_config.Root().attributes;
    Check(arena_attributes.size() == 3u &&
              arena_attributes[0].name == "area" &&
              arena_attributes[1].name == "diff" &&
              arena_attributes[2].name == "world" &&
              arena_attributes[0].type == 3u &&
              arena_attributes[1].type == 3u &&
              arena_attributes[2].type == 3u,
          "original ARENA document field schema");
    pal4::NativeGameSaveArchiveBuffer arena_rebuilt;
    pal4::NativeArenaConfigSaveRuntime arena_writer(arena_rebuilt);
    Check(arena_writer.Serialize(
              std::get<std::string>(arena_attributes[2].value),
              std::get<std::string>(arena_attributes[0].value),
              std::get<std::string>(arena_attributes[1].value)) &&
              arena_rebuilt.Bytes().size() == arena_end - arena_start &&
              std::memcmp(arena_rebuilt.Bytes().data(),
                          bytes.data() + arena_start,
                          arena_rebuilt.Bytes().size()) == 0,
          "native ARENA writer matches original document bytes");
    std::cout << "PASS: original player save layout\n";
}

void CheckPlayerWriterEdges() {
    pal4::NativeGameSaveArchiveBuffer archive;
    pal4::NativePlayerSaveRuntime writer(archive);
    Check(!writer.Serialize({}, {}) && archive.Bytes().empty(),
          "Player writer rejects missing value resolver");
    Check(!writer.Serialize(
              [](const pal4::NativePlayerBasicField&)
                  -> std::optional<pal4::BinaryXmlAttribute::Value> {
                  return std::string("wrong type");
              }, {}) && archive.Bytes().empty(),
          "Player writer rejects mismatched field types before writing");
    const std::int32_t skills[]{42, -7};
    Check(writer.Serialize(
              [](const pal4::NativePlayerBasicField& field)
                  -> std::optional<pal4::BinaryXmlAttribute::Value> {
                  if (field.type == 1u) return std::int32_t{0};
                  if (field.type == 2u) return 0.0f;
                  if (field.type == 3u) return std::string{};
                  return std::nullopt;
              }, skills),
          "Player writer accepts complete typed values");
    pal4::BinaryXmlConfig parsed;
    Check(parsed.Parse(std::span(
              reinterpret_cast<const std::uint8_t*>(archive.Bytes().data()),
              archive.Bytes().size())) &&
              parsed.Root().children.size() == 1u &&
              parsed.Root().children[0].children.size() == 2u &&
              parsed.Root().children[0].children[0].name == "42" &&
              parsed.Root().children[0].children[1].name == "-7",
          "Player writer preserves ordered signed forbidden skills");

    std::array<std::uint32_t, 60u> arena_image{};
    const char world[] = "Q01";
    const char area[] = "N01";
    const char diff[] = "Q01/N01";
    arena_image[8u / 4u] = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(world));
    arena_image[12u / 4u] = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(area));
    arena_image[224u / 4u] = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(diff));
    pal4::NativeGameSaveArchiveBuffer live_arena_archive;
    pal4::NativeArenaConfigSaveRuntime live_arena_writer(live_arena_archive);
    Check(live_arena_writer.SerializeArena(static_cast<std::uint32_t>(
              reinterpret_cast<std::uintptr_t>(arena_image.data()))),
          "live ARENA writer reads three native text fields");
    pal4::NativeGameSaveArchiveBuffer expected_arena_archive;
    pal4::NativeArenaConfigSaveRuntime expected_arena_writer(
        expected_arena_archive);
    Check(expected_arena_writer.Serialize(world, area, diff) &&
              live_arena_archive.Bytes() == expected_arena_archive.Bytes(),
          "live ARENA fields match direct writer values");
}

void CheckArenaCacheSectionEdges() {
    pal4::NativeArenaCacheSection section;
    pal4::NativeArenaCacheEntry entry;
    entry.names = {"Q01", "N01"};
    entry.minimap_header = 0xabcd0002u;
    entry.minimap_elements = {{1u, 2u}, {3u}};
    section.entries.push_back(std::move(entry));

    pal4::NativeGameSaveArchiveBuffer archive;
    pal4::NativeArenaCacheSaveSection writer(archive);
    Check(writer.Serialize(section), "ArenaCache writes nonempty minimap");
    const auto bytes = std::span(
        reinterpret_cast<const std::uint8_t*>(archive.Bytes().data()),
        archive.Bytes().size());
    std::size_t consumed{};
    const auto parsed = pal4::NativeArenaCacheSaveSection::ParsePrefix(
        bytes, consumed);
    Check(parsed && consumed == bytes.size() &&
              parsed->entries.size() == 1u &&
              parsed->entries[0].minimap_header == 0xabcd0002u &&
              parsed->entries[0].minimap_elements ==
                  section.entries[0].minimap_elements,
          "ArenaCache preserves packed minimap header and elements");
    Check(!pal4::NativeArenaCacheSaveSection::ParsePrefix(
              bytes.first(bytes.size() - 1u), consumed) && consumed == 0u,
          "ArenaCache rejects truncated minimap payload");

    auto corrupt = std::vector<std::uint8_t>(bytes.begin(), bytes.end());
    const std::uint32_t impossible_count = 0xffffffffu;
    std::memcpy(corrupt.data(), &impossible_count, sizeof(impossible_count));
    Check(!pal4::NativeArenaCacheSaveSection::ParsePrefix(
              corrupt, consumed) && consumed == 0u,
          "ArenaCache rejects impossible entry count");
    section.entries[0].minimap_header = 1u;
    pal4::NativeGameSaveArchiveBuffer invalid_archive;
    pal4::NativeArenaCacheSaveSection invalid_writer(invalid_archive);
    Check(!invalid_writer.Serialize(section),
          "ArenaCache rejects mismatched minimap element count");
}

void CheckBasicReaderEdges() {
    pal4::PlayerManagerSlotState player{};
    const auto address = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&player));
    const auto fields = pal4::NativePlayerBasicFieldSchema::Build();
    pal4::BinaryXmlNode basic{"BASIC"};
    // Nonzero state proves missing attributes don't reset live values.
    std::memset(&player, 0xa5, sizeof(player));
    const auto original = player;
    pal4::NativePlayerBasicLoadRuntime::Apply(address, basic);
    Check(std::memcmp(&player, &original, sizeof(player)) == 0,
          "absent BASIC attributes preserve the entire player image");
    unsigned enums = 0u;
    for (const auto& field : fields) {
        basic.attributes.clear();
        std::uint32_t expected{};
        if (field.type == 2u) {
            basic.attributes.push_back({field.name, 3u, std::string(" -12.5tail")});
            expected = std::bit_cast<std::uint32_t>(-12.5f);
        } else {
            const auto value = field.index_count ? static_cast<std::int32_t>(field.index_count - 1u) : -123;
            basic.attributes.push_back({field.name, 1u, value});
            expected = std::bit_cast<std::uint32_t>(value);
        }
        auto before = player;
        pal4::NativePlayerBasicLoadRuntime::Apply(address, basic);
        std::memcpy(reinterpret_cast<std::byte*>(&before) + field.slot_offset, &expected, 4u);
        Check(std::memcmp(&before, &player, sizeof(player)) == 0,
              "single BASIC field changes only its mapped word");
        if (field.index_count) {
            ++enums;
            for (const auto invalid : {-1, static_cast<int>(field.index_count)}) {
                basic.attributes[0].value = static_cast<std::int32_t>(invalid);
                const auto prior = player;
                pal4::NativePlayerBasicLoadRuntime::Apply(address, basic);
                Check(std::memcmp(&prior, &player, sizeof(player)) == 0,
                      "BASIC enum rejects negative and count boundary");
            }
        }
        basic.attributes[0].value = std::string("invalid");
        basic.attributes[0].type = 3u;
        pal4::NativePlayerBasicLoadRuntime::Apply(address, basic);
        std::uint32_t value{};
        std::memcpy(&value, reinterpret_cast<const std::byte*>(&player) + field.slot_offset, 4u);
        Check(value == 0u, "present unparseable BASIC scalar defaults to zero");
    }
    Check(enums == 2u, "RACE and COMBAT_POSITION retain enum bounds");
    bool rejected = false;
    try { pal4::NativePlayerBasicLoadRuntime::Apply(address, pal4::BinaryXmlNode{"Player"}); }
    catch (const std::invalid_argument&) { rejected = true; }
    Check(rejected, "BASIC helper rejects wrong document root");
}

void CheckPlayerFieldReaderEdges() {
    using Text = pal4::NativeXmlAttributeText;
    Check(Text::Convert({"x", 0u, std::int8_t{'A'}}) == "A" &&
              Text::Convert({"x", 0u, std::int8_t{0}}).empty() &&
              Text::Convert({"x", 1u, std::int32_t{-123}}) == "-123" &&
              Text::Convert({"x", 2u, 1.25f}) == "1.250000" &&
              Text::Convert({"x", 3u, std::string("abc\0tail", 8u)}) == "abc" &&
              Text::Convert({"x", 4u, std::vector<std::uint8_t>{}}).empty(),
          "shared XML conversion preserves char, number and CString semantics");
    bool rejected = false;
    try { (void)Text::Convert({"x", 4u, std::vector<std::uint8_t>{1u}}); }
    catch (const std::length_error&) { rejected = true; }
    Check(rejected, "unsafe original raw-to-text conversion fails explicitly");
    rejected = false;
    try { (void)Text::Convert({"x", 1u, std::string("wrong")}); }
    catch (const std::invalid_argument&) { rejected = true; }
    Check(rejected, "XML conversion rejects mismatched type storage");

    pal4::PlayerManagerSlotState player{};
    const auto address = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&player));
    pal4::NativePlayerSlotCopyRuntime owner;
    pal4::BinaryXmlNode node{"Player"};
    auto before = player;
    pal4::NativePlayerLoadFields::Apply(address, node, owner);
    Check(std::memcmp(&before, &player, sizeof(player)) == 0,
          "missing Player fields preserve the full slot image");
    unsigned texts = 0u, enums = 0u, equipment = 0u;
    for (const auto& field : pal4::NativePlayerFieldSchema::Build()) {
        const auto location = address + field.slot_offset;
        if (field.slot_offset >= 0x9f8u && field.type == 3u) {
            ++texts;
            node.attributes = {{field.name, 3u, std::string("first role text")}};
            pal4::NativePlayerLoadFields::Apply(address, node, owner);
            const auto data = owner.ReadDword(location + 4u);
            owner.AssignText(location, std::string_view(reinterpret_cast<const char*>(data) + 6u, 4u));
            Check(std::string(reinterpret_cast<const char*>(owner.ReadDword(location + 4u))) == "role" &&
                      owner.ReadDword(location + 8u) == 4u,
                  "Role text replacement handles aliasing its owned buffer");
            node.attributes[0].value = std::string{};
            pal4::NativePlayerLoadFields::Apply(address, node, owner);
            Check(owner.ReadDword(location + 4u) == 0u && owner.ReadDword(location + 8u) == 0u &&
                      owner.ReadDword(location + 12u) == 0u,
                  "empty Role text releases and clears its record");
            owner.ReleaseText(location);
        } else if (field.slot_offset >= 0x9f8u && field.index_count) {
            ++enums;
            node.attributes = {{field.name, 1u, static_cast<std::int32_t>(field.index_count - 1u)}};
            pal4::NativePlayerLoadFields::Apply(address, node, owner);
            Check(owner.ReadDword(location) == field.index_count - 1u, "Role enum accepts last valid index");
            for (const auto invalid : {-1, static_cast<int>(field.index_count)}) {
                node.attributes[0].value = static_cast<std::int32_t>(invalid);
                before = player;
                pal4::NativePlayerLoadFields::Apply(address, node, owner);
                Check(std::memcmp(&before, &player, sizeof(player)) == 0, "Role enum rejects both boundaries");
            }
        } else if (field.name.starts_with("ERES_")) {
            ++equipment;
            for (const auto text : {"-1...", "  +123 item name", "invalid"}) {
                node.attributes = {{field.name, 3u, std::string(text)}};
                pal4::NativePlayerLoadFields::Apply(address, node, owner);
                Check(owner.ReadDword(location) == static_cast<std::uint32_t>(std::atoi(text)),
                      "equipment parses original leading signed instance ID");
            }
            node.attributes = {{field.name, 1u, std::int32_t{42}}};
            pal4::NativePlayerLoadFields::Apply(address, node, owner);
            Check(owner.ReadDword(location) == 42u, "numeric XML equipment input uses shared text conversion");
        }
    }
    Check(texts == 9u && enums == 3u && equipment == 6u, "all Role text/enums and equipment slots exercised");
    std::array<std::uint32_t, 4u> foreign{};
    foreign[1] = address;
    const auto record = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(foreign.data()));
    rejected = false;
    try { owner.AssignText(record, "replacement"); }
    catch (const std::logic_error&) { rejected = true; }
    Check(rejected && foreign[1] == address, "text assignment rejects foreign ownership without mutation");
}

void CheckScriptReaderEdges() {
    pal4::PlayerManagerSlotState player{};
    const auto address = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&player));
    const auto script = address + 0x654u;
    const auto read = [script](unsigned word) { return *reinterpret_cast<const std::uint32_t*>(script + word * 4u); };
    const auto set = [script](unsigned slot, unsigned index, unsigned value) {
        return pal4::NativePlayerScriptPropertyRuntime::Set(script, slot, index, value);
    };
    pal4::BinaryXmlNode node{"Player"};
    auto before = player;
    pal4::NativePlayerScriptPropertyRuntime::Apply(address, node);
    Check(std::memcmp(&before, &player, sizeof(player)) == 0, "missing Player script fields retain values");
    node.attributes.push_back({"CombatPosition", 1u, std::int32_t{5}});
    pal4::NativePlayerScriptPropertyRuntime::Apply(address, node);
    Check(read(83u) == 5u && read(167u) == 5u, "script combat position updates both original copies");
    (void)set(0x44u, 4u, 12345u);
    Check(read(56u) == 12345u && read(140u) == 12345u, "progression setter synchronizes its mirror");
    (void)set(0x40u, 0u, 100u);
    (void)set(0x40u, 2u, 80u);
    Check(set(0x44u, 7u, 200u) == 100u && read(143u) == 100u && read(162u) == 0u,
          "HP clamps and updates normal health state");
    Check(set(0x44u, 7u, 10u) == 10u && read(162u) == 2u,
          "low HP notifies health state");
    Check(set(0x44u, 7u, 0xffffffffu) == 0u && read(162u) == 3u,
          "negative HP floors and marks death");
    Check(set(0x44u, 9u, 81u) == 80u && read(145u) == 80u, "MP upper clamp");
    Check(set(0x44u, 8u, 101u) == 100u && read(144u) == 100u, "special progression bound");
    before = player;
    (void)set(0xa4u, 0u, 2u);
    Check(std::memcmp(&before, &player, sizeof(player)) == 0, "script enum rejects upper bound");
    node.attributes = {{"CombatPosition", 3u, std::string("invalid")}};
    pal4::NativePlayerScriptPropertyRuntime::Apply(address, node);
    Check(read(83u) == 0u && read(167u) == 0u, "present unparseable script integer defaults to zero");
    bool rejected = false;
    try { (void)set(0x04u, 0u, 0u); }
    catch (const std::logic_error&) { rejected = true; }
    Check(rejected, "unsupported scalar dispatch fails explicitly");
}

}  // namespace

int main(const int argc, char** argv) {
    if (argc != 2) return EXIT_FAILURE;
    try {
        CheckBasicReaderEdges();
        CheckPlayerFieldReaderEdges();
        CheckScriptReaderEdges();
        CheckPlayerWriterEdges();
        CheckArenaCacheSectionEdges();
        CheckOriginal(argv[1]);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
