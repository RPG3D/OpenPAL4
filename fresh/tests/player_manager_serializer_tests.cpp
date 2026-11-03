#include "pal4/player_manager_serializer.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::uint32_t kManager = 0x5000u;
constexpr std::uint32_t kData = 0x7000u;
constexpr std::uint32_t kStride = 0xb14u;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PlayerManagerSerializeRuntimeStub {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::string, std::uint32_t> found_nodes;
    std::map<std::uint32_t, std::string> lookup_names;
    std::map<std::uint32_t, std::string> attribute_names;
    std::vector<std::string> events;
    std::vector<std::uint32_t> reads;
    std::vector<std::uint32_t> processed;
    std::vector<std::uint32_t> loaded;
    std::vector<std::string> added_names;
    std::vector<std::string> lookup_order;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> attribute_writes;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> vectors;
    std::vector<std::uint8_t> incoming_modes;
    std::map<std::uint32_t, std::uint32_t> parsed_attributes;
    std::map<std::uint32_t, std::array<std::uint32_t, 3>> read_vectors;
    std::vector<std::array<std::uint32_t, 3>> applied_vectors;
    std::vector<std::uint32_t> applied_slots;
    std::vector<std::uint32_t> applied_virtual_slots;
    std::uint32_t loaded_camera{};
    std::int32_t flag_compare{};
    bool version_valid{true};
    std::uint32_t leading_attribute{};
    std::uint32_t next_attribute{0xb000u};
    std::uint32_t next_lookup{0xc000u};
    std::uint8_t header_count{};
    bool allocate_flag{true};
    bool throw_add{};
    bool throw_find{};
    bool throw_write{};
    std::string throw_add_name;

    Runtime() {
        words[kManager + 0x2c50u] = 2u;
        for (std::uint32_t i = 0u; i < 4u; ++i)
            words[kManager + 0x2c54u + i * 4u] = 10u + i;
        words[kManager + 2u * kStride + 0xb0cu] = 77u;
        bytes[0x9000u] = 0u;
    }

    void InitializeFlag(pal4::PlayerSerializationFlag& flag,
                        std::uint8_t initial) override {
        Check(initial == 0x34u, "incoming stack byte reaches flag initializer");
        flag.data = 0u;
        events.emplace_back("flag:init");
    }
    bool AllocateFlag(pal4::PlayerSerializationFlag& flag,
                      std::uint32_t length, std::uint8_t mode) override {
        Check(length == 20u && mode == 1u,
              "flag allocation uses literal length and mode one");
        events.emplace_back("flag:alloc");
        if (allocate_flag) flag.data = 0x9001u;
        bytes[0x9000u] = header_count;
        return allocate_flag;
    }
    void CopyFlagText(pal4::PlayerSerializationFlag& flag,
                      std::uint32_t source, std::uint32_t length) override {
        Check(flag.data == 0x9001u && source ==
                  pal4::PlayerManagerSerializer::kFlagSource && length == 20u,
              "flag text copies from the IDA literal address");
        events.emplace_back("flag:copy");
    }
    void SetFlagSize(pal4::PlayerSerializationFlag& flag,
                     std::uint32_t length) override {
        Check(flag.data == 0x9001u && length == 20u,
              "flag size follows text copy");
        events.emplace_back("flag:size");
    }
    void DataReady(std::uint32_t data,
                   const pal4::PlayerSerializationFlag& flag) override {
        Check(data == kData && flag.data ==
                  (allocate_flag ? 0x9001u : 0u),
              "data readiness receives the prepared flag");
        events.emplace_back("data:ready");
    }
    void FillFlag(std::uint32_t data,
                  pal4::PlayerSerializationFlag& flag) override {
        Check(data == kData, "loader fills flag from caller data");
        flag.data = 0x9001u;
        flag.length = 20u;
        bytes[0x9000u] = header_count;
        events.emplace_back("flag:fill");
    }
    std::int32_t CompareFlag(const pal4::PlayerSerializationFlag& flag,
                             std::uint32_t source,
                             std::uint32_t source_length) override {
        Check(flag.data == 0x9001u && flag.length == 20u &&
                  source == pal4::PlayerManagerSerializer::kFlagSource &&
                  source_length == 20u,
              "loaded flag is compared with full literal length");
        events.emplace_back("flag:compare");
        return flag_compare;
    }
    void ProcessPlayer(std::uint32_t slot, std::uint32_t data) override {
        Check(data == kData && slot == kManager + processed.size() * kStride,
              "player config processes four slots in order");
        processed.push_back(slot);
        events.emplace_back("player:" + std::to_string(processed.size() - 1u));
    }
    void ProcessLoadedData(std::uint32_t manager) override {
        Check(manager == kManager, "loaded-data preparation receives manager");
        events.emplace_back("loaded:prepare");
    }
    void LoadPlayer(std::uint32_t slot, std::uint32_t data) override {
        Check(data == kData && slot == kManager + loaded.size() * kStride,
              "loader processes four player slots in order");
        loaded.push_back(slot);
        events.push_back("load:" + std::to_string(loaded.size() - 1u));
    }
    std::uint32_t CreateDocument(std::string_view root,
                                 std::uint32_t option) override {
        Check(root == "P_MGR" && option == 0u,
              "document uses P_MGR root and zero option");
        events.emplace_back("document:create");
        return 0xa000u;
    }
    void DestroyDocument(std::uint32_t document) override {
        Check(document == 0xa000u, "document destructor receives its handle");
        events.emplace_back("document:destroy");
    }
    bool CheckDocumentVersion(std::uint32_t document,
                              std::uint32_t data) override {
        Check(document == 0xa000u && data == kData,
              "version check uses document and caller data");
        events.emplace_back("document:version");
        return version_valid;
    }
    std::uint32_t FindLeadingAttribute(std::uint32_t document,
                                       std::string_view name) override {
        Check(document == 0xa000u && name == "Leading",
              "leading uses direct config lookup");
        events.emplace_back("leading:find");
        return leading_attribute;
    }
    std::uint32_t CreateAttribute(std::string_view name,
                                  std::uint32_t value) override {
        const auto handle = next_attribute++;
        attribute_names[handle] = std::string(name);
        attribute_writes.emplace_back(handle, value);
        events.push_back("attribute:create:" + std::string(name));
        return handle;
    }
    void AddAttribute(std::uint32_t document,
                      std::uint32_t attribute) override {
        Check(document == 0xa000u && attribute_names.contains(attribute),
              "new attribute is attached to P_MGR document");
        const auto& name = attribute_names.at(attribute);
        events.push_back("attribute:add:" + name);
        if (throw_add && name == throw_add_name)
            throw std::runtime_error("add failure");
        added_names.push_back(name);
    }
    void DestroyAttribute(std::uint32_t attribute) override {
        events.push_back("attribute:destroy:" + attribute_names.at(attribute));
    }
    void SetAttributeInt(std::uint32_t attribute,
                         std::uint32_t value) override {
        attribute_writes.emplace_back(attribute, value);
        events.push_back("attribute:set:" + std::to_string(attribute));
    }
    std::uint32_t AttributeToInt(std::uint32_t attribute,
                                 std::uint32_t fallback) override {
        events.push_back("attribute:parse:" + std::to_string(attribute));
        const auto found = parsed_attributes.find(attribute);
        return found == parsed_attributes.end() ? fallback : found->second;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        reads.push_back(address);
        return words[address];
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        events.push_back("word:write:" + std::to_string(address));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x9000u, "flag cleanup reads reference header");
        events.emplace_back("header:read");
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x9000u, "flag cleanup writes reference header");
        bytes[address] = value;
        events.emplace_back("header:write");
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x9000u, "flag cleanup frees header address");
        events.emplace_back("header:free");
    }
    void SetVector(std::uint32_t document, std::uint32_t name,
                   std::uint32_t source) override {
        Check(document == 0xa000u, "vector uses P_MGR document");
        vectors.emplace_back(name, source);
        events.emplace_back("vector");
    }
    std::array<std::uint32_t, 3> GetVector(
        std::uint32_t document, std::uint32_t name) override {
        Check(document == 0xa000u, "vector reader uses P_MGR document");
        events.push_back("vector:read:" + std::to_string(name));
        return read_vectors[name];
    }
    void ApplyTransform(
        std::uint32_t slot, std::uint32_t virtual_slot,
        const std::array<std::uint32_t, 3>& value,
        std::uint32_t option) override {
        Check(option == 0u, "loaded transform passes zero option");
        applied_slots.push_back(slot);
        applied_virtual_slots.push_back(virtual_slot);
        applied_vectors.push_back(value);
        events.push_back("transform:" + std::to_string(virtual_slot));
    }
    void SetPlayerState(std::uint32_t manager,
                        std::uint32_t value) override {
        Check(manager == kManager, "loaded camera state receives manager");
        loaded_camera = value;
        events.emplace_back("camera:set");
    }
    std::uint32_t CreateLookupKey(std::string_view name,
                                  std::uint8_t& mode_byte) override {
        const auto key = next_lookup++;
        lookup_names[key] = std::string(name);
        lookup_order.emplace_back(name);
        incoming_modes.push_back(mode_byte);
        ++mode_byte;
        events.push_back("key:create:" + std::string(name));
        return key;
    }
    pal4::PlayerAttributeCursor FindTreeAttribute(
        std::uint32_t key) override {
        const auto& name = lookup_names.at(key);
        events.push_back("key:find:" + name);
        if (throw_find) throw std::runtime_error("find failure");
        const auto found = found_nodes.find(name);
        return {found == found_nodes.end() ? 0xdead0000u : found->second,
                0xdead0000u};
    }
    void ResetLookupKey(std::uint32_t key) override {
        events.push_back("key:reset:" + lookup_names.at(key));
    }
    std::uint8_t WriteDocument(std::uint32_t document,
                               std::uint32_t data) override {
        Check(document == 0xa000u && data == kData,
              "write uses the constructed document and caller data");
        events.emplace_back("document:write");
        if (throw_write) throw std::runtime_error("write failure");
        return 0x5au;
    }
};

