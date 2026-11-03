#include "pal4/game_data_role_record.h"
#include "pal4/game_data_role_registry.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

struct RegistryRuntime final : pal4::IGameDataRoleMasterRuntime {
    std::vector<pal4::GameDataFieldDescriptor> appended;
    std::vector<std::string> calls;
    std::uint32_t processed_collection = 0;
    std::size_t processed_count = 0;

    void InitializeGlobalStorage(std::uint32_t address) override {
        Check(address == 0x8ef194 || address == 0x8ef160,
              "Role child or master collection address");
        calls.emplace_back("globals");
    }
    void RegisterGlobalCleanup(std::uint32_t callback) override {
        Check(callback == 0x4e7db0 || callback == 0x4e8090,
              "Role child or master cleanup callback");
        calls.emplace_back("register");
    }
    void CleanupGlobalStorage(std::uint32_t address) override {
        Check(address == 0x8ef194 || address == 0x8ef160,
              "Role child or master cleanup address");
        calls.emplace_back("cleanup");
    }
    std::string SourceName(std::uint32_t address) override {
        calls.emplace_back("source");
        return "src" + std::to_string(address);
    }
    std::string ErrorSuffix() override { return "_E"; }
    void AppendField(std::vector<pal4::GameDataFieldDescriptor>& fields,
                     pal4::GameDataFieldDescriptor field) override {
        appended.push_back(field);
        fields.push_back(std::move(field));
        calls.emplace_back("append");
    }
    void Sort(std::vector<pal4::GameDataFieldDescriptor>& fields) override {
        calls.emplace_back("sort");
        std::sort(fields.begin(), fields.end(),
                  [](const auto& left, const auto& right) {
                      return left.offset < right.offset;
                  });
    }
    void MergeRoleFields(
        std::vector<pal4::GameDataFieldDescriptor>& destination,
        const std::vector<pal4::GameDataFieldDescriptor>& source,
        std::uint32_t offset) override {
        Check(offset == 144 && source.size() == 16,
              "Role child collection offset and size");
        calls.emplace_back("merge role");
        for (auto field : source) {
            field.offset += offset;
            destination.push_back(std::move(field));
        }
    }
    void MergeOtherFields(
        std::vector<pal4::GameDataFieldDescriptor>& destination,
        std::uint32_t offset) override {
        Check(offset == 0, "second child collection has zero offset");
        calls.emplace_back("merge other");
        destination.push_back({20, "OTHER", pal4::GameDataFieldKind::numeric,
                               0x843368, 24, 0, 0});
    }
    std::uint8_t ProcessCollection(
        std::uint32_t collection,
        const std::vector<pal4::GameDataFieldDescriptor>& fields) override {
        calls.emplace_back("process");
        processed_collection = collection;
        processed_count = fields.size();
        return 0x5b;
    }
};

void CheckField(const pal4::GameDataFieldDescriptor& actual,
                std::uint32_t offset, const std::string& name,
                pal4::GameDataFieldKind kind, std::uint32_t vtable,
                std::uint32_t object_size, std::uint32_t source,
                std::uint32_t source_count) {
    Check(actual == pal4::GameDataFieldDescriptor{
                        offset, name, kind, vtable, object_size,
                        source, source_count},
          "Role field descriptor matches IDA layout");
}

void CheckRegistry() {
    pal4::GameDataRoleRegistry registry;
    RegistryRuntime r;
    const auto& fields = registry.Initialize(r);
    Check(fields.size() == 16 && r.appended.size() == 16 &&
              r.calls.front() == "globals" &&
              r.calls[1] == "register" && r.calls.back() == "sort",
          "Role registry initializes globals then appends 16 fields and sorts");
    using Kind = pal4::GameDataFieldKind;
    CheckField(r.appended[0], 88, "PHYSICAL_ATK_TARGET_E",
               Kind::action_type, 0x844494, 32, 0x8ef438, 2);
    CheckField(r.appended[1], 92, "PHYSICAL_ATK_RANGE_E",
               Kind::action_type, 0x844494, 32, 0x8ef418, 2);
    CheckField(r.appended[2], 80, "PLAYER_TYPE_E",
               Kind::action_type, 0x844494, 32, 0x8ef9c8, 4);
    for (std::uint32_t i = 0; i < 4; ++i) {
        const auto source = 0x8ef1c8 + i * 16;
        CheckField(r.appended[3 + i], i * 16,
                   "src" + std::to_string(source) + "_E",
                   Kind::text, 0x844574, 24, source, 0);
    }
    for (std::uint32_t i = 0; i < 4; ++i) {
        const auto source = 0x8ef9c8 + i * 16;
        CheckField(r.appended[7 + i], 64 + i * 4,
                   "src" + std::to_string(source) + "_FAVOR",
                   Kind::numeric, 0x843368, 24, source, 0);
    }
    for (std::uint32_t i = 0; i < 5; ++i) {
        const auto source = 0x8efa58 + i * 16;
        CheckField(r.appended[11 + i], 96 + i * 16,
                   "src" + std::to_string(source) + "_E",
                   Kind::text, 0x844574, 24, source, 0);
    }
    constexpr std::array<std::uint32_t, 16> sorted_offsets{
        0, 16, 32, 48, 64, 68, 72, 76,
        80, 88, 92, 96, 112, 128, 144, 160};
    for (std::size_t i = 0; i < fields.size(); ++i)
        Check(fields[i].offset == sorted_offsets[i],
              "Role descriptor offsets sort by record position");
    const auto calls = r.calls.size();
    Check(&registry.Initialize(r) == &fields &&
              r.calls.size() == calls && r.appended.size() == 16,
          "repeat initialization does not duplicate registration");
    registry.Cleanup(r);
    Check(r.calls.back() == "cleanup" && fields.empty(),
          "Role cleanup releases global collection");
}

