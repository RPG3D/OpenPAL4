#include "pal4/game_data_persistence.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Attribute {
    std::string key;
    std::string value;
    bool set;
};

struct Runtime final : pal4::IGameDataPersistenceRuntime {
    std::vector<std::string> calls;
    std::vector<Attribute> attributes;
    std::vector<std::pair<std::string, std::int32_t>> defaults;
    std::map<std::uint32_t, std::uint32_t> memory;
    std::map<std::uint32_t, std::string> strings{{0x8000u, "ElapsedTime"},
                                                  {0x8100u, "MVar"},
                                                  {0x8200u, "MovieFlag"}};
    std::map<std::string, std::int32_t> input_ints;
    std::set<std::string> existing;
    std::optional<std::string> marker = std::string("GA_SERIALIZE_FLAG");
    std::string tutorial;
    std::string throw_key;
    bool valid_type = true;

    Runtime() {
        memory[0x1128u] = 0x8000u;
        memory[0x1138u] = 0x8100u;
        memory[0x1148u] = 0x8200u;
        memory[0x1004u] = 10u;
        memory[0x90dec0u] = 8u;
        memory[0x6018u] = 0x11u;
    }

    void PrepareTarget(std::uint32_t target,
                       std::string_view value) override {
        Check(target == 0x4000u && value == "GA_SERIALIZE_FLAG",
              "serialization marker and target");
        calls.emplace_back("prepare marker");
    }
    std::optional<std::string> ReadMarker(std::uint32_t source) override {
        Check(source == 0x4000u, "deserialization source");
        calls.emplace_back("read marker");
        return marker;
    }
    void ProcessNativeRecords(std::uint32_t stream,
                              bool reading) override {
        Check(stream == 0x4000u, "native-record stream");
        calls.emplace_back(reading ? "records read" : "records write");
    }
    std::uint32_t CreateNode(std::string_view name) override {
        Check(name == "gameData", "serialized XML root");
        calls.emplace_back("create node");
        return 0x5000u;
    }
    std::uint32_t ParseNode(std::uint32_t source,
                            std::string_view name) override {
        Check(source == 0x4000u && name == "gameData",
              "parsed XML root and source");
        calls.emplace_back("parse node");
        return 0x5000u;
    }
    bool CheckVersionAndType(std::uint32_t node,
                             std::uint32_t source) override {
        Check(node == 0x5000u && source == 0x4000u,
              "version/type operands");
        calls.emplace_back("check type");
        return valid_type;
    }
    bool HasAttribute(std::uint32_t node,
                      std::string_view key) override {
        Check(node == 0x5000u, "attribute lookup node");
        calls.emplace_back("has " + std::string(key));
        return existing.contains(std::string(key));
    }
    void Record(std::uint32_t node, std::string_view key,
                std::string value, bool set) {
        Check(node == 0x5000u, "attribute node");
        calls.emplace_back(set ? "set " + std::string(key) :
                                 "add " + std::string(key));
        if (key == throw_key) throw std::runtime_error("attribute failed");
        attributes.push_back({std::string(key), std::move(value), set});
    }
    void SetInt(std::uint32_t node, std::string_view key,
                std::int32_t value) override {
        Record(node, key, std::to_string(value), true);
    }
    void AddInt(std::uint32_t node, std::string_view key,
                std::int32_t value) override {
        Record(node, key, std::to_string(value), false);
    }
    void SetString(std::uint32_t node, std::string_view key,
                   std::string_view value) override {
        Record(node, key, std::string(value), true);
    }
    void AddString(std::uint32_t node, std::string_view key,
                   std::string_view value) override {
        Record(node, key, std::string(value), false);
    }
    std::int32_t ReadInt(std::uint32_t node, std::string_view key,
                         std::int32_t fallback) override {
        Check(node == 0x5000u, "integer reader node");
        calls.emplace_back("read int " + std::string(key));
        defaults.emplace_back(key, fallback);
        const auto found = input_ints.find(std::string(key));
        return found == input_ints.end() ? fallback : found->second;
    }
    std::string ReadString(std::uint32_t node, std::string_view key,
                           std::string_view fallback) override {
        Check(node == 0x5000u && key == "TUTORIAL" && fallback.empty(),
              "tutorial string reader and default");
        calls.emplace_back("read tutorial");
        return tutorial;
    }
    std::uint8_t Write(std::uint32_t node,
                        std::uint32_t target) override {
        Check(node == 0x5000u && target == 0x4000u,
              "Config_Write operands");
        calls.emplace_back("write config");
        return 0xa5u;
    }
    void DestroyNode(std::uint32_t node) override {
        Check(node == 0x5000u, "XML document cleanup");
        calls.emplace_back("destroy node");
    }
    void ReleaseNativeRecords(std::uint32_t stream) override {
        Check(stream == 0x4000u, "native record cleanup");
        calls.emplace_back("release records");
    }
    void ReleaseMarker() override { calls.emplace_back("release marker"); }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return memory[address];
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        memory[address] = value;
        calls.emplace_back("write dword " + std::to_string(address));
    }
    std::string_view ReadCString(std::uint32_t address) override {
        return strings.at(address);
    }
    std::uint32_t ObjectInternalId(std::uint32_t object) override {
        Check(object == 0x1000u, "game data object ID");
        calls.emplace_back("internal ID");
        return 5u;
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("frame manager");
        return 0x3000u;
    }
    std::uint32_t TimerFrameDuration(std::uint32_t manager) override {
        Check(manager == 0x3000u, "timer duration manager");
        calls.emplace_back("timer duration");
        return 77u;
    }
    std::uint32_t GlobalDataSubsystem() override {
        calls.emplace_back("global data");
        return 0x6000u;
    }
    std::uint32_t PetManager() override {
        calls.emplace_back("pet manager");
        return 0x7000u;
    }
};