std::size_t At(const std::vector<std::string>& events,
               const std::string& name) {
    const auto found = std::find(events.begin(), events.end(), name);
    Check(found != events.end(), "expected serialization event exists");
    return static_cast<std::size_t>(found - events.begin());
}

void MissingAttributesAndFallbacks() {
    Runtime runtime;
    Check(pal4::PlayerManagerSerializer::Serialize(
              kManager, kData, 0x34u, runtime) == 0x5au,
          "serializer forwards the config writer's byte result");
    Check(runtime.processed.size() == 4u && runtime.added_names ==
              std::vector<std::string>{"Leading", "Valid0", "Valid1",
                                       "Valid2", "Valid3", "CameraAutoSeek"},
          "missing attributes are added in IDA order");
    Check(runtime.vectors == std::vector<std::pair<std::uint32_t, std::uint32_t>>{
              {pal4::PlayerManagerSerializer::kEmptyName,
               kManager + 2u * kStride + 0xacu},
              {pal4::PlayerManagerSerializer::kEmptyName,
               kManager + 2u * kStride + 0xa0u}},
          "null global vector names use the shared empty string");
    Check(runtime.lookup_order == std::vector<std::string>{
              "Valid0", "Valid1", "Valid2", "Valid3", "CameraAutoSeek"},
          "four Valid names precede camera setting");
    Check(runtime.incoming_modes == std::vector<std::uint8_t>{
              0x34u, 0x35u, 0x36u, 0x37u, 0x38u},
          "mode byte is passed by reference between lookup constructors");
    Check(runtime.attribute_writes.size() == 6u &&
          runtime.attribute_writes[0].second == 2u &&
          runtime.attribute_writes[1].second == 10u &&
          runtime.attribute_writes[2].second == 11u &&
          runtime.attribute_writes[3].second == 12u &&
          runtime.attribute_writes[4].second == 13u &&
          runtime.attribute_writes[5].second == 77u,
          "attribute values come from current index, four flags and camera state");
    const auto& e = runtime.events;
    Check(At(e, "flag:init") < At(e, "flag:alloc") &&
          At(e, "flag:alloc") < At(e, "flag:copy") &&
          At(e, "flag:copy") < At(e, "flag:size") &&
          At(e, "flag:size") < At(e, "data:ready") &&
          At(e, "data:ready") < At(e, "player:0") &&
          At(e, "player:3") < At(e, "document:create") &&
          At(e, "document:create") < At(e, "leading:find") &&
          At(e, "attribute:add:Leading") < At(e, "vector") &&
          At(e, "key:reset:CameraAutoSeek") < At(e, "document:write") &&
          At(e, "document:write") < At(e, "document:destroy") &&
          At(e, "document:destroy") < At(e, "header:free"),
          "major serialization stages and cleanup retain observed order");
}