void CheckMasterRegistry() {
    pal4::GameDataRoleMasterRegistry registry;
    RegistryRuntime r;
    const auto& fields = registry.Initialize(r);
    Check(fields.size() == 18 && r.appended.size() == 17 &&
              r.calls[0] == "globals" && r.calls[1] == "register" &&
              r.calls[2] == "append" && r.calls[3] == "globals" &&
              r.calls[4] == "register" &&
              r.calls[r.calls.size() - 3] == "merge role" &&
              r.calls[r.calls.size() - 2] == "merge other" &&
              r.calls.back() == "sort",
          "master initializes and merges Role then other child collection");
    CheckField(r.appended[0], 228, "ROLE_ID_E",
               pal4::GameDataFieldKind::numeric, 0x843368,
               24, 0, 0);
    Check(fields.front().offset == 20 &&
              fields.back().offset == 304 &&
              std::count_if(fields.begin(), fields.end(),
                            [](const auto& field) {
                                return field.offset == 228 &&
                                       field.name == "ROLE_ID_E";
                            }) == 1,
          "master sorts merged offsets and retains ROLE_ID at 228");
    const auto call_count = r.calls.size();
    Check(&registry.Initialize(r) == &fields &&
              r.calls.size() == call_count,
          "master repeat initialization does not duplicate child fields");
    Check(registry.Process(0x9000, r) == 0x5b &&
              r.processed_collection == 0x9000 &&
              r.processed_count == 18 &&
              r.calls.back() == "process",
          "wrapper initializes then forwards external collection");
    registry.Cleanup(r);
    Check(r.calls.back() == "cleanup" && fields.empty(),
          "master cleanup releases its own global collection");
}

struct AttributeRuntime final : pal4::IGameDataRoleAttributeRuntime {
    std::vector<pal4::GameDataFieldDescriptor> appended;
    std::vector<std::string> calls;
    std::uint32_t processed_collection = 0;
    std::size_t processed_count = 0;
    void InitializeGlobalStorage(std::uint32_t address) override {
        Check(address == 0x8ef12c, "attribute collection address");
        calls.emplace_back("globals");
    }
    void RegisterGlobalCleanup(std::uint32_t callback) override {
        Check(callback == 0x4e84a0, "attribute cleanup callback");
        calls.emplace_back("register");
    }
    void CleanupGlobalStorage(std::uint32_t address) override {
        Check(address == 0x8ef12c, "attribute cleanup address");
        calls.emplace_back("cleanup");
    }
    std::string SourceName(std::uint32_t) override { return {}; }
    std::string ErrorSuffix() override { return "_E"; }
    void AppendField(std::vector<pal4::GameDataFieldDescriptor>& fields,
                     pal4::GameDataFieldDescriptor field) override {
        appended.push_back(field);
        fields.push_back(std::move(field));
        calls.emplace_back("append");
    }
    void Sort(std::vector<pal4::GameDataFieldDescriptor>& fields) override {
        calls.emplace_back("sort");
        std::sort(fields.begin(), fields.end(),
                  [](const auto& left, const auto& right) {
                      return left.offset < right.offset;
                  });
    }
    void MergeStatsFields(
        std::vector<pal4::GameDataFieldDescriptor>& fields,
        std::uint32_t offset) override {
        Check(offset == 0, "stats child offset is zero");
        fields.push_back({0, "STATS", pal4::GameDataFieldKind::numeric,
                          0x843368, 24, 0, 0});
        calls.emplace_back("merge stats");
    }
    std::uint8_t ProcessCollection(
        std::uint32_t collection,
        const std::vector<pal4::GameDataFieldDescriptor>& fields) override {
        processed_collection = collection;
        processed_count = fields.size();
        calls.emplace_back("process");
        return 0x6a;
    }
};