struct CoreRuntime final : pal4::IGameSystemDataRuntime {
    Runtime& persistence;
    explicit CoreRuntime(Runtime& value) : persistence(value) {}

    void InitializeNamedText(std::uint32_t, std::string_view) override {}
    void ReleaseNamedText(std::uint32_t) override {}
    void ZeroBytes(std::uint32_t address, std::uint32_t count) override {
        for (std::uint32_t i = 0; i < count; i += 4u)
            persistence.memory[address + i] = 0u;
    }
    void FillDwords(std::uint32_t address, std::uint32_t count,
                    std::uint32_t value) override {
        for (std::uint32_t i = 0; i < count; ++i)
            persistence.memory[address + 4u * i] = value;
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        persistence.memory[address] = value;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return persistence.memory[address];
    }
    void ErasePointerRange(std::uint32_t, std::uint32_t,
                               std::uint32_t) override { pointers.clear(); }
    void InsertPointer(std::uint32_t, std::uint32_t address,
                       bool) override { pointers.push_back(address); }
    std::vector<std::uint32_t> PointerEntries(std::uint32_t) override {
        return pointers;
    }
    void ReleasePointerCollection(std::uint32_t) override {}
    void Log(std::string_view, std::string_view,
             std::uint32_t line) override {
        persistence.calls.emplace_back(line == 40u ? "core begin" : "core end");
    }
    std::uint32_t UiFrameManager() override { return 0x3000u; }
    void UpdateTimerFrameText(std::uint32_t) override {}
    void InitializeTimerName(std::string_view) override {}
    void RemoveTimer(std::uint32_t, std::string_view) override {}
    std::uint32_t AllocateTimerCallback(std::uint32_t,
        std::uint32_t, std::uint32_t, std::uint32_t) override { return 0u; }
    std::uint32_t ConstructTimerEntry(std::string_view,
        std::uint32_t, std::uint32_t) override { return 1u; }
    void AddTimer(std::uint32_t, std::uint32_t) override {}
    void DestroyTimerEntry(std::uint32_t) override {}
    std::int32_t CleanupTimerName() override { return 0; }
    std::uint32_t GlobalDataSubsystem() override { return 0x6000u; }
    std::vector<std::uint32_t> pointers;
};