void ExistingAttributesAndReferenceCounts() {
    Runtime runtime;
    runtime.leading_attribute = 0xd000u;
    runtime.found_nodes["Valid1"] = 0xd100u;
    runtime.found_nodes["Valid3"] = 0xffffffe4u;
    runtime.found_nodes["CameraAutoSeek"] = 0xd200u;
    runtime.words[pal4::PlayerManagerSerializer::kLeaderPositionName] = 0xe100u;
    runtime.words[pal4::PlayerManagerSerializer::kLeaderRotationName] = 0xe200u;
    runtime.header_count = 4u;
    static_cast<void>(pal4::PlayerManagerSerializer::Serialize(
        kManager, kData, 0x34u, runtime));
    Check(runtime.added_names == std::vector<std::string>{
              "Valid0", "Valid2", "Valid3"},
          "direct and tree matches update, while null-adjusted node is added");
    Check(std::find(runtime.attribute_writes.begin(),
                    runtime.attribute_writes.end(),
                    std::pair<std::uint32_t, std::uint32_t>{0xd000u, 2u}) !=
              runtime.attribute_writes.end() &&
          std::find(runtime.attribute_writes.begin(),
                    runtime.attribute_writes.end(),
                    std::pair<std::uint32_t, std::uint32_t>{0xd11cu, 11u}) !=
              runtime.attribute_writes.end() &&
          std::find(runtime.attribute_writes.begin(),
                    runtime.attribute_writes.end(),
                    std::pair<std::uint32_t, std::uint32_t>{0xd21cu, 77u}) !=
              runtime.attribute_writes.end(),
          "existing tree node's XML attribute sits at node +0x1c");
    Check(runtime.vectors == std::vector<std::pair<std::uint32_t, std::uint32_t>>{
              {0xe100u, kManager + 2u * kStride + 0xacu},
              {0xe200u, kManager + 2u * kStride + 0xa0u}},
          "non-null global vector names are used directly");
    Check(runtime.bytes[0x9000u] == 3u &&
          At(runtime.events, "header:write") >
              At(runtime.events, "document:destroy"),
          "shared flag reference header decrements after document teardown");
}