void CheckAttributeRegistry() {
    pal4::GameDataRoleAttributeRegistry registry;
    AttributeRuntime r;
    const auto& fields = registry.Initialize(r);
    Check(fields.size() == 5 && r.appended.size() == 4 &&
              r.calls == std::vector<std::string>{
                  "globals", "register", "append", "append",
                  "append", "append", "merge stats", "sort"},
          "attribute registry appends four fields before merging stats");
    constexpr std::array<const char*, 4> names{
        "ID_E", "ROLE_ID_E", "CA_LEVEL_E", "CA_EXPERIENCE_E"};
    for (std::uint32_t i = 0; i < 4; ++i)
        CheckField(r.appended[i], 144 + i * 4, names[i],
                   pal4::GameDataFieldKind::numeric,
                   0x843368, 24, 0, 0);
    Check(fields.front().offset == 0 &&
              fields.back().offset == 156,
          "merged attribute collection sorts by offset");
    const auto calls = r.calls.size();
    Check(&registry.Initialize(r) == &fields && r.calls.size() == calls,
          "repeat attribute initialization does not duplicate fields");
    Check(registry.Process(0x9000, r) == 0x6a &&
              r.processed_collection == 0x9000 &&
              r.processed_count == 5 && r.calls.back() == "process",
          "attribute wrapper forwards collection and result");
    registry.Cleanup(r);
    Check(r.calls.back() == "cleanup" && fields.empty(),
          "attribute cleanup releases collection");
}

struct RecordRuntime final : pal4::IGameDataRoleRecordRuntime {
    std::vector<std::uint32_t> texts;
    std::vector<std::uint32_t> writes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;
    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words[address];
    }
    void InitializeText(std::uint32_t address) override {
        texts.push_back(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        writes.push_back(address);
        words[address] = value;
    }
};

void CheckRecordInitialize() {
    RecordRuntime r;
    Check(pal4::GameDataRoleRecord::Initialize(0x2000, r) == 0x2000 &&
              r.texts == std::vector<std::uint32_t>{
                  0x2000, 0x2010, 0x2020, 0x2030,
                  0x2060, 0x2070, 0x2080, 0x2090, 0x20a0},
          "Role record constructs four leading and five trailing texts");
    Check(r.writes == std::vector<std::uint32_t>{
              0x2040, 0x2044, 0x2048, 0x204c,
              0x2058, 0x205c, 0x2050} &&
              r.words[0x2050] == 4,
          "Role record zeros six scalar slots then sets type four");
    for (std::uint32_t offset : {64u, 68u, 72u, 76u, 88u, 92u})
        Check(r.words[0x2000 + offset] == 0,
              "other Role scalar slots are zero");
}

void CheckNormalize() {
    RecordRuntime r;
    r.words[0x2000] = 6;
    for (std::uint32_t id = 1001; id <= 1004; ++id) {
        r.words[0x2000 + 228] = id;
        Check(pal4::GameDataRoleRecord::NormalizeAndValidate(
                  0x2000, r) &&
                  r.words[0x2000 + 224] == id - 1001,
              "IDs 1001 through 1004 normalize into four valid slots");
    }
    for (std::uint32_t id : {1000u, 1005u}) {
        r.words[0x2000 + 228] = id;
        r.reads.clear();
        Check(!pal4::GameDataRoleRecord::NormalizeAndValidate(
                  0x2000, r) &&
                  r.words[0x2000 + 224] == id - 1001 &&
                  r.reads == std::vector<std::uint32_t>{0x2000 + 228},
              "out-of-range index writes before rejection and skips category");
    }
    r.words[0x2000 + 228] = 1001;
    r.words[0x2000] = 7;
    Check(!pal4::GameDataRoleRecord::NormalizeAndValidate(
              0x2000, r) && r.words[0x2000 + 224] == 0,
          "category greater than six fails after valid index normalization");
    r.writes.clear();
    for (std::uint32_t id = 1001; id <= 1004; ++id) {
        r.words[0x2000 + 148] = id;
        Check(pal4::GameDataRoleRecord::HasValidLinkedRoleId(
                  0x2000, r),
              "linked role IDs 1001 through 1004 pass");
    }
    for (std::uint32_t id : {1000u, 1005u}) {
        r.words[0x2000 + 148] = id;
        Check(!pal4::GameDataRoleRecord::HasValidLinkedRoleId(
                  0x2000, r),
              "linked role ID below or above range fails");
    }
    Check(r.writes.empty() && r.reads.back() == 0x2000 + 148,
          "linked role validation only reads word at +148");
}

}  // namespace

int main() {
    try {
        CheckRegistry();
        CheckMasterRegistry();
        CheckAttributeRegistry();
        CheckRecordInitialize();
        CheckNormalize();
        std::cout << "game_data_role_registry_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