std::size_t Position(const Runtime& runtime, std::string_view call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call exists");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void SerializesEveryFieldAndBit() {
    Runtime runtime;
    runtime.existing = {"MVar", "dataInt4", "TUTORIAL"};
    for (std::uint32_t i = 0; i < 71u; ++i)
        runtime.memory[0x1008u + i * 4u] = 100u + i;
    runtime.memory[0x1164u] = 0x80000001u;
    runtime.memory[0x1170u] = 0x80000000u;
    const pal4::GameDataPersistence persistence(0x1000u);
    Check(persistence.Serialize(0x4000u, runtime) == 0xa5u &&
              runtime.memory[0x1120u] == 77u &&
              runtime.attributes.size() == 75u,
          "serialization returns low byte, snapshots timer and emits 75 fields");
    Check(runtime.attributes[0].key == "ElapsedTime" &&
              runtime.attributes[0].value == "15" &&
              runtime.attributes[1].key == "MVar" &&
              runtime.attributes[1].value == "8" &&
              runtime.attributes[1].set &&
              runtime.attributes[2].key == "MovieFlag" &&
              runtime.attributes[2].value == "17" &&
              runtime.attributes[3].key == "dataInt0" &&
              runtime.attributes[3].value == "100" &&
              runtime.attributes[7].key == "dataInt4" &&
              runtime.attributes[7].set &&
              runtime.attributes[73].key == "dataInt70" &&
              runtime.attributes[73].value == "77",
          "special names, integer values, upsert branches and 71 indices");
    for (std::uint32_t i = 0; i < 71u; ++i) {
        const auto expected = i == 70u ? 77u : 100u + i;
        Check(runtime.attributes[3u + i].key ==
                  "dataInt" + std::to_string(i) &&
                  runtime.attributes[3u + i].value ==
                      std::to_string(expected),
              "every numbered attribute preserves index and value");
    }
    const auto& bits = runtime.attributes[74];
    Check(bits.key == "TUTORIAL" && bits.set &&
              bits.value.size() == 128u && bits.value.front() == '1' &&
              bits.value[31] == '0' && bits.value[96] == '1' &&
              bits.value.back() == '1',
          "128-bit tutorial string uses high-to-low bit order");
    Check(runtime.calls[0] == "prepare marker" &&
              Position(runtime, "timer duration") <
                  Position(runtime, "records write") &&
              Position(runtime, "records write") <
                  Position(runtime, "create node") &&
              Position(runtime, "write config") <
                  Position(runtime, "destroy node") &&
              runtime.calls[runtime.calls.size() - 2u] == "release records" &&
              runtime.calls.back() == "release marker",
          "marker, timer, native records, XML and cleanup order");
}

void DeserializesDefaultsGlobalsAndTutorial() {
    Runtime runtime;
    runtime.input_ints = {{"ElapsedTime", 42}, {"MVar", 31},
                          {"MovieFlag", 6}, {"dataInt0", 7},
                          {"dataInt21", 121}, {"dataInt27", 127},
                          {"dataInt68", 168}, {"dataInt69", 169},
                          {"dataInt70", 170}};
    runtime.tutorial = "1001";
    CoreRuntime core(runtime);
    const pal4::GameDataPersistence persistence(0x1000u);
    Check(persistence.Deserialize(0x4000u, runtime, core),
          "valid marker and node deserialize");
    Check(runtime.memory[0x1004u] == 42u &&
              runtime.memory[0x90dec0u] == 31u &&
              runtime.memory[0x6018u] == 0x17u &&
              runtime.memory[0x1008u] == 7u &&
              runtime.memory[0x1028u] == 1u &&
              runtime.memory[0x1058u] == 1u &&
              runtime.memory[0x105cu] == 121u &&
              runtime.memory[0x1060u] == 0u &&
              runtime.memory[0x1070u] == 1u &&
              runtime.memory[0x90dec4u] == 121u &&
              runtime.memory[0x90ded8u] == 127u &&
              runtime.memory[0x90df7cu] == 168u &&
              runtime.memory[0x7654u] == 169u &&
              runtime.memory[0x1120u] == 170u &&
              runtime.memory[0x1164u] == 9u,
          "special values, flag OR, defaults, global mapping, pet and bit order");
    Check(runtime.defaults.size() == 74u &&
              runtime.defaults[3u + 8u] ==
                  std::pair{std::string("dataInt8"), 1} &&
              runtime.defaults[3u + 20u] ==
                  std::pair{std::string("dataInt20"), 1} &&
              runtime.defaults[3u + 26u] ==
                  std::pair{std::string("dataInt26"), 1} &&
              runtime.defaults[3u + 21u] ==
                  std::pair{std::string("dataInt21"), 0},
          "71 numbered defaults and three special defaults");
    for (std::uint32_t i = 0; i < 47u; ++i) {
        const auto local_index = i < 5u ? i + 21u : i + 22u;
        Check(runtime.memory[0x90dec4u + i * 4u] ==
                  runtime.memory[0x1008u + local_index * 4u],
              "every global receives its corresponding local dataInt");
    }
    Check(Position(runtime, "read marker") < Position(runtime, "core begin") &&
              Position(runtime, "core end") <
                  Position(runtime, "records read") &&
              Position(runtime, "check type") <
                  Position(runtime, "read int ElapsedTime") &&
              Position(runtime, "pet manager") <
                  Position(runtime, "read tutorial") &&
              runtime.calls.back() == "release marker",
          "deserialization validates before reading and cleans resources");
}