void AllocationAndExceptionPaths() {
    Runtime failed_alloc;
    failed_alloc.allocate_flag = false;
    static_cast<void>(pal4::PlayerManagerSerializer::Serialize(
        kManager, kData, 0x34u, failed_alloc));
    Check(std::find(failed_alloc.events.begin(), failed_alloc.events.end(),
                    "flag:copy") == failed_alloc.events.end() &&
          std::find(failed_alloc.events.begin(), failed_alloc.events.end(),
                    "flag:size") == failed_alloc.events.end() &&
          std::find(failed_alloc.events.begin(), failed_alloc.events.end(),
                    "header:free") == failed_alloc.events.end() &&
          At(failed_alloc.events, "data:ready") <
              At(failed_alloc.events, "player:0"),
          "flag allocation failure still continues into data and player save");

    Runtime sentinel;
    sentinel.header_count = 0xffu;
    static_cast<void>(pal4::PlayerManagerSerializer::Serialize(
        kManager, kData, 0x34u, sentinel));
    Check(At(sentinel.events, "header:free") >
              At(sentinel.events, "document:destroy"),
          "0xff reference header follows the free branch");

    Runtime add_failure;
    add_failure.throw_add = true;
    add_failure.throw_add_name = "Valid2";
    bool threw = false;
    try {
        static_cast<void>(pal4::PlayerManagerSerializer::Serialize(
            kManager, kData, 0x34u, add_failure));
    } catch (const std::runtime_error&) { threw = true; }
    Check(threw && At(add_failure.events, "attribute:add:Valid2") <
                       At(add_failure.events, "attribute:destroy:Valid2") &&
          At(add_failure.events, "attribute:destroy:Valid2") <
              At(add_failure.events, "document:destroy") &&
          At(add_failure.events, "document:destroy") <
              At(add_failure.events, "header:free") &&
          std::find(add_failure.events.begin(), add_failure.events.end(),
                    "document:write") == add_failure.events.end(),
          "add failure unwinds temporary attribute, document and flag");

    Runtime find_failure;
    find_failure.throw_find = true;
    threw = false;
    try {
        static_cast<void>(pal4::PlayerManagerSerializer::Serialize(
            kManager, kData, 0x34u, find_failure));
    } catch (const std::runtime_error&) { threw = true; }
    Check(threw && At(find_failure.events, "key:find:Valid0") <
                       At(find_failure.events, "key:reset:Valid0") &&
          At(find_failure.events, "key:reset:Valid0") <
              At(find_failure.events, "document:destroy"),
          "lookup failure resets key before enclosing document");

    Runtime write_failure;
    write_failure.throw_write = true;
    threw = false;
    try {
        static_cast<void>(pal4::PlayerManagerSerializer::Serialize(
            kManager, kData, 0x34u, write_failure));
    } catch (const std::runtime_error&) { threw = true; }
    Check(threw && At(write_failure.events, "document:write") <
                       At(write_failure.events, "document:destroy") &&
          At(write_failure.events, "document:destroy") <
              At(write_failure.events, "header:free"),
          "write failure still destroys document and flag");
}