void RejectsInvalidInputsAndReleasesResources() {
    const pal4::GameDataPersistence persistence(0x1000u);
    Runtime wrong_marker;
    wrong_marker.marker = "bad";
    CoreRuntime first_core(wrong_marker);
    Check(!persistence.Deserialize(0x4000u, wrong_marker, first_core) &&
              wrong_marker.calls == std::vector<std::string>{
                  "read marker", "release marker"},
          "wrong marker exits before core reset");

    Runtime wrong_type;
    wrong_type.valid_type = false;
    CoreRuntime second_core(wrong_type);
    Check(!persistence.Deserialize(0x4000u, wrong_type, second_core) &&
              Position(wrong_type, "core end") <
                  Position(wrong_type, "check type") &&
              wrong_type.calls[wrong_type.calls.size() - 3u] ==
                  "destroy node" &&
              wrong_type.calls.back() == "release marker",
          "bad XML type cleans node and native records after core reset");

    Runtime save_failure;
    save_failure.throw_key = "dataInt5";
    try {
        (void)persistence.Serialize(0x4000u, save_failure);
        throw std::runtime_error("expected save failure");
    } catch (const std::runtime_error& error) {
        Check(std::string_view(error.what()) == "attribute failed",
              "attribute write failure propagates");
    }
    Check(save_failure.calls[save_failure.calls.size() - 3u] ==
              "destroy node" && save_failure.calls.back() ==
              "release marker",
          "save failure cleans XML and native records");

    Runtime bad_bits;
    bad_bits.tutorial = "10x";
    CoreRuntime third_core(bad_bits);
    try {
        (void)persistence.Deserialize(0x4000u, bad_bits, third_core);
        throw std::runtime_error("expected bitset failure");
    } catch (const std::invalid_argument& error) {
        Check(std::string_view(error.what()) == "invalid bitset<N> char",
              "IDA invalid tutorial character text");
    }
    Check(bad_bits.calls[bad_bits.calls.size() - 3u] == "destroy node" &&
              bad_bits.calls.back() == "release marker",
          "invalid tutorial character throws and cleans resources");
}

void TutorialLengthCapsAtFirst128Characters() {
    Runtime runtime;
    runtime.tutorial = std::string(128u, '0') + "1";
    CoreRuntime core(runtime);
    const pal4::GameDataPersistence persistence(0x1000u);
    Check(persistence.Deserialize(0x4000u, runtime, core) &&
              runtime.memory[0x1164u] == 0u,
          "native parser caps length at first 128 characters");
}

void MissingTutorialAndHighFlagBit() {
    Runtime runtime;
    runtime.input_ints["MovieFlag"] = -2147483647 - 1;
    CoreRuntime core(runtime);
    const pal4::GameDataPersistence persistence(0x1000u);
    Check(persistence.Deserialize(0x4000u, runtime, core) &&
              runtime.memory[0x6018u] == 0x80000011u &&
              runtime.memory[0x1164u] == 0u &&
              runtime.memory[0x1168u] == 0u &&
              runtime.memory[0x116cu] == 0u &&
              runtime.memory[0x1170u] == 0u,
          "missing tutorial clears bitset and signed high flag bit is ORed");
}

}  // namespace

int main() {
    try {
        SerializesEveryFieldAndBit();
        DeserializesDefaultsGlobalsAndTutorial();
        RejectsInvalidInputsAndReleasesResources();
        TutorialLengthCapsAtFirst128Characters();
        MissingTutorialAndHighFlagBit();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