void DeserializeEarlyExits() {
    Runtime mismatch;
    mismatch.flag_compare = -1;
    Check(!pal4::PlayerManagerSerializer::Deserialize(
              kManager, kData, 0x34u, mismatch) &&
          mismatch.loaded.empty() &&
          At(mismatch.events, "flag:fill") <
              At(mismatch.events, "flag:compare") &&
          At(mismatch.events, "flag:compare") <
              At(mismatch.events, "header:free") &&
          std::find(mismatch.events.begin(), mismatch.events.end(),
                    "loaded:prepare") == mismatch.events.end(),
          "flag mismatch returns before player processing and releases flag");

    Runtime wrong_version;
    wrong_version.version_valid = false;
    Check(!pal4::PlayerManagerSerializer::Deserialize(
              kManager, kData, 0x34u, wrong_version) &&
          wrong_version.loaded.size() == 4u &&
          At(wrong_version.events, "loaded:prepare") <
              At(wrong_version.events, "load:0") &&
          At(wrong_version.events, "load:3") <
              At(wrong_version.events, "document:create") &&
          At(wrong_version.events, "document:version") <
              At(wrong_version.events, "document:destroy") &&
          At(wrong_version.events, "document:destroy") <
              At(wrong_version.events, "header:free") &&
          wrong_version.words[kManager + 0x2c50u] == 2u,
          "version failure happens after four loads but before field writes");
}

void DeserializeDefaultsAndParsedValues() {
    Runtime defaults;
    defaults.read_vectors[pal4::PlayerManagerSerializer::kEmptyName] =
        {0x3f800000u, 0x40000000u, 0x40400000u};
    Check(pal4::PlayerManagerSerializer::Deserialize(
              kManager, kData, 0x34u, defaults) &&
          defaults.words[kManager + 0x2c50u] == 0u &&
          defaults.loaded_camera == 1u,
          "missing leading and camera attributes use defaults zero and one");
    for (std::uint32_t i = 0u; i < 4u; ++i)
        Check(defaults.words[kManager + 0x2c54u + i * 4u] == 0u,
              "missing Valid attributes clear all four manager words");
    Check(defaults.applied_slots == std::vector<std::uint32_t>{
              kManager, kManager} &&
          defaults.applied_virtual_slots ==
              std::vector<std::uint32_t>{0x4u, 0x8u} &&
          defaults.applied_vectors.size() == 2u &&
          defaults.applied_vectors[0] ==
              defaults.read_vectors[pal4::PlayerManagerSerializer::kEmptyName] &&
          defaults.applied_vectors[1] ==
              defaults.read_vectors[pal4::PlayerManagerSerializer::kEmptyName],
          "both vector values are copied before virtual transform calls");
    Check(At(defaults.events, "document:version") <
              At(defaults.events, "leading:find") &&
          At(defaults.events, "transform:4") <
              At(defaults.events, "transform:8") &&
          At(defaults.events, "transform:8") <
              At(defaults.events, "camera:set") &&
          At(defaults.events, "camera:set") <
              At(defaults.events, "document:destroy"),
          "loaded fields, transforms and camera state preserve stage order");

    Runtime parsed;
    parsed.leading_attribute = 0xd000u;
    parsed.parsed_attributes[0xd000u] = 3u;
    parsed.found_nodes["Valid0"] = 0xd100u;
    parsed.found_nodes["Valid2"] = 0xd200u;
    parsed.found_nodes["Valid3"] = 0xffffffe4u;
    parsed.found_nodes["CameraAutoSeek"] = 0xd300u;
    parsed.parsed_attributes[0xd11cu] = 9u;
    parsed.parsed_attributes[0xd21cu] = 11u;
    parsed.parsed_attributes[0xd31cu] = 0u;
    parsed.words[pal4::PlayerManagerSerializer::kLeaderPositionName] = 0xe100u;
    parsed.words[pal4::PlayerManagerSerializer::kLeaderRotationName] = 0xe200u;
    parsed.read_vectors[0xe100u] = {1u, 2u, 3u};
    parsed.read_vectors[0xe200u] = {4u, 5u, 6u};
    Check(pal4::PlayerManagerSerializer::Deserialize(
              kManager, kData, 0x34u, parsed) &&
          parsed.words[kManager + 0x2c50u] == 3u &&
          parsed.words[kManager + 0x2c54u] == 9u &&
          parsed.words[kManager + 0x2c58u] == 0u &&
          parsed.words[kManager + 0x2c5cu] == 11u &&
          parsed.words[kManager + 0x2c60u] == 0u &&
          parsed.loaded_camera == 0u,
          "found attributes parse via node +0x1c; absent and sentinel use zero");
    Check(parsed.applied_slots == std::vector<std::uint32_t>{
              kManager + 3u * kStride, kManager + 3u * kStride} &&
          parsed.applied_vectors[0] == std::array<std::uint32_t, 3>{1u, 2u, 3u} &&
          parsed.applied_vectors[1] == std::array<std::uint32_t, 3>{4u, 5u, 6u},
          "parsed current index selects the transform target and named vectors");
}

}  // namespace

int main() {
    MissingAttributesAndFallbacks();
    ExistingAttributesAndReferenceCounts();
    AllocationAndExceptionPaths();
    DeserializeEarlyExits();
    DeserializeDefaultsAndParsedValues();
}
